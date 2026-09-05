# Architecture

## Overview

`strictfmt` formats one source text at a time. Source text and formatter configuration enter `FormatSourceText`, tree-sitter parses the text, `BuildFormatModel` converts the syntax tree into the formatter-owned format model, the pretty printer turns that model into print tokens, `BuildFormatBreakModel` creates break models for formatted segments, the break optimizer chooses compact or split layouts, and the pretty printer emits formatted source with the original line ending style preserved.

Print-token construction materializes canonical known-token text and immutable syntax traits used by later compact checks, spacing, and break-model construction. Ancestry traits are propagated during the same syntax traversal that emits tokens. Syntax normalization materializes targeted immutable descendant facts on their owning nodes when later formatting would otherwise repeat the recursive query. Adjacent-source spacing is cached once and reused only when consecutive source indices prove that the same tokens remain adjacent in a buffered segment; spacing at formatter-controlled segment boundaries is recomputed.

## Module Ownership

- `src/strictfmt_main.cpp` owns the standalone executable `main` entry point.
- `src/format/strictfmt_cli.h|cpp` own the embeddable `RunStrictfmtCli(argc, argv)` entry point.
- `src/format/format.h|cpp` own source text formatting, line ending preservation, and second-pass verification.
- `src/format/format_cli.cpp` owns the end-user formatter command orchestration: input collection, configuration lookup, ignore filtering, parallel file formatting, output routing, summaries, and exit codes.
- `src/format/impl/format_args.h|cpp` own command-line option parsing and usage text.
- `src/format/impl/format_diff.h|cpp` own greedy line synchronization and unified-diff emission for `--diff`.
- `src/format/impl/format_break_cost.h|cpp` own structural prefix-depth adjustments and final break-cost subtree discounts, including the no-discount traversal shortcut.
- `src/format/impl/format_break_emitter.h|cpp` own recursive solved-layout emission through a physical-output adapter and report deferred list/chain/block indentation to the printer.
- `src/format/impl/format_break_model.h|cpp` own the break model data structures and shared break model predicates.
- `src/format/impl/format_break_model_builder.h|cpp` own conversion from print tokens to break models.
- `src/format/impl/format_break_model_dump.h|cpp` own serialization of break-decision trees.
- `src/format/impl/format_break_model_inline_helpers.h` owns small inline accessors for optional break model tokens.
- `src/format/impl/format_compact_layout.h|cpp` own exact compact physical-line measurement and its immutable-model cache.
- `src/format/impl/format_chain_continuation.h|cpp` own uniform chain constraints and render-base propagation across mandatory block boundaries.
- `src/format/impl/format_list_continuation.h|cpp` own virtual list-delimiter planning, selected continuation indentation, and list boundary state across blocks and conditional preprocessor regions.
- `src/format/impl/format_syntax_helpers.h` owns shared direct-child lexical queries used by structural printing and continuation planning.
- `src/format/impl/format_break_solver.h|cpp` own the break optimizer; see [break_solver.md].
- `src/format/impl/format_choice_history.h|cpp` own immutable choice-history storage, lookup, concatenation, and materialization.
- `src/format/impl/format_candidates.h|cpp` own layout-candidate value storage, overflow accounting, cost comparison, continuation-state equivalence, and dominance pruning.
- `src/format/impl/format_break_solution.h` owns the materialized layout data shared by solving, emission, diagnostics, and declaration analysis.
- `src/format/impl/format_value_profile.h|cpp` own the sparse value profile shared by break optimization costs.
- `src/format/impl/format_config.h|cpp` own formatter configuration, ignore files, upward discovery, inheritance, parsing, and caching.
- `src/format/impl/format_declaration_layout.h|cpp` own declaration-value advance analysis, declaration grouping, and exact reuse of pre-solved layouts.
- `src/format/impl/format_model_text_stats.h` owns optional model-to-text phase timings.
- `src/format/impl/format_include_sort.h|cpp` own include run normalization, grouping, main-include detection, and sorting.
- `src/format/impl/format_model.h|cpp` own format model storage/construction, node kinds, `SyntaxNodeClass`, symbol mappings, and syntax metadata; category checks must use `SyntaxNodeClass` helpers, not duplicated `SyntaxNodeKind` lists, with exact kind comparisons reserved for one concrete syntax rule.
- `src/format/impl/format_model_builder.h|cpp` own conversion from tree-sitter nodes to the format model, source trivia, declarator-field preservation, and opening include-run grouping.
- `src/format/impl/format_model_normalize.h|cpp` own bottom-up syntax normalization and materialized semantic facts on formatter-owned nodes.
- `src/format/impl/format_preprocessor_validation.h|cpp` own preprocessor placement validation.
- `src/format/impl/format_model_dump.h|cpp` own syntax-tree and break-tree dump command orchestration.
- `src/format/impl/format_model_parse.h|cpp` own tree-sitter parser setup, macro-category callbacks, and parse-to-format-model wiring.
- `vendor/tree-sitter/tree-sitter-cpp/src/scanner.c` owns custom tree-sitter external tokens, including runtime-configured macro identifiers, raw string delimiter state, and preprocessor directive newline ownership; see [scanner.md](scanner.md).
- `src/format/impl/format_print_token.h` owns print-token data and borrowed-source metadata.
- `src/format/impl/format_print_token_builder.h|cpp` own normalized syntax traversal through a private inherited context, centralized print-token construction, ancestry facts, comment continuations, and initial adjacent-source spacing.
- `src/format/impl/format_pretty_printer.h|cpp` own mandatory line breaks, segment build/solve/emit orchestration, structural brace/statement indentation, and syntax-based comment placement.
- `src/format/impl/format_output.h|cpp` own physical text, columns, pending line indentation, macro continuation suffixes, and deferred comment alignment through a syntax-independent output buffer.
- `src/format/impl/format_preprocessor_text.h|cpp` own directive text canonicalization, preserved payload indentation, and conditional payload terminal-comma normalization.
- `src/format/impl/format_raw_macro.h|cpp` own raw macro replacement whitespace normalization and the raw preprocessor line-preservation helpers used by the pretty printer.
- `src/format/impl/format_string_literals.h|cpp` own safe adjacent-string spelling joins and escaped-newline split requirements.
- `src/format/impl/format_spacing.h|cpp` own print token text/width accessors, classification, and spacing rules.
- `src/tools/tools_common.h|cpp` own shared tool helpers for paths, recursive discovery, file lists, source lines, include text, counts, and lightweight string operations.
- `src/tools/tools_parallel.h|cpp` own tool concurrency parsing, default worker selection, and indexed parallel execution.
- `src/tools/tools_progress.h|cpp` own elapsed-time formatting and terminal progress rendering.
- `src/util/file_path.h|cpp` own portable path wrappers and binary file I/O.
- `src/util/strings.h|cpp` own general string normalization, splitting, matching, joining, and sorting helpers.
- `src/util/utf8.h|cpp` own UTF-8 character counting shared by layout estimation and emission.
- `vendor/unicode/` owns the pinned grapheme property data and conformance fixture; see its [README](../vendor/unicode/README.md).

## Build Ownership

- `strictfmt_tree_sitter_runtime` owns the vendored static tree-sitter runtime, subject to the upstream-runtime constraint below.
- `strictfmt_tree_sitter_cpp_grammar` owns the vendored generated C++ grammar and custom scanner; see [scanner.md](scanner.md).
- `strictfmt_util` owns utility modules shared by CLI and formatter code.
- `strictfmt_core` owns the formatter core pipeline from source text through formatted source.
- `strictfmt_cli` owns command-line and embedding support on top of `strictfmt_core`.
- `strictfmt` owns the standalone executable when `STRICTFMT_BUILD_STANDALONE` is enabled.
- `strictfmt_tests` owns the custom test runner target backed by `tests/format/format_test.py` when Python is available.
- `StrictfmtFormatTests` owns the CTest entry for the formatter test suite when Python is available.
- `strictfmt_utf8_tests` and `StrictfmtUtf8Tests` own the Unicode utility test executable and its CTest entry.
- `strictfmt_layout_tests` and `StrictfmtLayoutTests` own the internal layout-contract test executable and its CTest entry.

## Upstream Tree-Sitter Runtime

Using an unmodified upstream tree-sitter C runtime is a hard architectural constraint. The runtime source, public and parser-facing APIs, ABI, parse-table representation, and table readers vendored under `vendor/tree-sitter/tree-sitter/` must match the pinned upstream release; strictfmt-specific patches to them are not permitted. Parser customization belongs in the C++ grammar, the custom external scanner, or strictfmt's parser/model integration.

The generated C++ parser must fit every limit imposed by the pinned upstream generator and runtime. In particular, parser state ids and parse-action indexes must remain representable by the upstream 16-bit table ABI and therefore must not exceed 65,535. A grammar change that exceeds an upstream limit must be reduced, redesigned, or rejected. Widening runtime or generated-table types, post-processing generated files to change their ABI, or maintaining a private runtime fork is not an acceptable solution.

Generated-source compaction may change only the C spelling of values emitted by the stock generator. It must preserve every table value, table dimension, and runtime-facing structure, keep the parser in one source file, and require no runtime or parser-header change. Re-encoding or splitting parse tables is not source compaction and is not permitted by this exception.

## Structural Genericity

Grammar must model C++ constructs generically, following the shape of the C++ language rather than the source samples. Every piece of the grammar must work and parse in a recursive way. A grammar rule must not encode a shallow convenience shape that only works for the current nesting level or current fixture; if adding one more nesting level would require another special case, the rule is not generic enough. If a construct can appear where another C++ construct can appear, the grammar must compose through the same recursive nonterminal.

Resolving syntactic ambiguity in the grammar is a hard architectural constraint. Grammar conflicts and precedence must select the intended complete recursive production. Later formatter stages must consume that selected syntax category as-is; the format model, spacing logic, and break model must not reinterpret expression tokens as declarations, templates, or other competing syntax.

Project-specific tokens are used only for intentionally non-C++ macro fragments or scanner-owned lexical features documented in [scanner.md](scanner.md) and are taken from configuration, not hard-coded. Otherwise, structured grammar productions compose with existing C++ declarators, type names, expressions, and statements.

Composite syntax must remain recursive in both the tree-sitter tree and the formatter model. A grammar token or formatter leaf must not hide any composite source span. The replacement text of a macro explicitly configured under `RawMacroDefinitions` is the sole opaque-source exception. The only other leaves are ordinary lexical tokens.

The format model preserves grammar declarator-field roles through wrapper flattening, so declaration boundaries do not depend on the declarator's spelling or shape.

Formatter behavior follows the same principle: rules use shared structural or configured semantic categories and apply at every supported recursion depth. Source spelling, incidental parser wrappers, and golden-fixture shape must not create one-off formatting categories.
