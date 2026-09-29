# UnbindVanillaControlsOR - changelog

Written as changes happen, not reconstructed afterwards (rule 61). A version number is issued by the version gate
only once a build is seen working in game (rule 48); until then the work sits under Unreleased.

## Unreleased

### Added
- the owner, 2026-09-29: "add a simple functionality to every currently known binding so that you can press the same key a second time to unbind it". On the game's Controls page (keyboard and controller rows), pressing the key a slot already has hands the row "no key", so the slot is unbound; the row's key is taken from its selector when listening starts, or from the row's last known binding when the selector shows none.
- a read-back one frame after each choice, logged, so a log shows what the row ended up with.
- TestBench tool uvc.state (state, rows, select).

### Known
- not tried in game yet (the owner, 2026-09-29: no in-game testing for now). Open: does the row accept an empty key, does Apply keep it (Save_Settings.sav) and does the next start keep it; keys on a selector's cancel list (Esc, controller B) cannot be unbound this way.
