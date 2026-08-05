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
- Uses prepared transparent HUD graphics, so black background pixels do not
  cover the playfield.
- Replaces the Window-layer pause display with two temporary `PAUSE` sprites.
- Keeps the HUD and gameplay sprites frozen while paused.
- Initializes the player health bar when gameplay begins.
- Uses a RAM HUD shadow, direct cell lookup, changed-cell queue, cached timer
  digits, and a limited VBlank update budget.
- Avoids rebuilding unchanged HUD sprite records and links every frame.
- Works with 68000 address-error emulation enabled, including Genesis Plus GX.
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
- Header checksum: `$CA48`
- SHA-256: `C00BE1A3C4F89D96117400B9C24333211CACD34BC0E9BC0EE0BECA23229520BB`

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
- Pause and unpause, confirming that gameplay sprites remain unchanged.
- Complete a stage and inspect the stage-clear HUD.
- Start both `COM VS` and `VS` and confirm both robots and the sprite HUD appear.
- Confirm A, B, and C perform heavy punch, light punch, and guard respectively.

## Technical documentation

See [README-sprite-hud.md](README-sprite-hud.md) for the detailed HUD design,
runtime optimizations, ROM/RAM allocation, VRAM use, and sprite layout.
