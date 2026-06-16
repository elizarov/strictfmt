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
`strictfmt_tests` CMake target in `build\windows\cmake\`.

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

By default the build uses the vendored static tree-sitter runtime under
`vendor\tree-sitter\tree-sitter\`. Package maintainers can set
`STRICTFMT_USE_SYSTEM_TREE_SITTER=ON` to require an installed tree-sitter runtime
provided by either `unofficial-tree-sitter` CMake config package or the
`tree-sitter` pkg-config package.

Windows wrapper compatibility coverage is skipped unless `STRICTFMT_FORMAT_CMD`
points to the wrapper command file under test.

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

By default the build uses the vendored static tree-sitter runtime under
`vendor/tree-sitter/tree-sitter/`. Package maintainers can set
`STRICTFMT_USE_SYSTEM_TREE_SITTER=ON` to require an installed tree-sitter runtime
provided by either `unofficial-tree-sitter` CMake config package or the
`tree-sitter` pkg-config package.

## Tree-Sitter Grammar Regeneration

Normal builds compile the vendored generated parser sources directly. After
editing `vendor/tree-sitter/tree-sitter-cpp/grammar.js`, regenerate the generated
grammar outputs from the repository root:

```sh
python3 tools/regenerate_tree_sitter_grammar.py
```

The regeneration tool supports Windows x64, macOS arm64/x64, and Linux
arm64/x64 hosts. When `--tree-sitter-cli <path>` is omitted, it downloads the
pinned tree-sitter CLI for the current host under `build/`, verifies the pinned
SHA-512, and uses it to update generated files under
`vendor/tree-sitter/tree-sitter-cpp/src/`.

Pass `--tree-sitter-cli <path>` to use an existing tree-sitter CLI executable
instead of the pinned download.
