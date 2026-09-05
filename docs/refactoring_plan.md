# Formatter Module Refactoring

Preserve formatter behavior while extracting internal modules with concise header
contracts. Complete each numbered step, format project sources, run the full
`scripts/test.sh` suite, inspect the diff, and commit before starting the next step.
`docs/architecture.md` remains the owner of the resulting module ownership map.

1. Print-token construction: extract syntax traversal and token metadata into
   `format_print_token_builder`, with shared token types in `format_print_token.h`.
2. Syntax normalization: extract formatter-owned node transformations into
   `format_model_normalize`, preserving bottom-up construction order.
3. Compact physical-line measurement: extract immutable break-model measurement
   and caching into `format_compact_layout`.
4. Declaration-group analysis: extract advance analysis, grouping decisions, and
   exact layout reuse into `format_declaration_layout`.
5. Solved break-model emission: extract recursive solution rendering into
   `format_break_emitter`, with explicit output and continuation contracts.
6. Preprocessor text formatting: extract directive and payload text normalization
   into `format_preprocessor_text`.
7. Adjacent-string analysis: extract safe spelling joins and split requirements
   into `format_string_literals`.
8. Break-depth and cost normalization: extract structural prefix adjustments and
   final subtree discounts into `format_break_cost`.

## Continued Architecture Review

Continue with the same format/test/review/commit cycle. The next candidates are:

9. Replace repeated print-token initializers and recursive argument lists with a
   private token factory and inherited syntax context.
10. Extract physical output buffering and comment alignment from the printer.
11. Extract continuation state across mandatory block boundaries in two commits:
    chain constraints/render bases, followed by deferred list layouts.
12. Isolate solver choice-history storage and materialization.
13. Isolate candidate continuation-state comparison and frontier maintenance.

Reassess the remaining builder, solver, and metadata modules after these steps.
Completion requires cohesive ownership, explicit lifetime/state contracts, and no
remaining substantial extraction that improves clarity without introducing broad
back-coupling. Keep performance-driven representation changes separate unless
measurements justify them; retain exact layout decisions throughout.

## Progress

- Baseline validation: all 78 tests passed.
- Step 1: complete; all 78 tests and 21 baseline comparisons passed.
- Step 2: complete; all 78 tests and 21 baseline comparisons passed.
- Step 3: complete; all 78 tests and 21 baseline comparisons passed.
- Step 4: complete; all 78 tests and 21 baseline comparisons passed.
- Step 5: complete; all 78 tests and 21 baseline comparisons passed.
- Step 6: complete; all 78 tests and 21 baseline comparisons passed.
- Step 7: complete; all 78 tests and 21 baseline comparisons passed.
- Step 8: complete; all 78 tests and 21 baseline comparisons passed.
- Step 9: complete; all 78 tests and 21 baseline comparisons passed.
- Step 10: complete; all 78 end-to-end tests, focused layout-contract tests,
  Unicode checks, and 21 baseline comparisons passed.
- Step 11a: chain continuation complete; the full test wrapper and 21 baseline
  comparisons passed.
- Step 11b: list continuation complete; the full test wrapper (including focused
  list-ownership checks) and 21 baseline comparisons passed.

## Validation

Each step uses the repository build and source-formatting wrappers, then the full
test wrapper. Existing golden, optimization, Unicode, preprocessor, macro,
diagnostic, source-formatting, and external-project idempotence checks must pass.
Compare representative syntax and break-tree dumps against the baseline as the
model, solver, and emitter boundaries move. Add focused coverage if an extraction
introduces a contract that existing behavioral tests do not exercise.
