# Sprite HUD patch

This patch disables the three-row Window HUD during gameplay, revealing the
top 24 pixels of Plane A, and recreates the player HUD, timer, and enemy/boss
health bar with eight priority sprites, including `HI SCORE` and its
eight-digit display.  Existing HUD draw calls now update an authoritative RAM
shadow directly; gameplay never writes or reads a Window name table in VRAM.

The patch carries a private uncompressed copy of every HUD tile in `ASSETS\`
with opaque palette index 1 changed to sprite color 0.  The game's original
resource data remains untouched.  Runtime atlas updates copy directly
from the prepared ROM tiles, making the space around HUD lettering transparent
without reading source VRAM, scanning CRAM, or converting pixels every frame.
Changed cells are refreshed with an eight-tile-per-VBlank budget; timer and
score/HP updates are separated when they occur in the same frame to avoid HUD
sprite tearing or flicker.  A RAM shadow of the logical HUD map suppresses
unchanged writes, preventing left-to-right update starvation.
Logical HUD cell keys use a 96-byte direct index, avoiding the previous
linear scan through all 60 sprite-atlas cells on every HUD write.
Changed atlas indices are queued and VBlank consumes at most eight directly,
so a one-cell update no longer scans all 60 dirty-bit positions.
Queued cells use their RAM shadow words directly instead of rereading the
Window map from VRAM.  Full atlas rebuilds now use the same RAM shadow, so no
HUD path depends on Window VRAM.  The eight constant HUD sprite records are copied
only when a staging-table signature shows that a screen transition changed
them.  Clean VBlanks bypass the refresh routine completely.
Prepared timeout graphics also eliminate the original pair of 128-byte
source-VRAM DMAs; only the four tiles belonging to a changed digit are copied.
During gameplay, sprite links 0-6 are initialized once and then preserved.
Only HUD link 7 and the gameplay-sprite portion of the chain are rebuilt each
VBlank.  Ordinary story gameplay sets a one-use `$FF` marker, VS uses `$56`,
and both reserve HUD entries 0-7.  The VS marker appends earned round-medallion
sprites after all robots and effects.  Story Stage Clear instead sets a `$5A`
marker and starts its dynamic list at entry 0.  The global linker consumes the
marker; markerless cutscenes and menus clear any stale HUD-link signature and
use the original general-purpose linker to rebuild every sprite link.
System setup leaves hidden Window rows zero-filled, so padding cells begin
transparent without requiring a costly full atlas build during gameplay.
The pause display no longer exposes or writes the Window layer and does not add
any SAT entries.  It temporarily copies the five prepared `PAUSE` tiles over
the five unique pattern cells used by the left `SCORE` label, then alternates
30 visible frames with 15 transparent frames.  The blink counter exists only
on the blocking pause routine's stack.  HUD pattern refresh is held while
paused, so the timer, score values, health bars, high-score display, boss bar,
and complete gameplay sprite list stay frozen and visible.  Every unpause path
discards the local counter and restores only the five original `SCORE`
patterns, so blinking cannot continue after gameplay resumes; normal queued
HUD work then resumes.
The story and shared COM VS/VS battle loops both seed the same eight fixed HUD
records before creating dynamic sprites.  The VS selection screen retains its
original display, while its handoff to battle hides the Window so the sprite
HUD is used consistently in all gameplay modes.  The original VS result path
loads a two-tile round medallion at VRAM `$1980/$19A0`, which now belongs to the
left `SCORE` sprite.  Both winner paths are redirected to `$1800/$1820`, and
the art is reloaded once after each round transition clears the compatibility
rows.  The original 1P/2P win counters drive 1x2 sprites at the original
positions; successive wins move eight pixels right.  These records are added
after dynamic sprites with a 64-entry guard, so they cannot shift or replace a
robot or effect.
Stage Clear also keeps the hardware Window hidden.  The original full-height
Window setup exposed rows 3-27, whose VRAM now holds sprite-HUD pattern pixels
rather than Window name-table words; interpreting those pixels as tile
references caused the vertical-line artifacts seen after later stages.  The
Stage Clear labels are sprites, but the original `CLEAR BONUS` and `LEVEL
BONUS` values were written to Window rows 16 and 22.  Story Stage Clear now
omits the eight gameplay HUD records, lets its original label sprites begin at
entry 0, then appends the same two calculations as right-aligned digit sprites
using the game's already-loaded tile `$1A8-$1B1` font.  Only the significant
digits consume SAT entries, the original eight-cell field width is enforced,
and the Window remains hidden.  The original full linker finalizes this
zero-based list; VS remains on the normal eight-entry HUD path.
Per-frame gameplay gating, Stage Clear state detection, and count preparation
live with the digit renderer, not inside the fixed sprite-link block.  This
keeps the linker below the PAUSE routine at `$1FD2A0`, while the one-use marker
prevents Stage 3 and later cutscenes from inheriting the gameplay-only fast
linker.
The merged three-button control patch maps A to heavy punch, B to light punch,
and C to guard.  Its guard/dash helper uses aligned per-player longword access,
so the layout works for both players with 68000 address-error emulation enabled.

The patch is guarded for this exact original ROM:

- Size: `2097152` bytes
- SHA-256: `946637995FC5781225EF1503112DF1F75B36A97F6BA39C371B03F6F23A7F82DD`

## Build

Place the verified original ROM at `ROMS\MADS.bin`, then run:

```bat
build.bat
```

The normal build uses `TOOLS\vasmm68k_mot_win32.exe` to assemble `patch.asm`
as a sparse S-record overlay, applies it to a copy of the verified ROM, and
runs `fixheader.exe` to regenerate the Mega Drive header checksum.  The output
is `OUTPUT\MADS (Mengze).bin`.  The original ROM is never modified.

`patch.asm` remains the single complete, ORG-based assembly entry point, while
the implementation stays organized in `SRC\*.inc`.  The prepared transparent
HUD and PAUSE pixel data is stable patch source under `ASSETS\`, rather than a
temporary build product.  `build.ps1` is retained only as an archived/reference
builder that can regenerate those assets and perform its additional byte-level
guards; it is not part of the normal build workflow.

## Resource use

- Sprite staging entries: 0-7 for the fixed HUD during ordinary story and VS
  gameplay; VS appends up to three normal-match medallion records after dynamic
  sprites; Story Stage Clear starts its labels at entry 0 and appends up to
  eight one-tile digits for each of its two bonus values
- VRAM `$1800-$183F`: VS-only two-tile medallion patterns, reloaded each round
- VRAM `$1840-$18BF`: no gameplay Window dependency; retained as logical RAM keys
- VRAM `$18C0-$1FFF`: 58-tile main sprite atlas in protected hidden Window rows
- VRAM `$17C0-$17FF`: two-tile `HI` label sprite slot
- VRAM `$18C0`, `$1920`, `$1980`, `$19E0`, `$1A40`: five left `SCORE`
  pattern cells temporarily replaced by `PAUSE`
- ROM `$1FBD4A-$1FD9E3`: code, lookup tables, prepared art, and fast paths
- ROM `$1FC560-$1FD1DF`: prepared uncompressed transparent HUD tiles
- ROM `$1FD500-$1FD59F`: prepared uncompressed transparent PAUSE tiles
- ROM `$1FD5A0-$1FD737`: frame selectors, Stage Clear renderer, and VS medallion support
- ROM `$1FD9A0-$1FD9E3`: aligned three-button guard/dash helper
- RAM `$FFFFF663`: one-byte HUD dirty flags
- RAM `$FFFFF680-$FFFFF681`: cached timeout right/left digit values
- RAM `$FFFFF682`: timeout-cache validity marker
- RAM `$FFFFF683`: fixed HUD-link validity marker
- RAM `$FFFFF684-$FFFFF68C`: per-cell HUD dirty bits
- RAM `$FFFFF68D-$FFFFF694`: per-cell shadow-valid bits
- RAM `$FFFFF695`: one-use frame selector (`$FF` = Story HUD, `$56` = VS HUD/medallions, `$5A` = Story Stage Clear)
- RAM `$FFFFF696-$FFFFF70D`: logical HUD map shadow
- RAM `$FFFFF70E`: changed-cell queue count
- RAM `$FFFFF70F-$FFFFF74A`: changed-cell queue (up to 60 unique indices)

Each gameplay setup resets the RAM shadow before drawing its initial health
bars and the rest of the HUD.  Shared Window clears retain a three-row compatibility clear because
non-game screens still use the hardware Window; no such clear can reach the
sprite atlas at `$18C0`.  Stage Clear leaves the Window hidden instead of
exposing the protected lower rows.
