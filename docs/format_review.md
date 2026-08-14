# Format Generality Review

This document records a generality audit of [format.md](format.md) and the current formatter implementation. It is a review record, not a normative formatting specification. `format.md` continues to own intended output, and [break_solver.md](break_solver.md) continues to own solver constraints.

The review looked for:

- line-layout decisions made outside the dynamic-programming break solver
- forced breaks that are not broad structural rules
- behavior selected by an exact keyword, operator spelling, literal suffix, or syntax-node kind when equivalent syntax is treated differently
- rules whose result depends on input whitespace or recursion depth instead of the formatted structure

Configured macro categories, the documented distinction between chain operators and ordinary binary operators, mandatory adjacent-string line-fragment boundaries, compact single-statement lambdas, compact empty control bodies, and preprocessor pragma/undef groups are intentionally outside the findings. Those are supported semantic categories and readability, expression-layout, or source-grouping rules rather than accidental layout patches.

## Summary

No unresolved non-generic behavior remains from this audit.

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

### Preprocessor directive groups

Preprocessor blank-line insertion now uses two structural source-item groups. Consecutive `#pragma` directives form one pragma group, consecutive `#undef` directives form one undef group, and each group is separated from neighboring items of another kind. The pragma operand is irrelevant:

```cpp
#pragma once
#pragma warning(disable: 4100)

#include <vector>
```

Likewise, consecutive undefinitions stay together:

```cpp
#define FIRST 1

#undef FIRST
#undef SECOND

int value;
```

The printer no longer inserts blanks through a raw `#pragma once` prefix check or per-directive `#undef` branches. Opening include-run detection recognizes the exact `#pragma` directive kind, so includes following any pragma group retain normal sorting or preservation. Other preprocessor directives do not participate in this grouping rule.

### Enum trailing commas

Enum comma normalization now depends only on the structural list kind. The final syntactic item of every non-empty enum body receives a comma, regardless of whether that item is an ordinary enumerator or has a macro-call shape:

```cpp
enum class Plain {
    Default,
};

#define ENUM_STRING_DECLARE(EnumType, ItemsMacro) \
    enum class EnumType { \
        ItemsMacro(ENUM_STRING_DECLARE_ENUMERATOR), \
    };
```

The former identifier-plus-argument-list check guessed at the macro's expansion and made the result depend on parser wrapper shape. That check is gone. The formatter does not infer expansion contents; it applies the same enum-list invariant at every recursion depth.

## Overall Direction

All findings from this audit have been resolved. Future layout changes should continue to use structural categories at every recursion depth, with golden fixtures demonstrating category boundaries rather than preserving one-off outputs.
