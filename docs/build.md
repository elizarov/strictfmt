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

To build and run the formatter tests:

```bat
scripts\test.cmd
```

The test script invokes `scripts\build.cmd` first, then builds the
`strictfmt_tests` CMake target.

The script writes the final executable directly under `build\` as `build\strictfmt.exe` and all generated
Windows state under `build\windows\`.

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

To build and run the formatter tests:

```sh
scripts/test.sh
```

The test script invokes `scripts/build.sh` first, then builds the
`strictfmt_tests` CMake target in the platform CMake build tree.

The script uses `CC` and `CXX` from the shell when they are already configured.
Otherwise it selects `clang`/`clang++`, or a matching versioned pair such as
`clang-17`/`clang++-17`. It does not install packages or configure the toolchain
environment.

The script writes the final executable directly under `build/` as `build/strictfmt` and all generated
Unix state under an OS-specific subdirectory (either `build/linux/` or `build/macos/`).

Windows and Linux builds can coexist in the same checkout because their generated state lives under separate subdirectories.

## Tree-sitter

The build uses the vendored static tree-sitter runtime under
`vendor/tree-sitter/tree-sitter/`. Runtime and generated-parser compatibility is
governed by the hard [upstream tree-sitter runtime constraint](architecture.md#upstream-tree-sitter-runtime).

The vendored C++ grammar also compiles a custom external scanner; see [scanner.md](scanner.md).
