Unbind Vanilla Controls (Oblivion Remastered)
====================================
Version 1.0.0

An original, GPL-3.0-or-later OBSE64 plugin for The Elder Scrolls IV: Oblivion Remastered.
On the game's own Controls page, pressing the key or button a slot already has unbinds it.

HOW TO USE IT
-------------
  1. Open System > Controls (keyboard or controller page).
  2. Select a row's slot to change it, as usual.
  3. Press the key or button that slot already has. The slot is emptied: the box turns red and the
     game warns that a control is unbound, as it does for any empty slot.
  4. To bind it again, select the slot and press any key - the same one included.
  5. Apply Changes to keep it.

Everything else on the page works as before: a different key rebinds the slot, and Revert / Reset to
default put the game's keys back.

GOOD TO KNOW
------------
  * A key the Controls page itself uses to cancel a rebind ends the rebind instead of being chosen,
    so a slot bound to such a key cannot be emptied by pressing it again. (B on a controller is not
    one of them - it unbinds like any other button.)
  * The mod changes nothing the game saves by itself: an emptied slot is saved by the game's own
    Apply Changes, in the game's own settings file.

REQUIREMENTS
------------
  * The Elder Scrolls IV: Oblivion Remastered (Steam, runtime 1.512.105)
  * OBSE64 (Oblivion Script Extender 64)
  * Address Library for OBSE Plugins

INSTALLING
----------
The plugin goes beside the game executable, in
OblivionRemastered\Binaries\Win64\OBSE\Plugins\. With Mod Organizer 2 that means the Root
folder layout (Root Builder); launch the game through OBSE64.

FILES
-----
  * OBSE/Plugins/UnbindVanillaControls.dll - the plugin (UnbindVanillaControls.pdb, its debug symbols, beside it)
  * OBSE/Plugins/UnbindVanillaControls.ini - bEnabled (1 = on) and the log level
  * The log is Documents/My Games/Oblivion Remastered/OBSE/Logs/UnbindVanillaControls.log.
    It records every key chosen on the Controls page and what the slot ended up with. It is written
    at info; set uLogLevel=0 in the INI for everything when reporting a problem.

LICENCE
-------
GPL-3.0-or-later, original work (LICENSE, NOTICE.md). The components it links and their
notices: THIRD_PARTY_NOTICES.md.
