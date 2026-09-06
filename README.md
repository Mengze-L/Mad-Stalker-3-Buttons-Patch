# Mad Stalker: Sprite HUD and Three-Button Patch

This project patches **Mad Stalker: Full Metal Forth** for the Sega Mega
Drive/Genesis. It replaces the gameplay Window-layer HUD with a sprite-based
HUD, improves its update performance, fixes compatibility problems in the
story and versus modes, and adds a dedicated guard button.

## Main features

- Replaces the gameplay Window HUD with eight priority sprites.
- Preserves the player score, health bar, timeout, `HI SCORE`, high-score
  digits, and enemy/boss health bar.
- Uses the original health-cell palette indices `$7-$2` and the game's original
  global palette colors.
- Supports story, `COM VS`, and `VS` gameplay modes.
- Reproduces the original per-side VS round-win medallions as sprites without
  overwriting the left `SCORE` graphics.
- Uses prepared transparent HUD graphics, so black background pixels do not
  cover the playfield.
- Replaces the five left `SCORE` letter patterns with a blinking `PAUSE` while
  paused; it consumes no extra sprite-table entries.
- Keeps the timer, score values, health bars, high-score HUD, and gameplay
  sprites frozen and visible while paused, then restores a steady `SCORE` on
  unpause.
- Initializes the player health bar when gameplay begins.
- Adjusts only Story Stage 3 Scene 0's Plane B tiles `$484-$487`: the upper orange
  (`#EF8A21`) and dark-orange (`#CE4521`, CRAM `$024C`) dither extends through
  7 scanlines, with dither line 3 shifted horizontally by one pixel and lines
  1 and 5 retaining their original phase, while the first three lower-transition
  scanlines remain shifted down two. This
  setup-only upload requires normalized scene state `$00`; later Stage 3
  resumes (normalized to `$0B`) skip it. It does not alter CRAM or add
  per-frame work.
- Darkens only the large Plane A display panels in the Story Stage 5 boss
  room. Eight private pattern copies replace pale palette-0 index `$D`
  (`#CECE8C`) with the existing darker index `$B` (`#8C8A63`) while retaining
  every border and detail pixel. The room transition only schedules the work;
  one VBlank performs the 256-byte upload and carries simultaneous ordinary
  HUD work into the following VBlank. It does not change CRAM, earlier Stage 5
  scenes, or the clean per-frame path.
- Uses a RAM HUD shadow, direct cell lookup, changed-cell queue, cached timer
  digits, and a limited VBlank update budget.
- Preserves the original timeout routine's return value, preventing Stage 3
  Continue from entering the game's dormant blocking input loop.
- Avoids rebuilding unchanged HUD sprite records and links every frame.
- Restores all eight fixed HUD sprite records once at each gameplay setup, so
  a soft reset cannot leave the original reset routine's `$0501` size words in
  the sprite HUD.
- Uses the HUD fast linker only on frames explicitly seeded by a gameplay
  wrapper; cutscenes and menus retain the original complete sprite linker.
- Works with 68000 address-error emulation enabled, including Genesis Plus GX.
- Keeps the hardware Window hidden on Stage Clear so sprite-HUD pattern data
  cannot appear as stray vertical-line tiles.
- Omits the eight gameplay HUD sprites on Story Stage Clear, allowing the
  original Stage Clear labels to start at sprite entry 0.
- Restores the `CLEAR BONUS` and `LEVEL BONUS` values with Stage Clear-only
  digit sprites while preserving the original bonus calculations.
- Keeps death/Game Over frames out of the Stage Clear sprite path, preserving
  Continue even when a stage-event state is still active.
- Leaves the original ROM unchanged.

## Controls

| Button | Action |
| --- | --- |
| A | Heavy punch |
| B | Light punch |
| C | Guard |

## Required original ROM

The normal build accepts only the verified original ROM revision:

- Filename used by the build: `ROMS\MADS.bin`
- Size: `2,097,152` bytes
- SHA-256: `946637995FC5781225EF1503112DF1F75B36A97F6BA39C371B03F6F23A7F82DD`

The build stops before patching if the ROM size or SHA-256 does not match.

## Build instructions

1. Place the verified original ROM at `ROMS\MADS.bin`.
2. Run or double-click `build.bat`.
3. Load the resulting `OUTPUT\MADS (Mengze).bin` in an emulator or on
   compatible hardware.

No command-line arguments are required:

```bat
build.bat
```

The normal build performs these steps automatically:

1. `_make_srec.bat` assembles `patch.asm` as a sparse Motorola S-record, then
   validates every record checksum and rejects overlapping patch ranges.
2. `_apply_srec.bat` validates the original ROM and applies the S-record.
3. `_fixcrc.bat` regenerates the Mega Drive header checksum.

The S-record validator does not impose a maximum patched address or output-ROM
size; it allows future patch data beyond the original 2 MiB image. The exact
2 MiB check applies only to identifying the required clean source ROM.

Expected patched output:

- Size: `2,097,152` bytes
- Header checksum: `$C6B6`
- SHA-256: `DE0F6B8A9906B4C58CE0F14DB69703C76EC4B1ED77FB89086AA2949460C20E86`

## Project structure

| Path | Purpose |
| --- | --- |
| `patch.asm` | Main sparse, ORG-based assembly patch entry point |
| `_validate_srec.ps1` | Rejects malformed or overlapping assembled S-record ranges |
| `SRC\` | Assembly implementation, symbols, ports, and RAM definitions |
| `ASSETS\` | Prepared transparent HUD and PAUSE graphics |
| `TOOLS\` | vasm, S-record patcher, and header-checksum utilities |
| `ROMS\` | Location of the verified original ROM |
| `OUTPUT\` | Normal patched-ROM output directory |
| `BUILD\` | Reference-build output and analysis artifacts |
| `MY BUTTON PATCH\` | Original standalone three-button patch source |
| `.agents\skills\patch-mad-stalker-sprite-hud\` | Project-local Codex workflow for maintaining this patch |

All ordinary project folder names are uppercase. The `.agents\skills` path
keeps Codex's required lowercase discovery name. Filenames inside the folders
retain their original spelling and capitalization.

## Build files

`build.bat` is the supported normal build command. It is location-independent,
stops immediately when a build stage fails, validates both prepared graphics
assets before assembly, and validates the assembled S-record before patching.

`build.ps1` is retained as an archived/reference builder. It can regenerate
the prepared graphics from the verified original ROM and performs additional
byte-level hook and code-cave checks, but it is not required for normal builds.

## Emulator notes

Genesis Plus GX and other accurate emulators may emulate 68000 address errors.
The patch uses aligned per-player longword accesses and is intended to run with
address-error emulation enabled. If the game does not advance beyond a black
screen, first confirm that the ROM was produced by the current `build.bat` and
matches the expected patched SHA-256 above.

Recommended gameplay checks after changing the assembly source:

- Start story mode and confirm the complete HUD and player health bar appear.
- Enter Story Stage 3 from a fresh stage setup and confirm the solid orange
  and dark-orange upper dither fills screen lines 0-6, with lines 7-23 solid
  orange, dither lines 1 and 5 in their original phase, and dither line 3
  shifted horizontally by one pixel.
  Confirm the first three lower transition lines remain shifted down two,
  while the later sky gradient and every other stage remain unchanged. Resume
  a later Stage 3 checkpoint and confirm this Scene 0-only upload is skipped.
- Confirm the player and enemy/boss health cells use the original colors and
  update correctly as damage is taken.
- Enter the Story Stage 5 boss room and confirm the large pale display panels
  use the darker gray-green index `$B`, while their borders, earlier Stage 5
  scenes, the health bars, and the rest of the background retain their
  original colors. Confirm the transition is clean on hardware-timed emulators,
  with no active-display tearing or delayed HUD corruption.
- Perform an emulator soft reset, enter Story mode again, and confirm every HUD
  label, number, health cell, and timer digit uses its correct sprite size.
- Pause and confirm that only the left `SCORE` label changes to a blinking
  `PAUSE`; the timer, score values, health bars, high-score HUD, and gameplay
  sprites must remain unchanged. Unpause and confirm that `SCORE` is restored
  correctly and no longer blinks.
- Complete stages 1-6 and confirm the normal gameplay HUD is absent from each
  Stage Clear screen, with no vertical-line or stray tiles. Confirm the Stage
  Clear labels and the numbers below both `CLEAR BONUS` and `LEVEL BONUS`
  appear and match the awarded score. Confirm the complete HUD returns at the
  start of the following stage. After Stage 3, confirm the following cutscene
  displays its complete sprite list.
- Trigger Game Over and use Continue, especially in Stage 3; confirm the Game
  Over sprites remain complete and Continue returns to the saved stage.
- Start both `COM VS` and `VS` and confirm both robots and the sprite HUD appear.
  Win rounds with each side and confirm one or two medallions appear at the
  original side-specific positions without changing `SCORE`; confirm earned
  medallions remain visible in the following round and reset for a new match.
- Confirm A, B, and C perform heavy punch, light punch, and guard respectively.

## Technical documentation

See [README-sprite-hud.md](README-sprite-hud.md) for the detailed HUD design,
runtime optimizations, ROM/RAM allocation, VRAM use, and sprite layout.
