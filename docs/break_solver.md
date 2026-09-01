# Break Solver

This document owns developer-facing details of the break solver and its shared cost profile in `src/format/impl/format_break_solver.h|cpp` and `format_value_profile.h|cpp`. [format.md] owns the user-facing layout objective and legality constraints.

## Solver Contract

The solver receives a `FormatBreakModel` for one formatted segment and returns a `FormatBreakSolution`, which records the selected `FormatBreakChoice` and render base indentation for each selected structural break-model node. Recording every structural render base is necessary because a compact parent may contain an earlier child that changes the current indentation before a later child is emitted. The solution also records the selected continuation-line count for declaration owner/value nodes because declaration grouping depends on the physical size of the selected value layout. The pretty printer must emit exactly the selected choices and render bases; it must not re-run local layout decisions or infer hidden choices from child nodes.

`Better` implements the break-selection cost from [format.md].

`FormatValueProfile` stores sorted occurrence counts for both cost profiles. It keeps four values inline and ignores zero, so an empty profile performs no heap allocation. Profile comparison proceeds from greatest value to least, and `Merge` adds child occurrence counts. Adding a shared surrounding profile therefore cannot change the greatest value where two alternatives differ.

The overflow-size profile contains completed physical lines. The current unfinished line is compared as one virtual occurrence and enters the stored profile only when a token newline, comment termination, or selected break completes it. A child that completes the caller's current line owns that occurrence, so profiles remain additive across `Merge`. Exact delimiter-stack search may price and then restore an outer line; its result marks that line as already recorded until the next break to prevent duplicate occurrences.

`AddBreak` records the current node's nonzero expansion cost on its first break and sets `ownExpansionCharged`; subsequent breaks only affect physical layout. `Merge` adds child occurrence counts without copying that flag, so a child cannot pay for its parent's expansion. Partial results include the flag in continuation-state comparison and dominance pruning because it changes the profile of a later break. Each independent subproblem starts uncharged; memoization needs no caller history.

A packed list's separately evaluated body inherits the opener's charged flag, but starts without the prefix's profile; the profiles merge afterward. Attached-open solving starts a fresh child result before merging it into the operator prefix. Both paths therefore obey the same ownership rule as ordinary recursive solving.

The builder retains initial depth in `rawDepth` and materializes the depth adjustments specified in [format.md] in `structuralDepth`. After building the complete break model, `NormalizeBreakCosts` initializes `breakCost` from the adjusted depth and applies the specified subtree adjustments from outer subtrees inward. Costs are fixed before solving, so memoization needs no layout-history state, and the pretty printer's choices and indentation rules are unchanged.

## Search Shape

`Solver::Solve` evaluates a subproblem identified by break node, current column, current indentation level, and whether the current line already has text. Both the best result and the complete ordered alternative set are memoized by this state. Reusing alternatives avoids repeating recursive search through nested layout combinations; it preserves candidate order and retains every continuation-sensitive choice. Cached results and their choice trees remain immutable for the lifetime of the segment's solver.

Each node kind exposes legal layout candidates through `SolveAlternatives`:

- Tokens produce one candidate.
- Sequences combine child candidates left to right.
- Delimited nodes expose compact, packed split, item-per-line split, and specialized indent-economy candidates.
- Prefix lists expose compact, packed split, and item-per-line split through the same candidate enumeration used by both solving paths. Packed split records `SplitPacked`, prices the prefix break normally, and uses the shared compact-item probe at continuation indentation to require a single fitting item line. The following body is costed by its enclosing node.
- Chains, function signatures, body headers, and adjacent strings expose their own compact and split forms.

Syntax-local `PrefixList` metadata routes colon-prefixed lists through the same builder, including grammar nodes without a dedicated formatter kind. An empty list leaves its prefix token in the surrounding sequence.

The solver compares complete candidates with `Better`. Intermediate candidate sets may be pruned only when the removed candidate cannot win any continuation under the same solver contract.

Composite candidates retain nondominated child layouts until their shared continuation has been costed. This includes function-signature children, ternary operands and operators, and list items with separators. A flat-only parent filters the child's flat alternatives; it does not reject the parent merely because the child's locally best layout breaks.

Qualified names are normalized into left-associated binary break nodes. Every node owns one non-leading `::` and uses the ordinary after-operator
compact and split candidates. Consequently, the final qualification operator is closest to the break-model root and
earlier operators are successively deeper. The standard break cost therefore prefers the latest
qualification boundary when all earlier cost components are equal, without a qualification-specific weight or solver
tie-break. Independent binary choices also enumerate the layouts with multiple qualification breaks. The leading
global-scope `::` remains attached to the first operand. Syntax metadata applies the same construction to scoped
declarators and namespace names.

An adjacent-string node stores the exact safely joined spellings of its compact ordinary-literal runs. Compact solving
prices those spellings rather than the original separated tokens, and compact one-line probes use the same widths.
The split candidate continues to solve and emit the original literal tokens. The pretty printer consumes the stored
compact spellings only when the compact choice is selected, so it neither repeats escape-boundary analysis nor joins
tokens across a selected break.

When a mandatory block flushes pending tokens from inside nested delimiter groups, the break model supplies a virtual closer for every enclosing group up to the containing block. Groups are identified by matched delimiters, regardless of item count. The solver therefore records each enclosing compact-or-split choice before any opener is emitted. Deferred comma and closer emission follows exactly the groups whose virtual delimiters selected split form; it does not infer an outer layout after the prefix has already been printed.

Deferred list separators belong to their nearest enclosing delimiter group. The shared ownership check traverses delimiter-free syntax wrappers, including comma expressions and conditional branches, but stops at nested groups or blocks. Both following-item detection and deferred comma emission (including preprocessor lists) use this check, so an outer list cannot force breaks or indentation inside an item.

Before a mandatory block is printed, the printer builds the containing source item's break model and records the break opportunities belonging to every uniform chain that crosses the block. Each formatted segment receives those requirements and the original chain base indentation, so it can select and emit the same split form on both sides of the boundary. Nested ternaries carry their colon breaks, while parenthesized comma expressions use the corresponding deferred-list state.

The function-signature candidate that keeps the return type and function name together while splitting the parameter list is legal only when the physical prefix through the parameter opener fits the column limit. A later unavoidable overflow, such as an atomic parameter type, does not make an additional avoidable prefix overflow legal.

Token candidates account for their complete physical rendering. Newlines embedded in token text contribute physical lines and reset the continuation column. Comment tokens also include the mandatory newline emitted after the comment and reset the continuation to the current indentation. A standalone comment between formatter-owned chain links remains in that chain model and is associated with the following operator, so both solving and emission use the chain-item indentation. These line transitions participate in the same optimization cost as selected break choices.

Compact one-line probes reject every token containing an intrinsic newline. Normal solving classifies a multiline literal token as an intrinsic tail expansion and propagates that classification only through an otherwise flat final sequence, chain operand, or delimiter item. Compact enclosing delimiters may therefore stay attached to the literal's first and last physical lines, while a multiline non-final item, comment, or earlier structural break still requires split form. The same classification is used for ordinary delimiters and transparent delimiter stacks; emission remains the normal compact choice.

Packed split and compact lists share inline-item candidate generation and final-item expansion checks. A packed split
adds the opener/body and body/closer breaks, rejects overflow in the item body, and records `SplitPacked`; emission and
deferred closers consume that choice and its selected base indentation. Chain continuation indentation is likewise
recorded from the selected chain layout before a mandatory block. Mandatory item separators exclude packed split.
Function-signature parameter-list expansion considers both split forms.

Owner/value syntax uses one generic after-owner candidate shape.

A structured macro definition is built as its header owner plus its complete replacement value. Its compact chain candidate is legal only when the whole definition fits on one physical line; all other candidates split after the owner and solve the complete replacement recursively. A replacement parsed as multiple top-level call units is represented by a force-split statement sequence inside that value, so its required unit boundaries compose with the required header boundary without printer inference. The solver's break-line suffix width accounts for text emitted only on taken breaks; structured macros use it for the trailing ` \`, including that physical suffix in overflow cost without making it a break heuristic.

## Choice Fidelity

Every observable layout distinction that the pretty printer needs must be represented by `FormatBreakChoice`. For example, delimiter-stack layouts distinguish an attached compact leaf from a detached leaf with separate choices. This keeps selection in the solver and emission in the printer.

Do not encode an observable layout distinction as a printer-side heuristic such as "if a child uses any split choice, print this parent differently." If two layouts can produce different output, they are separate solver candidates.

## Allowed Speedups

Heuristics are allowed only when they are equivalence-preserving: the result must be the same as exhaustive enumeration of all legal layouts under the objective in [format.md].

Allowed speedups include:

- Memoization by solver state.
- Rejecting layouts that violate documented legality constraints.
- Dominance pruning when another candidate with the same continuation state is no worse in every cost component and better in at least one.
- Fast paths that prove the returned layout is already optimal for the subproblem, such as a compact one-line layout with an empty overflow-size profile and zero extra lines.
- Specialized candidate generation that only omits layouts proven dominated by generated candidates.

Not allowed:

- Local rules that force a break because a nearby token would overflow unless that break is a legal candidate selected by `Better`.
- Arbitrary weights, preferences, or tie-breaks outside [format.md].
- Printer-side reconstruction of a choice that the solver did not record.
- Dropping a legal candidate because it looks unlikely to win without a dominance or optimality proof.

When adding a speedup whose proof is not obvious from the code, document the invariant in this file or with a short code comment near the pruning site.

Dominance requires equal continuation state except that the dominating candidate may end at an earlier column. Cost profiles compose by addition, so adding a shared continuation preserves their first differing value. Starting that continuation earlier cannot increase its overflow relative to the dominated candidate. The same argument permits replacing equal-state candidates with the locally better one.

An exact search may reject a partial candidate whose greatest overflow already exceeds the best complete candidate's greatest overflow. Later layout cannot remove a completed-line occurrence, so that candidate loses at the first optimization tier regardless of its break cost or line count.

For compact and packed-split lists, every non-final item must remain on the body's first physical line. A shared one-line probe checks those items and their separators before recursive enumeration, at each form's own starting column:

- The per-item split candidate is evaluated first. If it has an empty overflow-size profile and the compact prefix (including the opener) cannot fit, compact layouts are illegal or worse on the primary cost and are skipped.
- Packed split requires a zero-overflow body. If its inline prefix cannot fit after the opening break, that entire branch is illegal and is skipped regardless of other candidates.

The final item is excluded from the probe because eligible non-angle layout may give it a multiline tail; angle lists reject that candidate later. These probes change search work, not candidate ordering or tie-breaking.

## Delimiter Stacks

Transparent single-item delimiter stacks are an indent-economy specialization. Their solver candidates must still obey the same contract:

- Compact stack layout is a normal compact candidate.
- Split stack layout with an attached leaf is a candidate only when the leaf itself has no selected breaks.
- Split stack layout with a detached leaf is a separate candidate.
- The solver chooses among those candidates with `Better`; leaf detachment is not decided by overflow or child-choice heuristics.

The opener-run construction uses a greedy zero-overflow fast path. It delays each run boundary until the next opener would overflow. If the completed candidate has an empty overflow-size profile, this is optimal under the normal cost:

- The empty overflow-size profile is minimal.
- All opener-run and matching closer breaks belong to one stack-root expansion. Once charged, extra runs add no cost and only reduce the space available to the leaf.
- By induction over runs, breaking earlier leaves at least as many openers for a line at the same or deeper indentation, so it cannot use fewer runs.
- Each extra run adds an opener line and a matching closer line. Among equally charged expansions, the minimum-run partition therefore minimizes line count.
- Among equal-run partitions, the latest boundaries are the source-order-stable compact choice and leave the final run no wider than an earlier partition.

The proof does not apply once the greedy candidate overflows. A different partition may exchange a small delimiter overflow for a larger leaf overflow, or improve a later occurrence in the overflow-size profile. The solver then uses exact partition DP and compares complete candidates with `Better`. A zero-overflow attached-leaf search uses the minimum-run invariant to restrict its prefix search without discarding a winning layout.

Every selected opener-run boundary is stored as `SplitDelimiterStackRun` on the corresponding opener node. The pretty printer reads those choices; it does not repeat the threshold calculation.
