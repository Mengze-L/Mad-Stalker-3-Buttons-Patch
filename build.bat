@echo off
setlocal

pushd "%~dp0" >nul
if errorlevel 1 exit /b 1

call "_make_srec.bat"
if errorlevel 1 goto :build_failed

call "_apply_srec.bat"
if errorlevel 1 goto :build_failed

call "_fixcrc.bat"
if errorlevel 1 goto :build_failed

echo.
echo Build completed: OUTPUT\MADS ^(Mengze^).bin
popd
endlocal
exit /b 0

:build_failed
set "MADS_BUILD_STATUS=%ERRORLEVEL%"
echo.
echo Build failed with error %MADS_BUILD_STATUS%.
popd
endlocal & exit /b %MADS_BUILD_STATUS%
