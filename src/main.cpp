// Unbind Vanilla Controls for Oblivion Remastered - plan: 4. plans\unbind-vanilla-controls-oblivion\PLAN.md.
#include "Settings.h"
#include "Tick.h"
#include "Tool.h"
#include "Unbind.h"

namespace
{
	// every frame (from the message pump)
	void OnFrame()
	{
		tool::Pump();
		unbind::Tick();
		static bool          toolRegistered = false;
		static std::uint64_t n = 0;
		if (!toolRegistered && ++n % 60 == 0) {
			toolRegistered = tool::Register();
		}
	}

	void OnMessage(OBSE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg || a_msg->type != OBSE::MessagingInterface::kPostLoad) {
			return;
		}
		tick::Install(&OnFrame);
	}
}

OBSE_PLUGIN_LOAD(const OBSE::LoadInterface* a_obse)
{
	OBSE::Init(a_obse);
	settings::Load();
	const auto level = static_cast<spdlog::level::level_enum>(std::clamp(settings::Get().logLevel, 0, 4));
	logger::set_level(level, level);
	// rule 14: the log names its level, what it captures and how to get everything
	logger::info("Unbind Vanilla Controls {} loaded (Oblivion Remastered) - log level {} ({}); set uLogLevel=0 in "
				 "UnbindVanillaControls.ini to capture everything",
		UVC_VERSION, settings::Get().logLevel, level <= spdlog::level::debug ? "debug lines included" : "debug lines left out");
	if (!settings::Get().enabled) {
		logger::info("bEnabled=0: rows are watched and logged, but a repeated key is passed to the game unchanged");
	}
	if (auto* messaging = OBSE::GetMessagingInterface(); !messaging || !messaging->RegisterListener(&OnMessage)) {
		logger::error("OBSE messaging unavailable - the Controls page will not be watched");
	}
	return true;
}
