# Architecture

## Overview

`strictfmt` formats one source text at a time. Source text and formatter configuration enter `FormatSourceText`, tree-sitter parses the text, `BuildFormatModel` converts the syntax tree into the formatter-owned format model, the pretty printer turns that model into print tokens, `BuildFormatBreakModel` creates break models for formatted segments, the break optimizer chooses compact or split layouts, and the pretty printer emits formatted source with the original line ending style preserved.

## Module Ownership

- `src/tools/strictfmt_cli.h|cpp` own the embeddable `RunStrictfmtCli(argc, argv)` entry point.
- `src/strictfmt_main.cpp` owns the standalone executable `main` entry point.
- `src/tools/format.h|cpp` own source text formatting, line ending preservation, and second-pass verification.
- `src/tools/format_cli.cpp` owns the end-user formatter command orchestration: input collection, configuration lookup, ignore filtering, parallel file formatting, output routing, summaries, and exit codes.
- `src/tools/format_model_dump.h|cpp` own the ad hoc format model dump command.
- `src/tools/impl/format_args.h|cpp` own command-line option parsing and usage text.
- `src/tools/impl/format_break_model.h|cpp` own the break model data structures and shared break model predicates.
- `src/tools/impl/format_break_model_builder.h|cpp` own conversion from print tokens to break models.
- `src/tools/impl/format_break_model_inline_helpers.h` owns small inline accessors for optional break model tokens.
- `src/tools/impl/format_break_solver.h|cpp` own the break optimizer, see [break_solver.md] for details.
- `src/tools/impl/format_config.h|cpp` own formatter configuration, ignore files, upward discovery, inheritance, parsing, and caching.
- `src/tools/impl/format_include_sort.h|cpp` own include run normalization, grouping, main-include detection, and sorting.
- `src/tools/impl/format_model.h|cpp` own format model node kinds, token classes, symbol mappings, and syntax metadata.
- `src/tools/impl/format_model_builder.h|cpp` own conversion from tree-sitter nodes to the normalized format model, including syntax normalization and preprocessor placement checks.
- `src/tools/impl/format_model_parse.h|cpp` own tree-sitter parser setup, macro-category callbacks, and parse-to-format-model wiring.
- `src/tools/impl/format_pretty_printer.h|cpp` own print token production, mandatory line breaks, break model solving integration, and formatted source emission.
- `src/tools/impl/format_spacing.h|cpp` own print token text, width, classification, and spacing rules.
- `src/tools/impl/tools_common.h|cpp` own shared tool helpers for paths, recursive discovery, file lists, source lines, include text, counts, and lightweight string operations.
- `src/tools/impl/tools_parallel.h|cpp` own tool concurrency parsing, default worker selection, and indexed parallel execution.
- `src/tools/impl/tools_progress.h|cpp` own elapsed-time formatting and terminal progress rendering.
- `src/util/file_path.h|cpp` own portable path wrappers and binary file I/O.
- `src/util/strings.h|cpp` own general string normalization, splitting, matching, joining, and sorting helpers.

## Build Ownership

- `strictfmt_tree_sitter_runtime` owns the vendored static tree-sitter runtime when `STRICTFMT_USE_SYSTEM_TREE_SITTER` is disabled.
- `strictfmt_tree_sitter_cpp_grammar` owns the vendored generated C++ grammar and scanner.
- `strictfmt_util` owns utility modules shared by CLI and formatter code.
- `strictfmt_core` owns the formatter core pipeline from source text through formatted source.
- `strictfmt_cli` owns command-line and embedding support on top of `strictfmt_core`.
- `strictfmt` owns the standalone executable when `STRICTFMT_BUILD_STANDALONE` is enabled.
- `strictfmt_tests` owns the custom test runner target backed by `tests/format/format_test.py` when Python is available.
- `StrictfmtFormatTests` owns the CTest entry for the formatter test suite when Python is available.
