@echo off
setlocal EnableDelayedExpansion

pushd "%~dp0SRC" >nul
if errorlevel 1 exit /b 1

if not exist "..\TOOLS\vasmm68k_mot_win32.exe" (
    echo ERROR: TOOLS\vasmm68k_mot_win32.exe was not found.
    popd
    exit /b 1
)
if not exist "..\ASSETS\hud_prepared.bin" (
    echo ERROR: ASSETS\hud_prepared.bin was not found.
    popd
    exit /b 1
)
if not exist "..\ASSETS\hud_pause_prepared.bin" (
    echo ERROR: ASSETS\hud_pause_prepared.bin was not found.
    popd
    exit /b 1
)

for %%F in ("..\ASSETS\hud_prepared.bin") do if not "%%~zF"=="3200" (
    echo ERROR: ASSETS\hud_prepared.bin must be exactly 3200 bytes.
    popd
    exit /b 1
)
for %%F in ("..\ASSETS\hud_pause_prepared.bin") do if not "%%~zF"=="160" (
    echo ERROR: ASSETS\hud_pause_prepared.bin must be exactly 160 bytes.
    popd
    exit /b 1
)

set "MADS_EXPECTED_HUD_ASSET_SHA256=EF85ACEDDCC8E29128A8B1106F74D71CD0B245E46380875D5B51522F9492BBB1"
set "MADS_EXPECTED_PAUSE_ASSET_SHA256=67BE7B71F8002319C7ABE7FE84B3383D650A099DEAFC7D61FC2D6D2C62388542"
set "MADS_HUD_ASSET_HASH="
for /f "skip=1 tokens=* delims=" %%H in ('certutil -hashfile "..\ASSETS\hud_prepared.bin" SHA256 2^>nul') do if not defined MADS_HUD_ASSET_HASH set "MADS_HUD_ASSET_HASH=%%H"
set "MADS_HUD_ASSET_HASH=!MADS_HUD_ASSET_HASH: =!"
if /i not "!MADS_HUD_ASSET_HASH!"=="!MADS_EXPECTED_HUD_ASSET_SHA256!" (
    echo ERROR: ASSETS\hud_prepared.bin failed its SHA-256 check.
    popd
    exit /b 1
)
set "MADS_PAUSE_ASSET_HASH="
for /f "skip=1 tokens=* delims=" %%H in ('certutil -hashfile "..\ASSETS\hud_pause_prepared.bin" SHA256 2^>nul') do if not defined MADS_PAUSE_ASSET_HASH set "MADS_PAUSE_ASSET_HASH=%%H"
set "MADS_PAUSE_ASSET_HASH=!MADS_PAUSE_ASSET_HASH: =!"
if /i not "!MADS_PAUSE_ASSET_HASH!"=="!MADS_EXPECTED_PAUSE_ASSET_SHA256!" (
    echo ERROR: ASSETS\hud_pause_prepared.bin failed its SHA-256 check.
    popd
    exit /b 1
)

if exist "..\srecfile.txt" del /q "..\srecfile.txt"

"..\TOOLS\vasmm68k_mot_win32.exe" "..\patch.asm" -I.. -m68000 -spaces -chklabels -nocase -rangewarnings -Dvasm=1 -DBuildGEN=1 -Fsrec -o "..\srecfile.txt"
if errorlevel 1 (
    echo ERROR: vasm failed.
    popd
    exit /b 1
)
if not exist "..\srecfile.txt" (
    echo ERROR: vasm did not create srecfile.txt.
    popd
    exit /b 1
)

powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "..\_validate_srec.ps1" -Path "..\srecfile.txt"
if errorlevel 1 (
    echo ERROR: S-record validation failed.
    popd
    exit /b 1
)

popd
endlocal
exit /b 0
