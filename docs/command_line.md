# Command Line

This document specifies the command-line parameters supported by the `strictfmt`
executable.

## Usage

```text
strictfmt [options] [file...]
strictfmt --stdin [options]
strictfmt --dump-syntax-tree <file> [--style <config-file>]
strictfmt --dump-break-tree <file> [--style <config-file>]
strictfmt --stdin (--dump-syntax-tree | --dump-break-tree) [--style <config-file>]
strictfmt --version
```

Options that take values expect the value as the following argument, for example
`--style path/to/.cpp-format`.

Invoking `strictfmt` without inputs prints usage help to stdout and exits with
code `0`.

## Inputs

- `file...` formats the listed source files. In default mode, formatted source is written to stdout. Multiple file outputs are concatenated in input order with no extra separator.
- `--stdin` reads one source file from stdin. In default mode it writes formatted text to stdout; with `--diff` or a dump mode it writes that mode's output to stdout. It cannot be combined with direct file arguments, `--files`, `-r`, or `--recursive`. It is also incompatible with `-i`.
- `--files <path>` reads input file paths from a newline-delimited file list. Each list line is trimmed, and blank lines are ignored. The listed files are appended to the explicit input list in list order.
- `-r <path>` and `--recursive <path>` recursively discover supported source files under a directory. The root must exist. Recursive input can be combined with direct file arguments and `--files`.

Recursive discovery includes files with these case-insensitive extensions:
`.c`, `.cc`, `.cpp`, `.cxx`, `.c++`, `.h`, `.hh`, `.hpp`, `.hxx`, `.h++`,
`.ipp`, `.inl`, and `.tpp`.

Direct file arguments and `--files` entries keep their specified order.
Recursively discovered files are sorted by normalized path and appended after
the explicit input list.

File inputs are checked against `.cpp-format-ignore`; ignored files are skipped.
Recursive discovery also skips ignored directories. The ignore-file syntax is
specified in [config.md](config.md).

## Modes

- Default mode formats input and writes formatted source to stdout. For file inputs and `--stdin`, the final summary is written to stderr so stdout contains only formatted source.
- `-i` rewrites files in place. It requires at least one file input from `file...`, `--files`, `-r`, or `--recursive`. It is incompatible with `--stdin`, `--dry-run`, and `--diff`.
- `-n` and `--dry-run` check formatting without writing formatted source or modifying files. The command exits with code `1` when formatting changes are needed. It is incompatible with `--diff`.
- `--diff` writes a unified diff between the source and formatted text to stdout without modifying files. It uses three context lines, emits changed files in input order, and uses paths relative to the current directory when possible. Its exit codes match `--dry-run`: `1` when formatting changes are needed and `0` when no changes are needed. It is incompatible with `-i` and `--dry-run`.
- `--dump-syntax-tree <file>` or `--stdin --dump-syntax-tree` prints the normalized syntax tree used by the formatter.
- `--dump-break-tree <file>` or `--stdin --dump-break-tree` prints each formatted segment's break-decision tree, including raw depth, surcharge, discount, effective cost, and the selected layout at each decision node.

Dump modes help inspect parsing and layout decisions. A syntax-tree dump includes `Error` and `Missing` nodes when parsing fails; either mode reports the failure to stderr and exits with code `1`. Dump modes do not format, check, rewrite, or honor ignore files. They are mutually exclusive and incompatible with formatting inputs, `-i`, `--dry-run`, `--diff`, `--concurrency`, and `--validate`.

## Configuration

- `--style <config-file>` uses the specified formatter configuration file for every input. The path is resolved to an absolute path. The special values `file` and `file:<path>` are rejected; pass the formatter configuration path directly instead.
- When `--style` is omitted, file inputs and file dump modes search upward from the source file for `.cpp-format`; `--stdin`, including stdin dump modes, searches upward from the current working directory.

Formatter configuration syntax, inheritance, and `.cpp-format-ignore` behavior
are specified in [config.md](config.md).

## Execution Options

- `--validate` enables slower output validation in any formatting mode: reparse the formatted text and format it again to check idempotence. A failed check reports an error and exits with code `1`; the affected output is neither emitted nor written. Without this option, formatting performs one pass. Input parse errors always fail. This follows the [no-silent-failure constraint](architecture.md#no-silent-failure).
- `--concurrency <n>` limits worker threads for file formatting. The value must be a positive integer. When omitted, `strictfmt` uses hardware concurrency, falling back to `4` workers when the platform does not report a value. The effective worker count is capped by the number of files.
- `-v` and `--verbose` print one line before and after each file is formatted. Each line includes the file's index in the input list and its absolute path; the completion line also includes that file's elapsed formatting time. Verbose progress uses the summary stream and replaces the terminal's in-place aggregate progress line. With multiple workers, lines reflect actual worker start and completion order. Final summaries are printed regardless of this flag. The summary stream is stderr in default and diff modes, and stdout otherwise.
- `--version` prints `strictfmt <version>` to stdout and exits with code `0` without loading configuration or formatting inputs. Release executables print the release tag version without its leading `v`.
- `-h` and `--help` print usage help to stdout and exit with code `0`.

For file inputs, when the summary stream is a terminal, `strictfmt` updates an
in-place progress line with completed file count and elapsed time. Final
summaries report completed files, lines of code, elapsed time, ignored files,
files needing formatting, and formatting errors when applicable.

## Unknown arguments

Unknown arguments that begin with `-` are rejected. A file path whose name starts
with `-` must be passed in a spelling that does not begin with `-`, such as
`./-name.cpp`.

## Exit Codes

- `0` means formatting, checking, help, or no-input usage completed successfully.
- `1` means formatting failed for source-level reasons, including input parse errors, output validation errors, read or write failures, or dry-run/diff inputs that require formatting changes.
- `2` means command-line usage, input discovery, file-list reading, configuration loading, or configuration parsing failed.
