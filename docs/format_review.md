# Format Generality Review

This document records a generality audit of [format.md](format.md) and the current formatter implementation. It is a review record, not a normative formatting specification. `format.md` continues to own intended output, and [break_solver.md](break_solver.md) continues to own solver constraints.

The review looked for:

- line-layout decisions made outside the dynamic-programming break solver
- forced breaks that are not broad structural rules
- behavior selected by an exact keyword, operator spelling, literal suffix, or syntax-node kind when equivalent syntax is treated differently
- rules whose result depends on input whitespace or recursion depth instead of the formatted structure

Configured macro categories, the documented distinction between chain operators and ordinary binary operators, mandatory adjacent-string line-fragment boundaries, compact single-statement lambdas, and compact empty control bodies are intentionally outside the findings. Those are supported semantic categories and readability or expression-layout rules rather than accidental layout patches.

## Summary

| Finding | Contract affected |
| --- | --- |
| Preprocessor blank lines depend on directive spellings | Generic item separation and exact directive recognition |
| Enum trailing commas are suppressed by a macro-call shape guess | Specification agreement and generic comma normalization |

## Resolved During Review

### Structured macro owner/value layout

The printer previously broke before a structured macro replacement based on its total compact width, top-level element count, or declaration-fragment class. These local decisions discarded legal layouts that attached a fitting replacement prefix and used ordinary opportunities deeper in the replacement.

Structured macro definitions now enter one solver model containing the header owner and the complete replacement value. The specification was subsequently made stricter: the attached form is legal only when the complete definition fits on one physical line. At `ColumnLimit: 22`, a call whose arguments require wrapping therefore starts after the definition header:

```cpp
#define VALUE \
    Build( \
        first, \
        second, \
        third \
    )
```

A short declaration replacement follows the same width-independent model and stays compact when that layout wins:

```cpp
#define D(v) void f(v)
```

The printer no longer annotates replacement widths or chooses replacement boundaries. Spec-mandated boundaries, including the header split for a non-fitting definition and boundaries between top-level call units, are structural constraints in the break model. The solver includes the two-column ` \` suffix in the overflow cost of every taken structured-macro break, so candidate cost reflects the emitted physical lines.

### Delimiter-stack partitioning

The delimiter-stack audit originally identified opener-run boundaries chosen by a current-column threshold and reconstructed by the printer. The initial examples did not prove a suboptimal result: when the greedy layout has zero overflow, delaying every boundary is a correct optimization because it minimizes the number of opener and closer runs.

A deeper-indentation counterexample did expose the missing case. Once delimiter lines overflowed, an additional run could reduce maximum overflow, which is more important than line count. The solver now uses the proven greedy fast paths only for zero-overflow layouts and detached layouts whose overflow is confined to the leaf, falls back to exact run-partition DP when delimiter overflow can change the result, records every selected run boundary in `FormatBreakSolution`, and makes the printer replay those choices. [break_solver.md](break_solver.md#delimiter-stacks) owns the optimization proof and fallback contract.

### Declaration groups

Groupable declaration-scope siblings now receive one structural classification: type, callable, object or field, or alias. The classification follows the declared entity through single-declaration wrappers and nested declarators, rather than relying on an exact wrapper kind or the first type-shaped child. Type declarations are isolated, different groups are separated, and consecutive fields or methods stay grouped. For example, omitted source whitespace no longer hides a field/method boundary:

```cpp
class Grouped {
    int field;

    void Method();
};
```

The layout-dependent rule uses the ordinary break solver rather than a width estimate. Moving only an initializer to a continuation line keeps adjacent fields together:

```cpp
class Grouped {
    VeryLongType wrapped =
        longValue;
    int next;
};
```

When the selected initializer or alias-target delimiter list closes at declaration indentation, that declaration is isolated from both neighbors:

```cpp
class Grouped {
    int before;

    Values values = {
        first,
        second
    };

    int after;
};
```

### Adjacent line-fragment strings

The forced boundary after an adjacent string fragment ending in source-spelled `\n` or `\r\n` is intentional. Such fragments conventionally encode authored lines of help, diagnostic, and similar text. Collapsing them onto one physical source line would discard useful visual structure, so [format.md](format.md#operator-chains) specifies this boundary as mandatory rather than offering it to the optimizer.

Ordinary adjacent strings remain a compact-or-split chain:

```cpp
const char* label = "first " "second";
```

Line-fragment text retains one fragment per physical source line:

```cpp
const char* help =
    "first line\n"
    "second line\n";
```

### Keyword-owned values

`return`, `co_return`, `throw`, and `co_yield` now share one structural keyword-owned-value category. The format model derives the category from the leading keyword, and the break model uses the same owner/value construction for statements and expressions. This removes the former exact-kind routing for only `ReturnStatement` and `CoReturnStatement`.

A forced adjacent-string sequence therefore detaches and aligns identically for every category member:

```cpp
throw
    "first line\n"
    "second line";

co_yield
    "first line\n"
    "second line";
```

### Compact single-statement lambdas

The compact single-statement lambda is an intentional expression-layout rule. Lambdas participate in expression contexts even though their bodies contain statements, so keeping a complete short lambda on one physical line can preserve the readability of the enclosing expression:

```cpp
auto increment = [](int value) { return value + 1; };
```

Ordinary function and standalone statement bodies remain expanded because they do not serve as inline expression operands:

```cpp
int Increment(int value) {
    return value + 1;
}
```

### Compact empty control bodies

Keeping an empty control body as `{}` is an intentional style rule; expanding an empty pair onto separate lines adds visual bulk without exposing any statement structure. A following attachment keyword begins a new line so the compact body finishes its own control-body line:

```cpp
if (ready) {}
else {}
```

The do-while form now follows the same rule instead of attaching its `while` as a one-off exception:

```cpp
do {}
while (running);
```

Non-empty bodies retain the ordinary block-attachment layout.

## Findings

### 1. Preprocessor blank lines depend on directive spellings

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

### 2. Enum trailing commas are suppressed by a macro-call shape guess

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

The remaining findings should be resolved by deleting each exception or replacing it with a small semantic category that applies at every recursion depth and in every equivalent context. Golden fixtures should demonstrate category boundaries rather than preserve one-off outputs.
