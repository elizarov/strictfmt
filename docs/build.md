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

## Formatting Project Sources

Before committing changes, build the current formatter and format all project
sources under `src/` in place. On Unix, run:

```sh
scripts/format_src.sh
```

On Windows, run from an x64 Visual Studio build environment:

```bat
scripts\format_src.cmd
```

Each script runs the platform build script first, then uses the newly built
executable with the repository configuration.

## Tree-sitter

The build uses the vendored static tree-sitter runtime under
`vendor/tree-sitter/tree-sitter/`. Runtime and generated-parser compatibility is
governed by the hard [upstream tree-sitter runtime constraint](architecture.md#upstream-tree-sitter-runtime).

The vendored C++ grammar also compiles a custom external scanner; see [scanner.md](scanner.md).
Grammar regeneration and the formatter test suite validate the closed set of lexical terminals and external tokens,
enforcing the composite-syntax rule owned by [architecture.md](architecture.md#structural-genericity). Run the validation
alone with `python3 tools/regenerate_tree_sitter_grammar.py --validate-structure-only`.

## Versions

CMake embeds a version string in the command-line executable. `strictfmt --version`
prints that value as specified in [command_line.md](command_line.md). A build may set
`STRICTFMT_VERSION` as a CMake cache variable, or as an environment variable when
using `scripts/build.sh|cmd`. Without an override, CMake derives a development
version from `git describe`; an exact `v<version>` tag becomes `<version>`.

## GitHub Workflows

`.github/workflows/build.yml` runs `scripts/test.sh` on a Linux x86_64 runner for
every pushed branch or tag. Checkout includes the top-level external-project
submodules but not submodules nested inside those fixtures, so this is the same
complete validation suite used locally.

`.github/workflows/release.yml` runs for `v<major>.<minor>.<patch>` tag pushes. It
first runs the complete tests on Linux x86_64, then builds and verifies versioned
executables on Linux x86_64, Windows x86_64, and macOS arm64 GitHub-hosted
runners. The executables are packaged as `.tar.gz` or `.zip` archives, collected
with a `SHA256SUMS` file, and published in a GitHub release. A manual workflow run
performs the tests and all platform builds without publishing a release; use it
to validate workflow changes before tagging.

## Creating a Release

From a clean `main` worktree, run:

```sh
scripts/release.sh 1.2.3
```

The script accepts a numeric three-component version, verifies that the tag does
not already exist, pushes `main`, creates the annotated `v1.2.3` tag, and pushes
that tag to the branch's configured remote. The tag push starts the release
workflow. If the tag push fails, the script removes the unpushed local tag so the
release can be retried after correcting the problem.
