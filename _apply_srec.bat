@echo off
setlocal EnableDelayedExpansion

pushd "%~dp0" >nul
if errorlevel 1 exit /b 1

set "MADS_SOURCE_ROM=ROMS\MADS.bin"
set "MADS_OUTPUT_ROM=OUTPUT\MADS (Mengze).bin"
set "MADS_EXPECTED_SHA256=946637995FC5781225EF1503112DF1F75B36A97F6BA39C371B03F6F23A7F82DD"

if not exist "TOOLS\srecpatch.exe" (
    echo ERROR: TOOLS\srecpatch.exe was not found.
    popd
    exit /b 1
)
if not exist "!MADS_SOURCE_ROM!" (
    echo ERROR: !MADS_SOURCE_ROM! was not found.
    popd
    exit /b 1
)
if not exist "srecfile.txt" (
    echo ERROR: srecfile.txt was not found.
    popd
    exit /b 1
)

for %%F in ("!MADS_SOURCE_ROM!") do set "MADS_ROM_SIZE=%%~zF"
if not "!MADS_ROM_SIZE!"=="2097152" (
    echo ERROR: !MADS_SOURCE_ROM! must be exactly 2097152 bytes.
    popd
    exit /b 1
)

set "MADS_ROM_HASH="
for /f "skip=1 tokens=* delims=" %%H in ('certutil -hashfile "!MADS_SOURCE_ROM!" SHA256 2^>nul') do if not defined MADS_ROM_HASH set "MADS_ROM_HASH=%%H"
set "MADS_ROM_HASH=!MADS_ROM_HASH: =!"
if /i not "!MADS_ROM_HASH!"=="!MADS_EXPECTED_SHA256!" (
    echo ERROR: Wrong original ROM revision.
    echo Expected: !MADS_EXPECTED_SHA256!
    echo Found:    !MADS_ROM_HASH!
    popd
    exit /b 1
)

if not exist "OUTPUT" mkdir "OUTPUT"
if errorlevel 1 (
    echo ERROR: Could not create the OUTPUT directory.
    popd
    exit /b 1
)

"TOOLS\srecpatch.exe" "!MADS_SOURCE_ROM!" "!MADS_OUTPUT_ROM!" < "srecfile.txt"
if errorlevel 1 (
    echo ERROR: srecpatch failed.
    popd
    exit /b 1
)

popd
endlocal
exit /b 0
