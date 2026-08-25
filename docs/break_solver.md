# Break Solver

This document owns developer-facing details of the break solver in `src/format/impl/format_break_solver.h|cpp`. [format.md] owns the user-facing layout objective and legality constraints.

## Solver Contract

The solver receives a `FormatBreakModel` for one formatted segment and returns a `FormatBreakSolution`, which records the selected `FormatBreakChoice` for each break model node and the render base indentation for delimiter choices. The pretty printer must emit exactly the selected choices; it must not re-run local layout decisions or infer hidden choices from child nodes. Declaration grouping uses the recorded delimiter indentation to recognize a selected list whose closer returns to declaration indentation.

`Better` implements the break-selection cost from [format.md].

## Search Shape

`Solver::Solve` evaluates a subproblem identified by break node, current column, current indentation level, and whether the current line already has text. It memoizes solved subproblems.

Each node kind exposes legal layout candidates through `SolveAlternatives`:

- Tokens produce one candidate.
- Sequences combine child candidates left to right.
- Delimited nodes expose compact, split, and specialized indent-economy candidates.
- Chains, function signatures, body headers, and adjacent strings expose their own compact and split forms.

The solver compares complete candidates with `Better`. Intermediate candidate sets may be pruned only when the removed candidate cannot win any continuation under the same solver contract.

Owner/value syntax uses one generic after-owner candidate shape. A structured macro definition is built as its header owner plus its complete replacement value. Its compact chain candidate is legal only when the whole definition fits on one physical line; all other candidates split after the owner and solve the complete replacement recursively. A replacement parsed as multiple top-level call units is represented by a force-split statement sequence inside that value, so its required unit boundaries compose with the required header boundary without printer inference. The solver's break-line suffix width accounts for text emitted only on taken breaks; structured macros use it for the trailing ` \`, including that physical suffix in overflow cost without making it a break heuristic.

## Choice Fidelity

Every observable layout distinction that the pretty printer needs must be represented by `FormatBreakChoice`. For example, delimiter-stack layouts distinguish an attached compact leaf from a detached leaf with separate choices. This keeps selection in the solver and emission in the printer.

Do not encode an observable layout distinction as a printer-side heuristic such as "if a child uses any split choice, print this parent differently." If two layouts can produce different output, they are separate solver candidates.

## Allowed Speedups

Heuristics are allowed only when they are equivalence-preserving: the result must be the same as exhaustive enumeration of all legal layouts under the objective in [format.md].

Allowed speedups include:

- Memoization by solver state.
- Rejecting layouts that violate documented legality constraints.
- Dominance pruning when another candidate with the same continuation state is no worse in every cost component and better in at least one.
- Fast paths that prove the returned layout is already optimal for the subproblem, such as a compact one-line layout with zero overflow and zero extra lines.
- Specialized candidate generation that only omits layouts proven dominated by generated candidates.

Not allowed:

- Local rules that force a break because a nearby token would overflow unless that break is a legal candidate selected by `Better`.
- Arbitrary weights, preferences, or tie-breaks outside [format.md].
- Printer-side reconstruction of a choice that the solver did not record.
- Dropping a legal candidate because it looks unlikely to win without a dominance or optimality proof.

When adding a speedup whose proof is not obvious from the code, document the invariant in this file or with a short code comment near the pruning site.

For a compact delimited list with at least two items, every non-final item is constrained to remain on the opener's physical line. Before enumerating compact candidates, the solver probes the opener, the non-final items, and their separators with the one-line renderer. If that prefix cannot remain legal and within the column limit while the split candidate has zero maximum overflow, every compact candidate is either illegal or worse on the primary overflow cost, so the solver returns the split candidate without enumerating compact layouts. The final item is excluded from the probe because compact layout may legally give it a multiline tail.

## Delimiter Stacks

Transparent single-item delimiter stacks are an indent-economy specialization. Their solver candidates must still obey the same contract:

- Compact stack layout is a normal compact candidate.
- Split stack layout with an attached leaf is a candidate only when the leaf itself has no selected breaks.
- Split stack layout with a detached leaf is a separate candidate.
- The solver chooses among those candidates with `Better`; leaf detachment is not decided by overflow or child-choice heuristics.

The opener-run construction uses a greedy zero-overflow fast path. It delays each run boundary until the next opener would overflow. If the completed candidate has zero overflow, this is optimal under the normal cost:

- Zero is the minimum possible maximum overflow and overflowing-line count.
- By induction over runs, breaking earlier leaves at least as many openers for a line at the same or deeper indentation, so it cannot use fewer runs.
- Each extra run adds an opener line and a matching closer line. The minimum-run partition therefore minimizes line count.
- Among equal-run partitions, the latest boundaries are the source-order-stable compact choice and leave the final run no wider than an earlier partition.

For a detached leaf, the same proof applies when only the leaf overflows but every delimiter line fits: the minimum run count gives the shallowest leaf indentation, and another run cannot improve the leaf layout.

The proof does not apply when a delimiter line overflows. An extra run can then reduce maximum overflow even though it adds lines. The solver uses exact partition DP for that case and compares the complete candidates with `Better`. A zero-overflow attached-leaf search uses the same minimum-run invariant to restrict its prefix search without discarding a winning layout.

Every selected opener-run boundary is stored as `SplitDelimiterStackRun` on the corresponding opener node. The pretty printer reads those choices; it does not repeat the threshold calculation.
