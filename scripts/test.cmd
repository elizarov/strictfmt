@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "REPO_ROOT=%%~fI"
set "BUILD_TEMP_DIR=%REPO_ROOT%\build\windows"
set "CMAKE_BUILD_DIR=%BUILD_TEMP_DIR%\cmake"

call "%SCRIPT_DIR%build.cmd"
if errorlevel 1 exit /b %errorlevel%

cmake.exe --build "%CMAKE_BUILD_DIR%" --target strictfmt_tests
if errorlevel 1 exit /b %errorlevel%

exit /b 0
