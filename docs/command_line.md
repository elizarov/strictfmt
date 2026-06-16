# Command Line

This document specifies the command-line parameters supported by the `strictfmt`
executable.

## Usage

```text
strictfmt [options] [file...]
strictfmt --stdin [options]
strictfmt --dump <file> [--style <config-file>]
```

Options that take values expect the value as the following argument, for example
`--style path/to/.cpp-format`.

Invoking `strictfmt` without inputs prints usage help to stdout and exits with
code `0`.

## Inputs

- `file...` formats the listed source files. In default mode, formatted source is written to stdout. Multiple file outputs are concatenated in input order with no extra separator.
- `--stdin` reads one source file from stdin. It cannot be combined with direct file arguments, `--files`, `-r`, or `--recursive`. It is also incompatible with `-i`.
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
- `--dump <file>` prints the parsed internal format model for one source file to stdout. This debugging mode helps inspect parsing, syntax normalization, macro classification, and the internal model used by the formatter. It does not format, check, rewrite, or honor ignore files. It is incompatible with formatting inputs, `--stdin`, `-i`, `--dry-run`, and `--concurrency`.
- `-i` rewrites files in place. It requires at least one file input from `file...`, `--files`, `-r`, or `--recursive`. It is incompatible with `--stdin` and `--dry-run`.
- `-n` and `--dry-run` check formatting without writing formatted source or modifying files. The command exits with code `1` when formatting changes are needed.

## Configuration

- `--style <config-file>` uses the specified formatter configuration file for every input. The path is resolved to an absolute path. The special values `file` and `file:<path>` are rejected; pass the formatter configuration path directly instead.
- When `--style` is omitted, file inputs and `--dump` search upward from the source file for `.cpp-format`; `--stdin` searches upward from the current working directory.

Formatter configuration syntax, inheritance, and `.cpp-format-ignore` behavior
are specified in [config.md](config.md).

## Execution Options

- `--concurrency <n>` limits worker threads for file formatting. The value must be a positive integer. When omitted, `strictfmt` uses hardware concurrency, falling back to `4` workers when the platform does not report a value. The effective worker count is capped by the number of files.
- `-v` and `--verbose` are accepted and reserved for verbose progress output. Final summaries are printed regardless of this flag.
- `-h` and `--help` print usage help to stdout and exit with code `0`.

For file inputs, when the summary stream is a terminal, `strictfmt` updates an
in-place progress line with completed file count and elapsed time. Final
summaries report completed files, lines of code, elapsed time, ignored files,
files needing formatting, and parse errors when applicable.

## Unknown arguments

Unknown arguments that begin with `-` are rejected. A file path whose name starts
with `-` must be passed in a spelling that does not begin with `-`, such as
`./-name.cpp`.

## Exit Codes

- `0` means formatting, checking, help, or no-input usage completed successfully.
- `1` means formatting failed for source-level reasons, including parse errors, read or write failures, or dry-run inputs that require formatting changes.
- `2` means command-line usage, input discovery, file-list reading, configuration loading, or configuration parsing failed.
