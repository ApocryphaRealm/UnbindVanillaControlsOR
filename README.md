# Unbind Vanilla Controls (Oblivion Remastered)

An OBSE64 plugin for The Elder Scrolls IV: Oblivion Remastered. On the game's own Controls page - keyboard and
controller - press the key or button a row already has, and that slot is unbound.

**Version 1.0.0.**

## How it works

The Controls page's rows (`WBP_Modern_Settings_KeyboardRebindWidget_C`, `WBP_Modern_Settings_GamepadRebindWidget_C`)
each own key selectors. When a selector starts listening it tells its row (`OnSelectorListenStateChanged`), and the key
the player presses reaches the row as `OnPrimaryKeyboardKeySelectorKeySelected`, `OnSecondaryKeyboardKeySelectorKeySelected`
or `OnGamepadKeySelectorKeySelected`. Both arrive through ProcessEvent, so the plugin watches the two row classes
(a vtable entry swap - nothing is patched): it remembers the row's key when listening starts, and when the key pressed
is that same key it hands the row "no key" instead, so the row records an empty binding. The next frame it reads the
selector back and logs what the row shows.

A key on a selector's cancel list ends listening instead of being chosen, so a row bound to one of those cannot be
unbound by pressing it again (B on a controller is not one - it unbinds like any other button). After an unbind the
selector is emptied too, and the slot counts as empty until a key is chosen for it again, so the same key binds it back.

## Settings

`UnbindVanillaControls.ini` beside the plugin: `[General] bEnabled=1`, `[Log] uLogLevel=2` (0 = everything).
The log is `Documents\My Games\Oblivion Remastered\OBSE\Logs\UnbindVanillaControls.log`.

## TestBench

`uvc.state`: op `state` (watched classes, counts, the last events), op `rows` (every live Controls row with its keys),
op `select {row, slot: primary|secondary|gamepad, key}` (drive a row's handler with a key, as a press would).

## Building

* [xmake](https://xmake.io) 3.0+, MSVC with C++23, and the submodule: `git clone --recurse-submodules`.
* `xmake f -p windows -a x64 -m releasedbg --toolchain=msvc`, then `xmake build UnbindVanillaControls`.
* The plugin and `dist/OblivionRemastered/Binaries/Win64/OBSE/Plugins/*` go to `OblivionRemastered\Binaries\Win64\OBSE\Plugins\`
  beside the game executable (Mod Organizer 2: the Root Builder layout).

## Licence

GPL-3.0-or-later (`LICENSE`); it links CommonLibOB64 (GPL-3.0). The modding exception from the CommonLibOB64 plugin
template is kept as `EXCEPTIONS`.
