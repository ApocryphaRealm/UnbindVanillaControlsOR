-- Unbind Vanilla Controls for The Elder Scrolls IV: Oblivion Remastered (OBSE64 plugin).
-- On the game's own Controls page, pressing the key or button a row already has unbinds it (the owner, 2026-09-29;
-- plan: 4. plans/unbind-vanilla-controls-oblivion/PLAN.md).
-- rule 45: no build-machine paths in any compiled object - set BEFORE includes() so CommonLibOB64's own library
-- target gets it too. /d1trimfile is wrapped in a TABLE so xmake passes it as one argument (logic library 7598); no
-- trailing separator. /PDBALTPATH:%_PDB% makes the debug directory record only the PDB's file name.
add_cxflags({"/d1trimfile:$(projectdir)"}, {force = true, expand = false})
add_shflags("/PDBALTPATH:%_PDB%", {force = true})

includes("lib/commonlibob64")

set_project("UnbindVanillaControls")
set_version("0.0.0")
set_license("GPL-3.0-or-later")
set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg")
add_requires("nlohmann_json")
add_rules("plugin.vsxmake.autoupdate")

target("UnbindVanillaControls")
    add_rules("commonlibob64.plugin", {
        name = "UnbindVanillaControls",
        author = "ApocryphaRealm",
        description = "Unbind Vanilla Controls - press a row's own key again on the Controls page to unbind it (Oblivion Remastered)"
    })
    add_syslinks("user32")
    add_packages("nlohmann_json")
    on_load(function (target)
        target:add("defines", "UVC_VERSION=\"" .. (target:version() or "0.0.0") .. "\"")
    end)
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
