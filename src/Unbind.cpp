#include "Unbind.h"

#include "PEHook.h"
#include "Reflect.h"
#include "Settings.h"
#include "Tick.h"
#include "Ue.h"

#include <unordered_map>

namespace unbind
{
	namespace
	{
		constexpr const wchar_t* kKeyboardRow = L"/Game/UI/Modern/MenuLayer/Settings/Rebind/Keyboard/WBP_Modern_Settings_KeyboardRebindWidget.WBP_Modern_Settings_KeyboardRebindWidget_C";
		constexpr const wchar_t* kGamepadRow = L"/Game/UI/Modern/MenuLayer/Settings/Rebind/Gamepad/WBP_Modern_Settings_GamepadRebindWidget.WBP_Modern_Settings_GamepadRebindWidget_C";

		// one binding slot of a row: the handler the selector's OnKeySelected delegate calls, and the row property
		// holding that selector (read from the running game's reflection data, 2026-09-29)
		struct Slot
		{
			const char*    handler;
			const wchar_t* handlerW;
			const char*    selectorProperty;
			const char*    name;   // the tool's name for it
		};
		constexpr std::array<Slot, 3> kSlots{ {
			{ "OnPrimaryKeyboardKeySelectorKeySelected", L"OnPrimaryKeyboardKeySelectorKeySelected", "PrimaryKeyboardKeySelector", "primary" },
			{ "OnSecondaryKeyboardKeySelectorKeySelected", L"OnSecondaryKeyboardKeySelectorKeySelected", "SecondaryKeyboardKeySelector", "secondary" },
			{ "OnGamepadKeySelectorKeySelected", L"OnGamepadKeySelectorKeySelected", "GamepadKeySelector", "gamepad" },
		} };
		constexpr int kOther = -1;
		constexpr int kListen = 100;
		constexpr int kUpdate = 101;
		// OnUpdateWidgetBinding's parameters, in kSlots order
		constexpr std::array<const char*, 3> kUpdateParams{ "NewPrimaryKeyboard", "NewSecondaryKeyboard", "NewPrimaryGamepad" };

		struct Pending
		{
			UE::UObject*  selector;
			const char*   slot;
			std::string   pressed;
			bool          unbound;
			std::uint64_t due;
		};

		Status                                         g_status;
		std::unordered_map<UE::UFunction*, int>        g_kind;     // learnt once per UFunction pointer
		std::unordered_map<UE::UObject*, std::string>  g_before;   // selector -> its key when it started listening
		// selector -> the last key the row was known to have (OnUpdateWidgetBinding, and every read-back): the fallback
		// when a selector that starts listening already shows no key (a "press a key" prompt may blank it first)
		std::unordered_map<UE::UObject*, std::string>  g_known;
		std::vector<Pending>                           g_pending;
		std::vector<UE::UObject*>                      g_rows;     // the tool's last row listing
		std::int32_t                                   g_chordKey = -2;   // FInputChord::Key's offset (-2: not read yet)

		void Note(std::string a_event)
		{
			logger::info("unbind: {}", a_event);
			g_status.recent.push_back(std::move(a_event));
			if (g_status.recent.size() > 12) {
				g_status.recent.erase(g_status.recent.begin());
			}
		}

		std::string KeyName(const void* a_key)
		{
			return a_key ? pe::Utf8(reinterpret_cast<const UE::FName*>(a_key)->ToString()) : std::string();
		}

		bool Bound(const std::string& a_key) { return !a_key.empty() && a_key != "None"; }

		// where FInputChord keeps its FKey (0 in UE5; read from the struct once, 0 when the struct cannot be found)
		std::int32_t ChordKeyOffset()
		{
			if (g_chordKey == -2) {
				auto* chord = UE::StaticFindObject<UE::UStruct>(nullptr, nullptr, L"/Script/Slate.InputChord");
				g_chordKey = chord ? reflect::Offset(chord, "Key") : -1;
				if (g_chordKey < 0) {
					logger::warn("unbind: FInputChord's Key not found by reflection - taking offset 0 (UE5's layout)");
					g_chordKey = 0;
				} else {
					logger::debug("unbind: FInputChord::Key at 0x{:X}", g_chordKey);
				}
			}
			return g_chordKey;
		}

		// the selector's key right now (GetCurrentKey returns an FInputChord); empty when it cannot be asked
		std::string SelectorKey(UE::UObject* a_selector)
		{
			if (!a_selector || !reflect::IsLive(a_selector)) {
				return {};
			}
			ue::Call c(a_selector, L"GetCurrentKey");
			auto* ret = static_cast<std::uint8_t*>(c ? c.At("ReturnValue") : nullptr);
			if (!ret) {
				return {};
			}
			c.Run();
			auto* key = reinterpret_cast<UE::FKey*>(ret + ChordKeyOffset());
			std::string name = KeyName(key);
			key->~FKey();   // the returned chord's key may hold key details: released here
			return name;
		}

		UE::UObject* SelectorOf(UE::UObject* a_row, const char* a_property)
		{
			auto* at = reflect::At<UE::UObject*>(a_row, reflect::Offset(a_row->GetClass(), a_property));
			return at ? *at : nullptr;
		}

		int Kind(UE::UFunction* a_fn)
		{
			if (auto it = g_kind.find(a_fn); it != g_kind.end()) {
				return it->second;
			}
			const std::string name = pe::FunctionName(a_fn);
			int kind = kOther;
			if (name == "OnSelectorListenStateChanged") {
				kind = kListen;
			} else if (name == "OnUpdateWidgetBinding") {
				kind = kUpdate;
			}
			for (std::size_t i = 0; i < kSlots.size(); ++i) {
				if (name == kSlots[i].handler) {
					kind = static_cast<int>(i);
				}
			}
			g_kind.emplace(a_fn, kind);
			return kind;
		}

		void OnListen(UE::UFunction* a_fn, void* a_params)
		{
			auto* fs = reinterpret_cast<UE::UStruct*>(a_fn);
			auto* target = reflect::At<UE::UObject*>(a_params, reflect::Offset(fs, "Target"));
			auto* state = reflect::At<bool>(a_params, reflect::Offset(fs, "NewState"));
			if (!target || !*target || !state) {
				logger::warn("unbind: OnSelectorListenStateChanged without its Target / NewState - its layout changed?");
				return;
			}
			if (!*state) {
				logger::debug("unbind: selector {} stopped listening", static_cast<const void*>(*target));
				return;
			}
			std::string key = SelectorKey(*target);
			const char* from = "its own key";
			if (!Bound(key)) {
				if (auto it = g_known.find(*target); it != g_known.end() && Bound(it->second)) {
					key = it->second;
					from = "the row's last known binding (the selector shows none while listening)";
				}
			}
			g_before[*target] = key;
			++g_status.listens;
			logger::debug("unbind: selector {} listening - the row holds '{}' ({})", static_cast<const void*>(*target), key.empty() ? "?" : key, from);
		}

		void OnKeySelected(UE::UObject* a_row, const Slot& a_slot, UE::UFunction* a_fn, void* a_params)
		{
			++g_status.selections;
			auto* chord = reflect::At<std::uint8_t>(a_params, reflect::Offset(reinterpret_cast<UE::UStruct*>(a_fn), "NewInputChord"));
			if (!chord) {
				logger::warn("unbind: {} without its NewInputChord - its layout changed?", a_slot.handler);
				return;
			}
			auto* key = reinterpret_cast<UE::FKey*>(chord + ChordKeyOffset());
			const std::string pressed = KeyName(key);
			auto* selector = SelectorOf(a_row, a_slot.selectorProperty);
			std::string before;
			if (auto it = g_before.find(selector); selector && it != g_before.end()) {
				before = it->second;
				g_before.erase(it);
			}
			const bool same = Bound(pressed) && pressed == before;
			if (same && settings::Get().enabled) {
				key->~FKey();
				new (key) UE::FKey();   // no key: the row records an empty binding for this slot
				++g_status.unbinds;
				Note(std::format("{} slot: '{}' pressed again - unbound", a_slot.name, pressed));
			} else {
				logger::debug("unbind: {} slot: '{}' chosen (held '{}'){} - passed to the game unchanged", a_slot.name, pressed,
					before.empty() ? "?" : before, same ? " - the mod is switched off (bEnabled=0)" : "");
			}
			if (selector) {
				g_pending.push_back({ selector, a_slot.name, pressed, same && settings::Get().enabled, tick::Reads() + 2 });
			}
		}

		// the row is told its bindings: remember them per selector
		void OnUpdate(UE::UObject* a_row, UE::UFunction* a_fn, void* a_params)
		{
			auto* fs = reinterpret_cast<UE::UStruct*>(a_fn);
			for (std::size_t i = 0; i < kSlots.size(); ++i) {
				if (reflect::Offset(a_row->GetClass(), kSlots[i].selectorProperty) < 0) {
					continue;   // a keyboard row has no gamepad selector, and the other way round
				}
				auto* key = reflect::At<std::uint8_t>(a_params, reflect::Offset(fs, kUpdateParams[i]));
				auto* selector = SelectorOf(a_row, kSlots[i].selectorProperty);
				if (key && selector) {
					g_known[selector] = KeyName(key);
					logger::debug("unbind: row {} {} binding is '{}'", static_cast<const void*>(a_row), kSlots[i].name, g_known[selector]);
				}
			}
		}

		void OnRowEvent(UE::UObject* a_row, UE::UFunction* a_fn, void* a_params)
		{
			if (!a_row || !a_fn || !a_params || !reflect::Ok()) {
				return;
			}
			const int kind = Kind(a_fn);
			if (kind == kListen) {
				OnListen(a_fn, a_params);
			} else if (kind == kUpdate) {
				OnUpdate(a_row, a_fn, a_params);
			} else if (kind >= 0) {
				OnKeySelected(a_row, kSlots[static_cast<std::size_t>(kind)], a_fn, a_params);
			}
		}

		void Attach(const wchar_t* a_path, bool& a_watching, const char* a_what)
		{
			if (a_watching) {
				return;
			}
			if (auto* cls = ue::Class(a_path)) {
				a_watching = pe::Watch(cls, &OnRowEvent);
				if (a_watching) {
					logger::info("unbind: watching the {} Controls rows - pressing a row's own key again unbinds it", a_what);
				} else if (g_status.problem.empty()) {
					g_status.problem = std::string("the ") + a_what + " row class could not be watched";
					logger::error("unbind: {}", g_status.problem);
				}
			}
		}
	}

	void Tick()
	{
		static std::uint64_t n = 0;
		++n;
		if ((!g_status.watchingKeyboard || !g_status.watchingGamepad) && n % 30 == 0) {
			// the row classes load with the settings menu; the reflected offsets must be proven first (rule 17: keep
			// asking until both exist)
			if (reflect::SelfCheck()) {
				Attach(kKeyboardRow, g_status.watchingKeyboard, "keyboard");
				Attach(kGamepadRow, g_status.watchingGamepad, "controller");
			}
		}
		if (g_pending.empty()) {
			return;
		}
		const auto now = tick::Reads();
		for (auto it = g_pending.begin(); it != g_pending.end();) {
			if (now < it->due) {
				++it;
				continue;
			}
			const std::string shown = SelectorKey(it->selector);
			if (!shown.empty()) {
				g_known[it->selector] = shown;
			}
			if (it->unbound) {
				Note(std::format("{} slot now shows '{}'{}", it->slot, shown.empty() ? "?" : shown,
					Bound(shown) ? " - the game kept the key (see the plan's fallback)" : " - empty, as intended"));
			} else {
				logger::debug("unbind: {} slot now shows '{}' after '{}'", it->slot, shown.empty() ? "?" : shown, it->pressed);
			}
			it = g_pending.erase(it);
		}
	}

	Status GetStatus() { return g_status; }

	std::vector<std::string> Rows()
	{
		g_rows.clear();
		std::vector<std::string> out;
		for (const auto* path : { kKeyboardRow, kGamepadRow }) {
			auto* cls = ue::Class(path);
			for (auto* row : cls ? reflect::Instances(cls) : std::vector<UE::UObject*>{}) {
				std::string line = std::format("{} {}:", g_rows.size(), path == kKeyboardRow ? "keyboard" : "controller");
				for (const auto& s : kSlots) {
					if (auto* sel = reflect::Offset(row->GetClass(), s.selectorProperty) >= 0 ? SelectorOf(row, s.selectorProperty) : nullptr) {
						line += std::format(" {}='{}'", s.name, SelectorKey(sel));
					}
				}
				g_rows.push_back(row);
				out.push_back(std::move(line));
			}
		}
		return out;
	}

	std::string Select(std::size_t a_row, const std::string& a_slot, const std::string& a_key)
	{
		if (a_row >= g_rows.size() || !reflect::IsLive(g_rows[a_row])) {
			return "no such row - list them with op rows first (the Controls page must be open)";
		}
		auto* row = g_rows[a_row];
		const Slot* slot = nullptr;
		for (const auto& s : kSlots) {
			if (a_slot == s.name && reflect::Offset(row->GetClass(), s.selectorProperty) >= 0) {
				slot = &s;
			}
		}
		auto* selector = slot ? SelectorOf(row, slot->selectorProperty) : nullptr;
		if (!selector) {
			return "that row has no " + a_slot + " slot";
		}
		// what a key press does: the selector starts listening (its key is remembered), then the key arrives
		g_before[selector] = SelectorKey(selector);
		++g_status.listens;
		ue::Call c(row, slot->handlerW);
		auto* chord = static_cast<std::uint8_t*>(c ? c.At("NewInputChord") : nullptr);
		if (!chord) {
			return std::string("the row has no ") + slot->handler;
		}
		auto* key = reinterpret_cast<UE::FKey*>(chord + ChordKeyOffset());
		new (key) UE::FKey(UE::FName(std::wstring(a_key.begin(), a_key.end()).c_str()));
		const auto before = g_status.unbinds;
		c.Run();   // through ProcessEvent, so the observer above sees it exactly as it sees a real press
		key->~FKey();
		return std::format("{} slot of row {} given '{}': {}", a_slot, a_row, a_key,
			g_status.unbinds > before ? "unbound (the read-back follows in the log)" : "passed to the game unchanged");
	}
}
