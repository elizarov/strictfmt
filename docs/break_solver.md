# Break Solver

This document owns developer-facing details of the break solver in `src/format/impl/format_break_solver.h|cpp`. [format.md] owns the user-facing layout objective and legality constraints.

## Solver Contract

The solver receives a `FormatBreakModel` for one formatted segment and returns a `FormatBreakSolution`, which records the selected `FormatBreakChoice` and render base indentation for each selected structural break-model node. Recording every structural render base is necessary because a compact parent may contain an earlier child that changes the current indentation before a later child is emitted. The solution also records the selected continuation-line count for declaration owner/value nodes because declaration grouping depends on the physical size of the selected value layout. The pretty printer must emit exactly the selected choices and render bases; it must not re-run local layout decisions or infer hidden choices from child nodes.

`Better` implements the break-selection cost from [format.md].

After line count, the structural tie-break keeps both the maximum taken-break depth and the sum of all taken-break depths. The maximum prefers the layout whose structurally deepest break is shallower. The sum refines equal maxima: shared deep breaks, such as the body breaks of a nested lambda, contribute equally to both layouts, leaving a shallower distinguishing operator break cheaper than a deeper one. Both components are additive or monotone, so they participate in the normal composite-candidate dominance checks.

## Search Shape

`Solver::Solve` evaluates a subproblem identified by break node, current column, current indentation level, and whether the current line already has text. Both the best result and the complete ordered alternative set are memoized by this state. Reusing alternatives avoids repeating recursive search through nested layout combinations; it preserves candidate order and retains every continuation-sensitive choice. Cached results and their choice trees remain immutable for the lifetime of the segment's solver.

Each node kind exposes legal layout candidates through `SolveAlternatives`:

- Tokens produce one candidate.
- Sequences combine child candidates left to right.
- Delimited nodes expose compact, packed split, item-per-line split, and specialized indent-economy candidates.
- Chains, function signatures, body headers, and adjacent strings expose their own compact and split forms.

The solver compares complete candidates with `Better`. Intermediate candidate sets may be pruned only when the removed candidate cannot win any continuation under the same solver contract.

Composite candidates retain nondominated child layouts until all following children and suffix tokens have been costed. A locally best child is not sufficient when a later separator, comment, closer, or statement terminator can make another child layout win.

Nested qualified names with at least two non-leading `::` operators are normalized from the grammar's right-recursive
representation into left-associated binary break nodes. Every node owns one `::` and uses the ordinary after-operator
compact and split candidates. Consequently, the final qualification operator is closest to the break-model root and
earlier operators are successively deeper. The standard structural-depth cost therefore prefers the latest
qualification boundary when all earlier cost components are equal, without a qualification-specific weight or solver
tie-break. Independent binary choices also enumerate the layouts with multiple qualification breaks. A single
qualification, the leading global-scope `::`, and pointer-to-member declarator scope are not represented by these
nodes.

An adjacent-string node stores the exact safely joined spellings of its compact ordinary-literal runs. Compact solving
prices those spellings rather than the original separated tokens, and compact one-line probes use the same widths.
The split candidate continues to solve and emit the original literal tokens. The pretty printer consumes the stored
compact spellings only when the compact choice is selected, so it neither repeats escape-boundary analysis nor joins
tokens across a selected break.

When a mandatory block flushes pending tokens from inside nested comma lists, the break model supplies a virtual closer for every enclosing list up to the containing block. The solver therefore records each enclosing compact-or-split choice before any opener is emitted. Deferred comma and closer emission follows exactly the lists whose virtual delimiters selected split form; it does not infer an outer-list layout after the prefix has already been printed.

Deferred list separators belong to their nearest enclosing delimiter group. The shared ownership check traverses delimiter-free syntax wrappers, including comma expressions and conditional branches, but stops at nested groups or blocks. Both following-item detection and deferred comma emission (including preprocessor lists) use this check, so an outer list cannot force breaks or indentation inside an item.

Before a mandatory block is printed, the printer builds the containing source item's break model and records the break opportunities belonging to every uniform chain that crosses the block. Each formatted segment receives those requirements and the original chain base indentation, so it can select and emit the same split form on both sides of the boundary. Nested ternaries carry their colon breaks, while parenthesized comma expressions use the corresponding deferred-list state.

The function-signature candidate that keeps the return type and function name together while splitting the parameter list is legal only when the physical prefix through the parameter opener fits the column limit. A later unavoidable overflow, such as an atomic parameter type, does not make an additional avoidable prefix overflow legal.

Token candidates account for their complete physical rendering. Newlines embedded in token text contribute physical lines and reset the continuation column. Comment tokens also include the mandatory newline emitted after the comment and reset the continuation to the current indentation. A standalone comment between formatter-owned chain links remains in that chain model and is associated with the following operator, so both solving and emission use the chain-item indentation. These line transitions participate in the same overflow and line-count cost as selected break choices.

Compact one-line probes reject every token containing an intrinsic newline. Normal solving classifies a multiline literal token as an intrinsic tail expansion and propagates that classification only through an otherwise flat final sequence, chain operand, or delimiter item. Compact enclosing delimiters may therefore stay attached to the literal's first and last physical lines, while a multiline non-final item, comment, or earlier structural break still requires split form. The same classification is used for ordinary delimiters and transparent delimiter stacks; emission remains the normal compact choice.

Packed split and compact lists share inline-item candidate generation and final-item expansion checks. A packed split
adds the opener/body and body/closer breaks, rejects overflow in the item body, and records `SplitPacked`; emission and
deferred closers consume that choice and its selected base indentation. Chain continuation indentation is likewise
recorded from the selected chain layout before a mandatory block. Mandatory item separators exclude packed split.
Function-signature parameter-list expansion considers both split forms.

Syntax metadata identifies lists attached to names. Their name-prefix break depths are normalized below the list's
depth, including preceding calls in a callable expression. The existing structural cost then prefers list expansion to
name-internal breaks without a new weight or a break between the name and opener.

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
- Fast paths that prove the returned layout is already optimal for the subproblem, such as a compact one-line layout with zero overflow and zero extra lines.
- Specialized candidate generation that only omits layouts proven dominated by generated candidates.

Not allowed:

- Local rules that force a break because a nearby token would overflow unless that break is a legal candidate selected by `Better`.
- Arbitrary weights, preferences, or tie-breaks outside [format.md].
- Printer-side reconstruction of a choice that the solver did not record.
- Dropping a legal candidate because it looks unlikely to win without a dominance or optimality proof.

When adding a speedup whose proof is not obvious from the code, document the invariant in this file or with a short code comment near the pruning site.

For compact and packed-split lists, every non-final item must remain on the body's first physical line. A shared one-line probe checks those items and their separators before recursive enumeration, at each form's own starting column:

- The per-item split candidate is evaluated first. If it has zero maximum overflow and the compact prefix (including the opener) cannot fit, compact layouts are illegal or worse on the primary overflow cost and are skipped.
- Packed split requires a zero-overflow body. If its inline prefix cannot fit after the opening break, that entire branch is illegal and is skipped regardless of other candidates.

The final item is excluded from the probe because eligible non-angle layout may give it a multiline tail; angle lists reject that candidate later. These probes change search work, not candidate ordering or tie-breaking.

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
