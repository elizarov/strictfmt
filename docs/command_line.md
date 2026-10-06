# Command Line

This document specifies the command-line parameters supported by the `strictfmt`
executable.

## Usage

```text
strictfmt [options] [ <file>... | -r <path> | --stdin | --files <path> ]
```

Options that take values expect the value as the following argument, for example
`--style path/to/.cpp-format`.

Invoking `strictfmt` without inputs prints usage help to stdout and exits with
code `0`.

## Inputs

- `<file>...` selects the listed source files for the chosen mode. In default mode, formatted source is written to stdout. Multiple file outputs are concatenated in input order with no extra separator.
- `-r <path>` and `--recursive <path>` recursively discover supported source files under a directory. The root must exist. Recursive input can be combined with direct file arguments and `--files`.
- `--stdin` reads one source file from stdin. In default mode it writes formatted text to stdout; with `--diff` or a dump mode it writes that mode's output to stdout. It cannot be combined with direct file arguments, `--files`, `-r`, or `--recursive`. It is also incompatible with `-i`.
- `--stdin-filename <path>` supplies the source filename for stdin configuration discovery, main-header sorting, diagnostics, and diffs. It requires `--stdin`; the named file need not exist and is never read or modified. Relative paths are resolved from the working directory.
- `--files <path>` reads input file paths from a newline-delimited file list. Each list line is trimmed, and blank lines are ignored. The listed files are appended to the explicit input list in list order.

Recursive discovery includes files with these case-insensitive extensions:
`.c`, `.cc`, `.cpp`, `.cxx`, `.c++`, `.h`, `.hh`, `.hpp`, `.hxx`, `.h++`,
`.ipp`, `.inl`, and `.tpp`.

Direct file arguments and `--files` entries keep their specified order.
Recursive files begin processing as they are discovered, while directory scanning
continues. Their output is sorted by normalized path and appended after the
explicit input list.

File inputs are checked against `.cpp-format-ignore`; ignored files are skipped.
Recursive discovery also skips ignored directories. The ignore-file syntax is
specified in [config.md](config.md).

## Modes

Formatting modes run naming checks enabled by the resolved `.cpp-format` before
formatting each file. With no enabled rules, the lint pass is skipped entirely.
Naming rules and suppressions are specified in [lint.md](lint.md).

- Default mode formats input and writes formatted source to stdout.
- `-i` rewrites files in place. It requires at least one file input from `<file>...`, `--files`, `-r`, or `--recursive`. It is incompatible with `--stdin`, `--dry-run`, and `--diff`.
- `-n` and `--dry-run` check formatting without writing formatted source or modifying files. The command exits with code `1` when formatting changes are needed. It is incompatible with `--diff`.
- `--diff` writes a unified diff between the source and formatted text to stdout without modifying files. It uses three context lines, emits changed files in input order, and uses paths relative to the current directory when possible. Its exit codes match `--dry-run`: `1` when formatting changes are needed and `0` when no changes are needed. It is incompatible with `-i` and `--dry-run`.
- `--lint-only` checks configured naming rules without formatting or producing source output. It accepts the same file and stdin inputs as formatting, but is incompatible with `-i`, `-n`/`--dry-run`, `--diff`, dump modes, `--validate`, and `--no-lint`.
- `--dump-syntax-tree <file>` or `--stdin --dump-syntax-tree` prints the normalized syntax tree used by the formatter.
- `--dump-break-tree <file>` or `--stdin --dump-break-tree` prints each formatted segment's break-decision tree, including raw depth, surcharge, discount, effective cost, and the selected layout at each decision node.

Dump modes help inspect parsing and layout decisions. A syntax-tree dump includes `Error` and `Missing` nodes when parsing fails; either mode reports the failure to stderr and exits with code `1`. Dump modes skip linting and do not format, check, rewrite, or honor ignore files. They are mutually exclusive and incompatible with formatting inputs, `-i`, `--dry-run`, `--diff`, `--lint-only`, `--concurrency`, and `--validate`.

## Configuration

- `--style <config-file>` uses the specified configuration file for every input. The path is resolved to an absolute path. The special values `file` and `file:<path>` are rejected; pass the formatter configuration path directly instead.
- When `--style` is omitted, file inputs and file dump modes search upward from the source file for `.cpp-format`; `--stdin`, including stdin dump modes, searches upward from `--stdin-filename` when provided, otherwise from the current working directory.

Configuration syntax, inheritance, and `.cpp-format-ignore` behavior
are specified in [config.md](config.md).

## Execution Options

- `--no-lint` disables naming checks in formatting modes. It is incompatible with `--lint-only`.
- `--validate` enables slower output validation in any formatting mode: reparse the formatted text and format it again to check idempotence. A failed check reports an error and exits with code `1`; the affected output is neither emitted nor written. Without this option, formatting performs one pass. Validation does not rerun lint; it checks output parsing and idempotence independently. Input parse errors always fail when parsing is performed. This follows the [no-silent-failure constraint](architecture.md#no-silent-failure).
- `--concurrency <n>` limits worker threads for file formatting or linting. The value must be a positive integer. When omitted, `strictfmt` uses hardware concurrency, falling back to `4` workers when the platform does not report a value. The effective worker count is capped by the number of files.
- `-v` and `--verbose` enable per-file progress; see [Console Progress and Summary](#console-progress-and-summary).
- `--version` prints `strictfmt <version>` to stdout and exits with code `0` without loading configuration or processing inputs. Release executables print the release tag version without its leading `v`.
- `-h` and `--help` print usage help to stdout and exit with code `0`.

## Console Progress and Summary

Progress and summaries go to stderr when stdout carries formatted source or
unified diffs, and to stdout otherwise.

For file inputs, a terminal shows an updating progress line with
completed/discovered file counts and elapsed time. A `+` and `(scanning)` mark
ongoing discovery; the total becomes fixed once scanning finishes. With
`--verbose`, start and completion lines replace this display, even outside a
terminal. Each includes the file's discovery index, current total, and absolute
path; completion also includes elapsed time. Lines follow worker execution order.

Final summaries report file counts, changed/total lines of code (LOC), elapsed
time, and any ignored files or errors. Checks report changes needed; formatting
reports changes made. Failed runs report processed files and LOC still needing
formatting. `--lint-only` reports checked files and lint diagnostics instead of
LOC. Counts use comma separators.

Total LOC counts input lines read. Changed LOC sums the larger of removed and
added line counts in each contiguous change block, using the same line matching
as `--diff`, including line endings. Replacements count once; insertions can make
changed LOC exceed total LOC.

Summaries do not require `--verbose`. Stdin formatting also prints a summary,
but has no progress display. Stdin lint-only and dump modes print neither.

## Diagnostics and Failures

Lint diagnostics go to stderr as
`path:line:column: error: message [identifier-naming:rule]`. Lines and byte columns
are one-based and refer to the input. Diagnostics follow the input-file order
described above, then source position within each file.

A lint violation prevents that file from formatting. Other input files are still
checked. In `-i` mode, no files are written if any input fails parsing, linting, or
validation. Exit codes distinguish source failures from configuration and usage
errors as listed below.

## Unknown arguments

Unknown arguments that begin with `-` are rejected. A file path whose name starts
with `-` must be passed in a spelling that does not begin with `-`, such as
`./-name.cpp`.

## Exit Codes

- `0` means formatting, linting, checking, help, or no-input usage completed successfully.
- `1` means processing failed for source-level reasons, including lint violations, input parse errors, output validation errors, read or write failures, or dry-run/diff inputs that require formatting changes.
- `2` means command-line usage, input discovery, file-list reading, configuration loading, or configuration parsing failed.
