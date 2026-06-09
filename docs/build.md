# Build

## Windows

The Windows build uses CMake with the Visual Studio `cl.exe` toolchain. The build
script expects the compiler environment to already be configured.

Open an x64 Visual Studio build environment, such as an x64 Native Tools Command
Prompt, or call the matching `vcvars64.bat` from your Visual Studio installation.
Then run the build script from the repository root:

```bat
scripts\build.cmd
```

The build script intentionally does not configure Visual Studio itself, so it can
fail fast when run from an unconfigured shell.

The script writes the final executable directly under `build\` and all generated
Windows state under `build\windows\`:

- `build\strictfmt.exe` is the built executable.
- `build\windows\cmake\` contains the CMake build tree.
- `build\windows\lib\` contains static libraries and other archive outputs.
- `build\windows\pdb\` contains generated debug symbol files when the compiler
  produces them.
- `build\windows\tests\` contains formatter test temporary files.

The build directory is ignored by Git.

Embedding hosts can set `STRICTFMT_BUILD_STANDALONE=OFF` to import the strictfmt
libraries without creating the standalone formatter target. Formatter tests can
still be reused by setting `STRICTFMT_FORMAT_TEST_EXE` to the host executable and
`STRICTFMT_FORMAT_TEST_EXE_ARGS` to fixed arguments that precede the normal
strictfmt test arguments.

## Unix

The Unix build uses CMake with the Clang C and C++ toolchain. It supports Linux
and macOS, and expects `cmake`, `make`, and a Clang toolchain to be installed
and available on `PATH`.

From the repository root:

```sh
scripts/build.sh
```

The script uses `CC` and `CXX` from the shell when they are already configured.
Otherwise it selects `clang`/`clang++`, or a matching versioned pair such as
`clang-17`/`clang++-17`. It does not install packages or configure the toolchain
environment.

The script writes the final executable directly under `build/` and all generated
Unix state under an OS-specific subdirectory:

- `build/strictfmt` is the built executable.
- `build/linux/cmake/` or `build/macos/cmake/` contains the CMake build tree.
- `build/linux/lib/` or `build/macos/lib/` contains static libraries and other
  library outputs.
- `build/linux/tests/` or `build/macos/tests/` contains formatter test temporary
  files.

The build directory is ignored by Git. Windows and Linux builds can coexist in
the same checkout because their generated state lives under separate
`build/windows/` and `build/linux/` subdirectories.
