# Break Solver

This document owns developer-facing details of the break solver in `src/tools/impl/format_break_solver.h|cpp`. `docs/format.md` owns the user-facing layout objective and legality constraints.

## Solver Contract

The solver receives a `FormatBreakModel` for one formatted segment and returns a `FormatBreakSolution`, which records the selected `FormatBreakChoice` for each break model node. The pretty printer must emit exactly the selected choices; it must not re-run local layout decisions or infer hidden choices from child nodes.

`Better` implements the break-selection cost from `docs/format.md`. Any change to that ordering changes the formatter spec and must update `docs/format.md` in the same change.

## Search Shape

`Solver::Solve` evaluates a subproblem identified by break node, current column, current indentation level, and whether the current line already has text. It memoizes solved subproblems.

Each node kind exposes legal layout candidates through `SolveAlternatives`:

- Tokens produce one candidate.
- Sequences combine child candidates left to right.
- Delimited nodes expose compact, split, and specialized indent-economy candidates.
- Chains, function signatures, body headers, and adjacent strings expose their own compact and split forms.

The solver compares complete candidates with `Better`. Intermediate candidate sets may be pruned only when the removed candidate cannot win any continuation under the same solver contract.

## Choice Fidelity

Every observable layout distinction that the pretty printer needs must be represented by `FormatBreakChoice`. For example, delimiter-stack layouts distinguish an attached compact leaf from a detached leaf with separate choices. This keeps selection in the solver and emission in the printer.

Do not encode an observable layout distinction as a printer-side heuristic such as "if a child uses any split choice, print this parent differently." If two layouts can produce different output, they are separate solver candidates.

## Allowed Speedups

Heuristics are allowed only when they are equivalence-preserving: the result must be the same as exhaustive enumeration of all legal layouts under the objective in `docs/format.md`.

Allowed speedups include:

- Memoization by solver state.
- Rejecting layouts that violate documented legality constraints.
- Dominance pruning when another candidate with the same continuation state is no worse in every cost component and better in at least one.
- Fast paths that prove the returned layout is already optimal for the subproblem, such as a compact one-line layout with zero overflow and zero extra lines.
- Specialized candidate generation that only omits layouts proven dominated by generated candidates.

Not allowed:

- Local rules that force a break because a nearby token would overflow unless that break is a legal candidate selected by `Better`.
- Arbitrary weights, preferences, or tie-breaks outside `docs/format.md`.
- Printer-side reconstruction of a choice that the solver did not record.
- Dropping a legal candidate because it looks unlikely to win without a dominance or optimality proof.

When adding a speedup whose proof is not obvious from the code, document the invariant in this file or with a short code comment near the pruning site.

## Delimiter Stacks

Transparent single-item delimiter stacks are an indent-economy specialization. Their solver candidates must still obey the same contract:

- Compact stack layout is a normal compact candidate.
- Split stack layout with an attached leaf is a candidate only when the leaf itself has no selected breaks.
- Split stack layout with a detached leaf is a separate candidate.
- The solver chooses among those candidates with `Better`; leaf detachment is not decided by overflow or child-choice heuristics.

The opener-run construction may be specialized for speed only when it preserves the best legal layout under the break-selection cost.
