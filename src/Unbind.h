#pragma once

// ============================================================================================================
// Press the same key again to unbind (the owner, 2026-09-29: "add a simple functionality to every currently known
// binding so that you can press the same key a second time to unbind it"). Plan: 4. plans\unbind-vanilla-controls-
// oblivion\PLAN.md.
//
// The game's Controls page rows (WBP_Modern_Settings_KeyboardRebindWidget_C / _GamepadRebindWidget_C, native
// VModernKeyboardRebindWidget / VModernGamepadRebindWidget) each own key selectors (VAltarNavigableInputKeySelector).
// A selector that starts listening tells its row through OnSelectorListenStateChanged(Target, true); the key the
// player then presses reaches the row as On<Primary|Secondary>KeyboardKeySelectorKeySelected / OnGamepadKey...
// (NewInputChord). Both arrive through ProcessEvent (the selector's delegates), so a ProcessEvent observer on the two
// row classes sees them before the row's own handler runs:
//   * listening starts  -> remember the selector's current key (GetCurrentKey): the row's binding at that moment;
//   * a key is chosen   -> when it is that same key, the chord handed to the row carries NO key instead, so the
//                          row's handler records an empty binding - the slot is unbound.
// The next frame the selector's key is read back and logged, so a log shows what the row ended up with.
// ============================================================================================================

namespace unbind
{
	void Tick();                 // every frame on the game thread: attach to the row classes once loaded; read back

	struct Status
	{
		bool          watchingKeyboard = false;
		bool          watchingGamepad = false;
		std::uint64_t listens = 0;       // selectors that started listening
		std::uint64_t selections = 0;    // keys chosen on a row
		std::uint64_t unbinds = 0;       // chosen keys that matched the row's own and were turned into "no key"
		std::string   problem;
		std::vector<std::string> recent;   // the last events, newest last
	};
	Status GetStatus();          // game thread

	// For the TestBench tool (game thread): every live row with its selectors' keys, and driving one row's handler
	// with a chosen key exactly as a key press would (listening start recorded first, then the key).
	std::vector<std::string> Rows();
	std::string Select(std::size_t a_row, const std::string& a_slot, const std::string& a_key);
}
