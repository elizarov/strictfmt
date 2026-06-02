# Build

## Windows

The Windows build uses CMake with the Visual Studio `cl.exe` toolchain. The build
script expects the compiler environment to already be configured.

Open an x64 Visual Studio build environment, such as an x64 Native Tools Command
Prompt, or call the matching `vcvars64.bat` from your Visual Studio installation.
Then run the build script from the repository root:

```bat
scripts\build_windows.cmd
```

The build script intentionally does not configure Visual Studio itself, so it can
fail fast when run from an unconfigured shell.

The script writes all generated files under `build_windows\`:

- `build_windows\strictfmt.exe` is the built executable.
- `build_windows\cmake\` contains the CMake build tree.
- `build_windows\lib\` contains static libraries and other archive outputs.
- `build_windows\pdb\` contains generated debug symbol files when the compiler
  produces them.
