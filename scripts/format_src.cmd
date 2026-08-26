@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "REPO_ROOT=%%~fI"

call "%SCRIPT_DIR%build.cmd"
if errorlevel 1 exit /b %errorlevel%

"%REPO_ROOT%\build\strictfmt.exe" -i -r "%REPO_ROOT%\src"
if errorlevel 1 exit /b %errorlevel%

exit /b 0
