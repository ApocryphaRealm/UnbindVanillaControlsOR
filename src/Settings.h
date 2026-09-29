#pragma once

// UnbindVanillaControls.ini beside the plugin. Read once at load; the compiled defaults are the shipped INI's values
// (rule 16).

namespace settings
{
	struct Values
	{
		int  logLevel = 2;      // [Log] uLogLevel: 0 trace .. 4 error (shipped at info)
		bool enabled = true;    // [General] bEnabled: pressing a row's own key again unbinds it
	};

	void Load();
	const Values& Get();
	std::filesystem::path PluginFolder();   // ...\OblivionRemastered\Binaries\Win64\OBSE\Plugins
}
