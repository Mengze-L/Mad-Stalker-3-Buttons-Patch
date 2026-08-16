# Mad Stalker: Sprite HUD and Three-Button Patch

This project patches **Mad Stalker: Full Metal Forth** for the Sega Mega
Drive/Genesis. It replaces the gameplay Window-layer HUD with a sprite-based
HUD, improves its update performance, fixes compatibility problems in the
story and versus modes, and adds a dedicated guard button.

## Main features

- Replaces the gameplay Window HUD with eight priority sprites.
- Preserves the player score, health bar, timeout, `HI SCORE`, high-score
  digits, and enemy/boss health bar.
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
- Uses a RAM HUD shadow, direct cell lookup, changed-cell queue, cached timer
  digits, and a limited VBlank update budget.
- Avoids rebuilding unchanged HUD sprite records and links every frame.
- Uses the HUD fast linker only on frames explicitly seeded by a gameplay
  wrapper; cutscenes and menus retain the original complete sprite linker.
- Works with 68000 address-error emulation enabled, including Genesis Plus GX.
- Keeps the hardware Window hidden on Stage Clear so sprite-HUD pattern data
  cannot appear as stray vertical-line tiles.
- Omits the eight gameplay HUD sprites on Story Stage Clear, allowing the
  original Stage Clear labels to start at sprite entry 0.
- Restores the `CLEAR BONUS` and `LEVEL BONUS` values with Stage Clear-only
  digit sprites while preserving the original bonus calculations.
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

1. `_make_srec.bat` assembles `patch.asm` as a sparse Motorola S-record.
2. `_apply_srec.bat` validates the original ROM and applies the S-record.
3. `_fixcrc.bat` regenerates the Mega Drive header checksum.

Expected patched output:

- Size: `2,097,152` bytes
- Header checksum: `$407F`
- SHA-256: `50CBF4092DE40FE6036E9A5B56A5969057E1C6EB89371B6D5BA7139946967E60`

## Project structure

| Path | Purpose |
| --- | --- |
| `patch.asm` | Main sparse, ORG-based assembly patch entry point |
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
stops immediately when a build stage fails, and validates both prepared
graphics assets before assembly.

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
- Start both `COM VS` and `VS` and confirm both robots and the sprite HUD appear.
  Win rounds with each side and confirm one or two medallions appear at the
  original side-specific positions without changing `SCORE`; confirm earned
  medallions remain visible in the following round and reset for a new match.
- Confirm A, B, and C perform heavy punch, light punch, and guard respectively.

## Technical documentation

See [README-sprite-hud.md](README-sprite-hud.md) for the detailed HUD design,
runtime optimizations, ROM/RAM allocation, VRAM use, and sprite layout.
