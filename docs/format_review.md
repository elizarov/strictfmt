# Format Generality Review

This document records a generality audit of [format.md](format.md) and the current formatter implementation. It is a review record, not a normative formatting specification. `format.md` continues to own intended output, and [break_solver.md](break_solver.md) continues to own solver constraints.

The review looked for:

- line-layout decisions made outside the dynamic-programming break solver
- forced breaks that are not broad structural rules
- behavior selected by an exact keyword, operator spelling, literal suffix, or syntax-node kind when equivalent syntax is treated differently
- rules whose result depends on input whitespace or recursion depth instead of the formatted structure

Configured macro categories and the documented distinction between chain operators and ordinary binary operators are intentionally outside the findings. Those are supported semantic categories rather than accidental layout patches.

## Summary

| Finding | Contract affected |
| --- | --- |
| Structured macro values are pre-broken by printer heuristics | Solver ownership and generic macro layout |
| Declaration-group rules have no structural implementation | Specification/implementation agreement |
| Adjacent strings are forced by two raw literal suffixes | Generic break opportunities and lexical correctness |
| Keyword-owned value layout exists only for `return` and `co_return` | Generic expression ownership |
| Compact single-statement blocks exist only for lambdas | Generic block structure |
| Empty control blocks override the normal attachment category | Generic block attachment |
| Preprocessor blank lines depend on directive spellings | Generic item separation and exact directive recognition |
| Enum trailing commas are suppressed by a macro-call shape guess | Specification agreement and generic comma normalization |

## Resolved During Review

The delimiter-stack audit originally identified opener-run boundaries chosen by a current-column threshold and reconstructed by the printer. The initial examples did not prove a suboptimal result: when the greedy layout has zero overflow, delaying every boundary is a correct optimization because it minimizes the number of opener and closer runs.

A deeper-indentation counterexample did expose the missing case. Once delimiter lines overflowed, an additional run could reduce maximum overflow, which is more important than line count. The solver now uses the proven greedy fast paths only for zero-overflow layouts and detached layouts whose overflow is confined to the leaf, falls back to exact run-partition DP when delimiter overflow can change the result, records every selected run boundary in `FormatBreakSolution`, and makes the printer replay those choices. [break_solver.md](break_solver.md#delimiter-stacks) owns the optimization proof and fallback contract.

## Findings

### 1. Structured macro values are pre-broken by printer heuristics

`AnnotateMacroValueWidths` and the macro-boundary handling in `src/format/impl/format_pretty_printer.cpp` compute the compact width of the entire replacement. When the full replacement would overflow, the printer breaks before the first replacement token before building or solving its break model. This discards layouts that keep a fitting prefix with the replacement owner and split later at ordinary opportunities.

At `ColumnLimit: 22`, an ordinary call keeps the call owner and opener together and lets the solver split the list:

```cpp
int value = Build(
    first,
    second,
    third
);
```

The equivalent macro replacement is broken before `Build`, even though `#define VALUE Build( \` fits exactly in 22 columns and the argument-list breaks remain available:

```cpp
#define VALUE \
    Build( \
        first, \
        second, \
        third \
    )
```

There is a second non-DP mechanism in the same path. `RequiresMacroValueBreak` forces a break when a replacement list has more than one top-level child or its child has `MacroDeclarationFragment`, independent of width. With the default 120-column limit, a short expression replacement stays compact:

```cpp
#define SHORT(value) (value)
```

A declaration replacement of comparable size is forced onto a continuation line solely because of its node class:

```cpp
#define DECL(value) \
    void f(value)
```

The supported macro categories are not the problem. Once a replacement has been parsed as structured content, owner attachment and its ordinary break opportunities should be candidates in the same solver model. Only macro syntax requirements such as physical continuation boundaries should constrain those candidates.

### 2. Declaration-group rules have no structural implementation

The **Declaration Groups** section of `format.md` requires separation by declaration kind, type-declaration boundaries, field/method boundaries, and multi-line delimiter ownership. The implementation has no sibling classifier that applies those categories. `ContainsBlankLine` and `AppendTsChild` in `src/format/impl/format_model_builder.cpp` create `BlankLine` nodes from source whitespace, and the printer can preserve those nodes, but it does not derive required separators from declaration structure or from the selected layout.

Consequently, a source blank line can make field/method separation appear to work:

```cpp
class Kept {
    int field;

    void Method();
};
```

The same structural boundary is not inserted when the input omitted the blank line:

```cpp
class NotInserted {
    int field;
    void Method();
};
```

The same omission occurs between neighboring type declarations and around fields whose initializer owns a multi-line delimiter expansion. Declaration grouping needs one structural classification pass, including the selected multi-line state where the rule requires it, rather than relying on source-history markers.

### 3. Adjacent strings are forced by two raw literal suffixes

`EndsWithEscapedLineFragment` in `src/format/impl/format_break_model_builder.cpp` recognizes only raw text ending in `\n` or `\r\n`. `GroupAdjacentStrings` then sets `forceSplit`, which removes the compact candidate from the solver. The rule is tied to two content spellings rather than to the adjacent-string structure.

A normal adjacent-string chain remains an optional break opportunity and stays compact when it fits:

```cpp
const char* plain = "first " "second";
```

A literal whose runtime value ends in a backslash followed by `n` is nevertheless forced to split because the raw token also ends with the characters `\n`:

```cpp
const char* escapedBackslash =
    "first\\n"
    "second";
```

The latter is not a semantic newline fragment. This exposes both the lack of lexical escape parsing and the narrowness of the rule. Adjacent-literal boundaries should remain solver choices unless a generic token-preservation constraint makes a layout illegal; if semantic fragment categories are retained, they must be derived from parsed literal contents rather than raw suffix matching.

### 4. Keyword-owned value layout exists only for `return` and `co_return`

`BreakModelBuilder::BuildSyntaxNode` routes only `ReturnStatement` and `CoReturnStatement` through `BuildKeywordValueStatement`. That path creates a break opportunity after the keyword and applies flat forced-string indentation. `throw` and `co_yield` have the same visible keyword-plus-value shape but use the generic expression sequence instead. `format.md` mirrors the one-off by naming `return` as a special single-value context.

A forced adjacent-string sequence after `return` detaches from the keyword and aligns as one owned value:

```cpp
const char* GetMessage() {
    return
        "first line\n"
        "second line";
}
```

The analogous `throw` value keeps its first fragment on the keyword line, so the remaining fragment is indented relative to a different owner:

```cpp
void Fail() {
    throw "first line\n"
        "second line";
}
```

`co_yield` behaves like `throw`, while `co_return` behaves like `return`. The rule should either be a broad keyword-owned-value category that covers equivalent value-bearing syntax or be removed in favor of the value expression's ordinary break model.

### 5. Compact single-statement blocks exist only for lambdas

`format.md` explicitly exempts a single-statement lambda from mandatory statement and block breaks. `LambdaBodyAllowsCompactSingleStatementForm` in `src/format/impl/format_model.cpp` recognizes exactly `CompoundStatement` under `LambdaExpression`, and the pretty printer skips the mandatory semicolon break when `inSingleStatementLambdaBody` is set.

An ordinary one-statement callable body exposes its structural boundaries:

```cpp
void Ordinary() {
    return;
}
```

The structurally equivalent lambda body is collapsed only because its owner is a lambda:

```cpp
auto compact = []() { return; };
```

This is a syntax-owner exception rather than a block rule and works against the goal that braces, statements, and nesting be visually separated consistently. A generic policy should apply to all statement blocks of the same structural shape; the most structural policy is to keep mandatory block and statement breaks for lambdas too.

### 6. Empty control blocks override the normal attachment category

`format.md` first defines `else`, `catch`, `finally`, and do-while `while` as block-attachment keywords, then gives compact empty control bodies an exception. `PrettyPrinter::ShouldBreakAfterCompactEmptyBlock` implements that exception with next-token checks and separately exempts the exact do-while `while` case.

A non-empty `if` body uses the general attachment rule:

```cpp
if (ready) {
    Run();
} else {
    Reset();
}
```

Changing only the body cardinality moves the same attachment keyword to a new line:

```cpp
if (ready) {}
else {}
```

An empty `do {}` still attaches its `while`, so empty blocks do not even form one consistent exception category. Attachment should be determined by the following structural role, independent of whether the preceding block contains zero or more statements.

### 7. Preprocessor blank lines depend on directive spellings

The **Line Hygiene** section of `format.md` assigns unique blank-line behavior to `#pragma once` and `#undef`. The printer implements `#undef` by directive kind but recognizes `#pragma once` with `StartsWith(line, "#pragma once")`. `IsPragmaOnceNode` in `src/format/impl/format_model_builder.cpp` uses the same raw prefix test when grouping an opening include area.

An ordinary definition at a declaration boundary receives only the generic post-directive separator:

```cpp
int before;
#define FEATURE 1

int after;
```

Changing only the directive keyword gives `#undef` separators on both sides:

```cpp
int before;

#undef FEATURE

int after;
```

The spelling check also accepts unrelated pragma operands. A normal unknown pragma before an include has no inserted separator:

```cpp
#pragma twice
#include <algorithm>
```

An equally unknown pragma whose operand merely begins with `once` is treated as `#pragma once`:

```cpp
#pragma once_more

#include <vector>
```

Blank-line insertion should be expressed through broad source-item categories. If `#pragma once` remains a necessary semantic exception, it should at least be recognized structurally as the exact directive and operand rather than by raw prefix.

### 8. Enum trailing commas are suppressed by a macro-call shape guess

`format.md` says that enum bodies always keep a trailing comma. `NormalizeTrailingCommas` in `src/format/impl/format_model_builder.cpp` instead calls `MacroLikeInvocationEnding` and suppresses the comma when the final enum child happens to have an identifier-plus-argument-list shape. This is not driven by a configured macro role or by known expansion semantics; it guesses that the call may generate its own comma-separated enumerators.

An ordinary final enumerator follows the generic enum rule:

```cpp
enum class Plain {
    Default,
};
```

The configured structured-macro fixture produces an enum whose final call-shaped child is exempted from the rule:

```cpp
#define ENUM_STRING_DECLARE(EnumType, ItemsMacro) \
    enum class EnumType { \
        ItemsMacro(ENUM_STRING_DECLARE_ENUMERATOR) \
    };
```

The formatter cannot infer whether `ItemsMacro` expands with no terminal comma, one terminal comma, several enumerators, or no enumerator at all. The exemption is also sensitive to the internal wrapper shape, so equivalent call text parsed in a different context need not receive it. Comma normalization should use the generic enum rule unless an explicit supported macro category supplies the otherwise unknowable expansion contract.

## Overall Direction

The remaining direct solver violation should be corrected first: macro owner/value attachment must become explicit candidates selected by `Better` and faithfully replayed by the printer. The other findings should be resolved by deleting the exception or replacing it with a small semantic category that applies at every recursion depth and in every equivalent context. Golden fixtures should then demonstrate category boundaries rather than preserve the current one-off outputs.
