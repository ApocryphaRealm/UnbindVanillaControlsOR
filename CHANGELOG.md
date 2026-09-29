# UnbindVanillaControlsOR - changelog

Written as changes happen, not reconstructed afterwards (rule 61). A version number is issued by the version gate
only once a build is seen working in game (rule 48); until then the work sits under Unreleased.

## Unreleased

### Added
- the owner, 2026-09-29: "add a simple functionality to every currently known binding so that you can press the same key a second time to unbind it". On the game's Controls page (keyboard and controller rows), pressing the key a slot already has hands the row "no key", so the slot is unbound; the row's key is taken from its selector when listening starts, or from the row's last known binding when the selector shows none.
- a read-back one frame after each choice, logged, so a log shows what the row ended up with.
- TestBench tool uvc.state (state, rows, select).
- the row's binding is read from its own action's mappings (GetRebindData -> InputAction and MappingContext; each Controls action keeps three entries there: keyboard primary, keyboard secondary, controller). The first in-game test (the owner, 2026-09-29: "it doesn't work") showed a selector's GetCurrentKey is only a key chosen on the page this session - every untouched row reported None, so 5 presses were seen and none matched. The selector's key still wins when it has one (a change not yet applied).
- after an unbind the selector is emptied too (UpdateSelectedKey with no key), and a slot this mod emptied counts as empty until a key is chosen for it again. The owner's second test, 2026-09-29: the game took the empty binding (red box, the unbound-control warning) but the box still drew Y, and Y could not be bound back - pressing it unbound it again, because the selector and the unapplied mapping still said Y. A key the page puts back (Revert, Reset to default) ends the cleared state.
- each choice on a row is logged at info (the key pressed and the key the row held), so a test log answers without raising the level.

### Known
- not tried in game yet (the owner, 2026-09-29: no in-game testing for now). Open: does the row accept an empty key, does Apply keep it (Save_Settings.sav) and does the next start keep it; keys on a selector's cancel list (Esc, controller B) cannot be unbound this way.
