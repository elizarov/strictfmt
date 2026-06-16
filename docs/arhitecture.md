# Architecture

## Tooling Ownership

- `strictfmt` owns explicit stdin input passed with `--stdin`, direct file arguments, recursive roots passed with `-r <path>` or `--recursive <path>` for `.c`, `.cc`, `.cpp`, `.cxx`, `.c++`, `.h`, `.hh`, `.hpp`, `.hxx`, `.h++`, `.ipp`, `.inl`, and `.tpp` files, newline file lists passed with `--files <path>`, `-i`, `--dry-run`, `--style <path>`, `--concurrency <n>` handling, parsing, parse-error rejection, checking, fixing, ignore-file filtering, and stdout rendering. Invoking `strictfmt` without inputs prints command-line help instead of reading stdin. Interactive file sessions show completed file count and elapsed time while work is active, and final summaries report completed files, LOC, and elapsed time.
- `strictfmt::cli` exposes the same CLI implementation through `RunStrictfmtCli(argc, argv)` for embedding in host tools. Wrapper scripts own their own mode selection and changed or staged file-list preparation.
- `src\tools\format_cli.cpp` owns formatter command orchestration, `src\tools\format.cpp` owns source-text formatting, and the internal `src\tools\impl\format_*` formatter modules own parser setup, model definitions, tree-sitter model builder helpers, preprocessor model helpers, and pretty printing.
- `format_model_dump` owns ad hoc formatter model debugging. It takes one source file path and writes YAML to stdout with each `SyntaxNode` represented by `kind`, plus `text` only when node text is non-empty and `children` only when child nodes exist; text values up to 60 bytes are YAML-quoted, and longer text values are emitted as their byte length.
- `.cpp-format`, `tests/format/.cpp-format`, `tests/format/.cpp-format-userver`, and `.cpp-format-ignore` own the formatter configuration data described in [Formatter Configuration](#formatter-configuration).
- `vendor\tree-sitter\` owns vendored tree-sitter grammar inputs and generated parser sources.
