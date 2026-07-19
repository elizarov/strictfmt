# Architecture

## Overview

`strictfmt` formats one source text at a time. Source text and formatter configuration enter `FormatSourceText`, tree-sitter parses the text, `BuildFormatModel` converts the syntax tree into the formatter-owned format model, the pretty printer turns that model into print tokens, `BuildFormatBreakModel` creates break models for formatted segments, the break optimizer chooses compact or split layouts, and the pretty printer emits formatted source with the original line ending style preserved.

## Module Ownership

- `src/strictfmt_main.cpp` owns the standalone executable `main` entry point.
- `src/format/strictfmt_cli.h|cpp` own the embeddable `RunStrictfmtCli(argc, argv)` entry point.
- `src/format/format.h|cpp` own source text formatting, line ending preservation, and second-pass verification.
- `src/format/format_cli.cpp` owns the end-user formatter command orchestration: input collection, configuration lookup, ignore filtering, parallel file formatting, output routing, summaries, and exit codes.
- `src/format/format_model_dump.h|cpp` own the debug format model dump used by `--dump`.
- `src/format/impl/format_args.h|cpp` own command-line option parsing and usage text.
- `src/format/impl/format_break_model.h|cpp` own the break model data structures and shared break model predicates.
- `src/format/impl/format_break_model_builder.h|cpp` own conversion from print tokens to break models.
- `src/format/impl/format_break_model_inline_helpers.h` owns small inline accessors for optional break model tokens.
- `src/format/impl/format_break_solver.h|cpp` own the break optimizer; see [break_solver.md].
- `src/format/impl/format_config.h|cpp` own formatter configuration, ignore files, upward discovery, inheritance, parsing, and caching.
- `src/format/impl/format_include_sort.h|cpp` own include run normalization, grouping, main-include detection, and sorting.
- `src/format/impl/format_model.h|cpp` own format model node kinds, `SyntaxNodeClass`, symbol mappings, and syntax metadata; category checks must use `SyntaxNodeClass` helpers, not duplicated `SyntaxNodeKind` lists, with exact kind comparisons reserved for one concrete syntax rule.
- `src/format/impl/format_model_builder.h|cpp` own conversion from tree-sitter nodes to the normalized format model, including syntax normalization and preprocessor placement checks.
- `src/format/impl/format_model_parse.h|cpp` own tree-sitter parser setup, macro-category callbacks, and parse-to-format-model wiring.
- `vendor/tree-sitter/tree-sitter-cpp/src/scanner.c` owns custom tree-sitter external tokens, including runtime-configured macro identifiers, raw string delimiter state, and preprocessor directive newline ownership; see [scanner.md](scanner.md).
- `src/format/impl/format_pretty_printer.h|cpp` own print token production, mandatory line breaks, break model solving integration, and formatted source emission.
- `src/format/impl/format_raw_macro.h|cpp` own raw macro replacement whitespace normalization and the raw preprocessor line-preservation helpers used by the pretty printer.
- `src/format/impl/format_spacing.h|cpp` own print token text, width, classification, and spacing rules.
- `src/tools/tools_common.h|cpp` own shared tool helpers for paths, recursive discovery, file lists, source lines, include text, counts, and lightweight string operations.
- `src/tools/tools_parallel.h|cpp` own tool concurrency parsing, default worker selection, and indexed parallel execution.
- `src/tools/tools_progress.h|cpp` own elapsed-time formatting and terminal progress rendering.
- `src/util/file_path.h|cpp` own portable path wrappers and binary file I/O.
- `src/util/strings.h|cpp` own general string normalization, splitting, matching, joining, and sorting helpers.

## Build Ownership

- `strictfmt_tree_sitter_runtime` owns the vendored static tree-sitter runtime, subject to the upstream-runtime constraint below.
- `strictfmt_tree_sitter_cpp_grammar` owns the vendored generated C++ grammar and custom scanner; see [scanner.md](scanner.md).
- `strictfmt_util` owns utility modules shared by CLI and formatter code.
- `strictfmt_core` owns the formatter core pipeline from source text through formatted source.
- `strictfmt_cli` owns command-line and embedding support on top of `strictfmt_core`.
- `strictfmt` owns the standalone executable when `STRICTFMT_BUILD_STANDALONE` is enabled.
- `strictfmt_tests` owns the custom test runner target backed by `tests/format/format_test.py` when Python is available.
- `StrictfmtFormatTests` owns the CTest entry for the formatter test suite when Python is available.

## Upstream Tree-Sitter Runtime

Using an unmodified upstream tree-sitter C runtime is a hard architectural constraint. The runtime source, public and parser-facing APIs, ABI, parse-table representation, and table readers vendored under `vendor/tree-sitter/tree-sitter/` must match the pinned upstream release; strictfmt-specific patches to them are not permitted. Parser customization belongs in the C++ grammar, the custom external scanner, or strictfmt's parser/model integration.

The generated C++ parser must fit every limit imposed by the pinned upstream generator and runtime. In particular, parser state ids and parse-action indexes must remain representable by the upstream 16-bit table ABI and therefore must not exceed 65,535. A grammar change that exceeds an upstream limit must be reduced, redesigned, or rejected. Widening runtime or generated-table types, post-processing generated files to change their ABI, or maintaining a private runtime fork is not an acceptable solution.

## Parser Genericity

Grammar must model C++ constructs generically, following the shape of the C++ language rather than the source samples. Every piece of the grammar must work and parse in a recursive way. A grammar rule must not encode a shallow convenience shape that only works for the current nesting level or current fixture; if adding one more nesting level would require another special case, the rule is not generic enough. If a construct can appear where another C++ construct can appear, the grammar must compose through the same recursive nonterminal.

Project-specific tokens are used only for intentionally non-C++ macro fragments or scanner-owned lexical features documented in [scanner.md](scanner.md) and are taken from configuration, not hard-coded. Otherwise, structured grammar productions compose with existing C++ declarators, type names, expressions, and statements.
