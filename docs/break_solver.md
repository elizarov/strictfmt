# Break Solver

This document owns developer-facing details of the break solver and its shared cost profile in `src/format/impl/format_break_solver.h|cpp` and `format_value_profile.h|cpp`. [format.md] owns the user-facing layout objective and legality constraints.

## Solver Contract

The solver receives a `FormatBreakModel` for one formatted segment and returns a `FormatBreakSolution`, which records the selected `FormatBreakChoice` and render base indentation for each selected structural break-model node. Recording every structural render base is necessary because a compact parent may contain an earlier child that changes the current indentation before a later child is emitted. The solution also records the selected continuation-line count for declaration owner/value nodes because declaration grouping depends on the physical size of the selected value layout. The break emitter must emit exactly the selected choices and render bases; it must not re-run local layout decisions or infer hidden choices from child nodes.

`Better` implements the break-selection cost from [format.md].

`FormatCompactLayout` owns cached compact physical-line measurement; its results contain no search or cost state.

`FormatChoiceHistory` owns immutable shared decision histories and their lookup/materialization precedence. Solver candidates carry opaque handles; `FormatBreakSolution` is the materialized contract consumed by emission and diagnostics.

`FormatValueProfile` stores sorted occurrence counts for both cost profiles. It keeps four values inline, allocates overflow storage only for a fifth distinct value, and ignores zero. Profile comparison proceeds from greatest value to least, and `Merge` adds child occurrence counts. Adding a shared surrounding profile therefore cannot change the greatest value where two alternatives differ.

The overflow-size profile contains completed physical lines. The current unfinished line is compared as one virtual occurrence and enters the stored profile only when a token newline, comment termination, or selected break completes it. A child that completes the caller's current line owns that occurrence, so profiles remain additive across `Merge`. Exact delimiter-stack search may price and then restore an outer line; its result marks that line as already recorded until the next break to prevent duplicate occurrences.

`AddBreak` records the current node's nonzero expansion cost on its first break and sets `ownExpansionCharged`; subsequent breaks only affect physical layout. `Merge` adds child occurrence counts without copying that flag, so a child cannot pay for its parent's expansion. Partial results include the flag in continuation-state comparison and dominance pruning because it changes the profile of a later break. Each independent subproblem starts uncharged; memoization needs no caller history.

A packed list's separately evaluated body inherits the opener's charged flag, but starts without the prefix's profile; the profiles merge afterward. Attached-open solving starts a fresh child result before merging it into the operator prefix. Both paths therefore obey the same ownership rule as ordinary recursive solving.

The builder retains initial depth in `rawDepth`; `FormatBreakCostNormalizer` materializes the depth adjustments specified in [format.md] in `structuralDepth`. `breakCost` starts at the same depth and every structural-depth shift updates both values. After building the complete model, the normalizer applies the specified subtree discounts from outer subtrees inward. Costs are fixed before solving, so memoization needs no layout-history state, and the emission choices and indentation rules are unchanged.

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

Composite candidates retain nondominated child layouts until their shared continuation has been costed. Parent legality constraints filter each child alternative independently; the child's locally best layout does not determine legality for other alternatives.

Literal pairing in `+` chains and stream insertions shares the literal classifier and compact follower probe. The probe requires complete operands, excluding partial views with context-only tokens across mandatory blocks. Both the attached-follower candidate and the operator-break candidate remain available until their continuation is costed. A pair fitting inside the chain subtree does not prove that enclosing punctuation or following syntax fits. The normal overflow, expansion, and line-count comparison selects the complete layout; an early binding decision must not discard the shorter continuation state. Selected pairs record their intervening operator in the solution, so emission omits exactly those chain breaks.

Qualified names are normalized into left-associated binary break nodes. Every node owns one non-leading `::` and uses the ordinary after-operator
compact and split candidates. Consequently, the final qualification operator is closest to the break-model root and
earlier operators are successively deeper. The standard break cost therefore prefers the latest
qualification boundary when all earlier cost components are equal, without a qualification-specific weight or solver
tie-break. Independent binary choices also enumerate the layouts with multiple qualification breaks. The leading
global-scope `::` remains attached to the first operand. Syntax metadata applies the same construction to scoped
declarators and namespace names.

Name-prefix normalization includes template argument lists. For a list in a qualified name's right operand, it adjusts the qualification decision and its left prefix without shifting the attached list.

A qualified declaration type and its declarator form an independent outer break node. Its type subtree is structurally deeper, so the boundary is cheaper than every break inside the type without a surcharge. Pointer and reference wrappers are partitioned with the type, assignment tails remain outside this grouping, and callable declarations retain the ordinary function-signature model and its prefix surcharge.

An adjacent-string node stores the exact safely joined spellings of its compact ordinary-literal runs. Compact solving
prices those spellings rather than the original separated tokens, and compact one-line probes use the same widths.
The split candidate continues to solve and emit the original literal tokens. The break emitter consumes the stored
compact spellings only when the compact choice is selected, so it neither repeats escape-boundary analysis nor joins
tokens across a selected break.

When a mandatory block flushes pending tokens from inside a list, the break model preserves the selected layout of every enclosing matched-delimiter group and colon-prefixed list up to the containing block. `FormatListContinuation` supplies virtual closers and retains selected list indentation, so the solver records compact-or-split choices before any opener is emitted. A colon-prefixed list needs no virtual closer: if its prefix-list node selects full-split form, deferred emission continues breaking its top-level commas until printing leaves the list. The printer therefore follows recorded choices across the block boundary instead of inferring an outer layout after the prefix has already been printed. The terminal block opener also carries its selected render base into the brace frame, with macro continuation indentation removed from the structural base. Enclosing list-item indentation governs deferred separators; it must not replace the opener’s own base when a nested chain or header introduces another indentation level.

Source blank-line separators between modeled list items are recorded on the following item, so the solver and emitter account for the same empty output line. The printer’s compact fast path must defer to the break model when a buffered segment contains a source blank-line marker. A source blank line encountered after a mandatory block preserves the pending item indentation selected for the next segment.

Deferred list separators belong to their nearest enclosing list. The shared ownership check traverses delimiter-free syntax wrappers, including comma expressions and conditional branches, but stops at nested delimiter groups or blocks. Both following-item detection and deferred comma emission (including prefix and preprocessor lists) use this check, so an outer list cannot force breaks or indentation inside an item.

Before a mandatory block is printed, `FormatChainContinuation` builds the containing source item's break model and records the break opportunities belonging to every uniform chain that crosses the block. Each formatted segment receives those requirements and the original chain base indentation, so it can select and emit the same split form on both sides of the boundary. Nested ternaries carry their colon breaks, while parenthesized comma expressions use the corresponding deferred-list state.

The function-signature candidate that keeps the return type and function name together while splitting the parameter list is legal only when the physical prefix through the parameter opener fits the column limit. A later unavoidable overflow, such as an atomic parameter type, does not make an additional avoidable prefix overflow legal.

Token candidates account for their complete physical rendering. Newlines embedded in token text contribute physical lines and reset the continuation column. Comment tokens also include the mandatory newline emitted after the comment and reset the continuation to the current indentation. A standalone comment between formatter-owned chain links remains in that chain model and is associated with the following link, so both solving and emission use the chain-item indentation. These line transitions participate in the same optimization cost as selected break choices.

Binary and stream chain construction collects boundary comments before flattening operands and attaches trailing comments to the preceding operand. A trailing operator moved before those comments is emitted in that operand's suffix; its chain entry remains context-only so break ownership is unchanged. Standalone comment prefixes of receiverless stream links are normalized into the following chain node. Reordered adjacency recomputes token spacing before operand construction, so solving and emission consume the same normalized token order.

Compact one-line probes reject every token containing an intrinsic newline. Normal solving classifies a multiline literal token as an intrinsic tail expansion and propagates that classification only through an otherwise flat final sequence, chain operand, or delimiter item. Compact enclosing delimiters may therefore stay attached to the literal's first and last physical lines, while a multiline non-final item, comment, or earlier structural break still requires split form. The same classification is used for ordinary delimiters and transparent delimiter stacks; emission remains the normal compact choice.

Packed split and compact lists share inline-item candidate generation and final-item expansion checks. A packed split
adds the opener/body and body/closer breaks, rejects overflow in the item body, and records `SplitPacked`; emission and
deferred closers consume that choice and its selected base indentation. Chain continuation indentation is likewise
recorded from the selected chain layout before a mandatory block. Mandatory item separators exclude packed split.
Function-signature parameter-list expansion considers both split forms.

The builder classifies list items by their outer syntax and marks sibling initializer-record lists with the shared unbroken-item requirement used by compact and packed candidates.

The builder records a comma-separated brace list's terminal comma as layout metadata instead of an ordinary item separator. Full-split candidates price it before a trailing comment; compact and packed candidates omit it. Emission follows that recorded list choice.

Owner/value syntax, including value-owning keywords and trailing-return arrows, uses one generic after-owner candidate shape.

A structured macro definition is built as its header owner plus its complete replacement value. Its compact chain candidate is legal only when the whole definition fits on one physical line; all other candidates split after the owner and solve the complete replacement recursively. A replacement parsed as multiple top-level call units is represented by a force-split statement sequence inside that value, so its required unit boundaries compose with the required header boundary without printer inference. The solver's break-line suffix width accounts for text emitted only on taken breaks; structured macros use it for the trailing ` \`, including that physical suffix in overflow cost without making it a break heuristic.

## Choice Fidelity

Every observable layout distinction that the pretty printer needs must be represented by `FormatBreakChoice`. For example, delimiter-stack layouts distinguish an attached compact leaf from a detached leaf with separate choices. This keeps selection in the solver and emission in the printer.

Do not encode an observable layout distinction as a printer-side heuristic such as "if a child uses any split choice, print this parent differently." If two layouts can produce different output, they are separate solver candidates.

## Allowed Speedups

Heuristics are allowed only when they are equivalence-preserving: the result must be the same as exhaustive enumeration of all legal layouts under the objective in [format.md].

Allowed speedups include:

- Memoization by solver state.
- Reusing an already built and solved declaration model when the buffered token span, incoming print state, suffix width, and every model context input are identical.
- Caching an exact compact physical-line shape by immutable break node. The shape records compact legality, text production, and widths with and without preceding line text; applying it is equivalent to recursively appending every compact token.
- Caching exact recursive predicates by immutable break or syntax node when the cache key includes every predicate input.
- Materializing an exact descendant predicate on its owning immutable syntax node during normalization when all later queries use that same root and predicate.
- Recording monotonic builder summaries, such as whether a model contains any layout choice or final-lambda discount target, while visiting the nodes that determine them.
- Omitting a recursive transformation when its complete input summary proves that it is an identity operation, such as final-lambda normalization without a final lambda or required-chain propagation without required operators.
- Bypassing the solver for a model containing only token and sequence nodes, because such a model has no layout choice; normal model emission still runs.
- Mutating an exclusively owned candidate in place instead of copying it before each token, break, or merge operation.
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

A compact uniform-chain candidate may solve an intermediate operand with the all-compact physical-line walker when the chain legality check would reject every selected break in that operand. A successful walk is the unique no-selected-break candidate, including when it overflows. If comments or intrinsic newlines make the walk inapplicable, normal alternative enumeration and exact filtering remain in use.

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
