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

The build directory is ignored by Git.

## Unix

The Unix build uses CMake with the Clang C and C++ toolchain. It supports Linux
and macOS, and expects `cmake`, `make`, and a Clang toolchain to be installed
and available on `PATH`.

From the repository root:

```sh
scripts/build_unix.sh
```

The script uses `CC` and `CXX` from the shell when they are already configured.
Otherwise it selects `clang`/`clang++`, or a matching versioned pair such as
`clang-17`/`clang++-17`. It does not install packages or configure the toolchain
environment.

The script writes all generated files under an OS-specific build directory:

- `build_linux/strictfmt` on Linux.
- `build_macos/strictfmt` on macOS.
- `build_<osname>/cmake/` contains the CMake build tree.
- `build_<osname>/lib/` contains static libraries and other library outputs.

The build directories are ignored by Git.
