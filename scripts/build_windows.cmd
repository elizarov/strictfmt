@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "REPO_ROOT=%%~fI"
set "BUILD_ROOT=%REPO_ROOT%\build_windows"
set "CMAKE_BUILD_DIR=%BUILD_ROOT%\cmake"

where cl.exe >nul 2>nul
if errorlevel 1 (
    echo error: cl.exe was not found in PATH.
    echo Run this script from an x64 Visual Studio build environment.
    exit /b 1
)

where cmake.exe >nul 2>nul
if errorlevel 1 (
    echo error: cmake.exe was not found in PATH.
    exit /b 1
)

if not exist "%BUILD_ROOT%" mkdir "%BUILD_ROOT%"

cmake.exe -S "%REPO_ROOT%" -B "%CMAKE_BUILD_DIR%" -G "NMake Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="%BUILD_ROOT%" ^
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="%BUILD_ROOT%\lib" ^
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="%BUILD_ROOT%\lib" ^
    -DCMAKE_PDB_OUTPUT_DIRECTORY="%BUILD_ROOT%\pdb" ^
    -DCMAKE_COMPILE_PDB_OUTPUT_DIRECTORY="%BUILD_ROOT%\pdb"
if errorlevel 1 exit /b %errorlevel%

cmake.exe --build "%CMAKE_BUILD_DIR%" --target strictfmt
if errorlevel 1 exit /b %errorlevel%

if not exist "%BUILD_ROOT%\strictfmt.exe" (
    echo error: expected executable was not produced: "%BUILD_ROOT%\strictfmt.exe"
    exit /b 1
)

echo Built "%BUILD_ROOT%\strictfmt.exe"
exit /b 0
