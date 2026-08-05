@echo off
setlocal EnableDelayedExpansion

pushd "%~dp0" >nul
if errorlevel 1 exit /b 1

set "MADS_OUTPUT_ROM=OUTPUT\MADS (Mengze).bin"

if not exist "TOOLS\fixheader.exe" (
    echo ERROR: TOOLS\fixheader.exe was not found.
    popd
    exit /b 1
)
if not exist "!MADS_OUTPUT_ROM!" (
    echo ERROR: !MADS_OUTPUT_ROM! was not found.
    popd
    exit /b 1
)

"TOOLS\fixheader.exe" "!MADS_OUTPUT_ROM!"
if errorlevel 1 (
    echo ERROR: fixheader failed.
    popd
    exit /b 1
)

set "MADS_OUTPUT_HASH="
for /f "skip=1 tokens=* delims=" %%H in ('certutil -hashfile "!MADS_OUTPUT_ROM!" SHA256 2^>nul') do if not defined MADS_OUTPUT_HASH set "MADS_OUTPUT_HASH=%%H"
set "MADS_OUTPUT_HASH=!MADS_OUTPUT_HASH: =!"
echo SHA-256: !MADS_OUTPUT_HASH!

popd
endlocal
exit /b 0
