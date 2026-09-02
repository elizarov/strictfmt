# Source Formatting

This document specifies the source layout produced by `strictfmt`. Wrapping examples use reduced column limits.

## Spacing Rules

- Put one space between a control keyword and `(`, e.g. `if (`.
- Put no space between the name and `(` in [call-like syntax](glossary.md#call-like-syntax), e.g. `Run(`.
- Put no padding inside compact [delimiter groups](glossary.md#delimiter-group), e.g. `call(value)`.
- Keep empty braces as `{}`, e.g. `return {};`.
- Put one space before a code-block `{`, e.g. `if (ok) {`.
- Put spaces around lambda trailing-return arrows, e.g. `[]() -> int`.
- Put one space before trailing function qualifiers, e.g. `Run() const`.
- Put one space between `template` and `<`, e.g. `template <typename T>`.
- Keep declaration modifiers compact and separate them from the modified type with one space, e.g. `alignas(8) int`.
- Separate a declaration type from its declarator with one space, e.g. `int value`.
- Put no space between a string or character literal prefix and the literal, e.g. `L"text"`.
- Put no space between a literal and its user-defined literal suffix, e.g. `100ms`.
- Put no padding before braced initializer braces, e.g. `std::string{}`.
- Put one space after commas and no space before commas, e.g. `a, b`.
- Put one space after non-empty `for` header semicolons. Put no space before semicolons. Keep `for (;;)` compact, e.g. `for (int i = 0; i < n; ++i)`.
- Put spaces around binary and ternary operators, e.g. `a + b`.
- Put no spaces around unary operators, e.g. `!ok`.
- Keep the reflection operator and splice delimiters tight, e.g. `value.[:member:]`.
- Put one space between a structured-binding pack ellipsis and its identifier, e.g. `[first, ... rest]`.
- Bind type declarator symbols to the type, e.g. `int* value`.
- Treat `operator` plus a following symbolic operator as one function name, e.g. `operator==(`.
- Put one space after `operator` for conversion, allocation, and deallocation operators, e.g. `operator bool(`.
- Treat destructor `~` plus the following type name as one function name, e.g. `~Widget(`.
- Put no space between a C-style cast and the expression it prefixes, e.g. `(void)value`.
- Put no space between a function-style fundamental type cast and `(`, e.g. `unsigned(value)`.
- Put spaces around range-for and list-prefix colons, e.g. `for (auto item : items)`.
- Put no space before access-specifier, label, or `case` colons, e.g. `public:`.
- Put no spaces around qualification or member-access operators, e.g. `std::string`.
- Put two spaces before a trailing `//` comment, e.g. `value;  // note`.
- Put one space before a trailing `/* ... */` comment, e.g. `Type& /*name*/`.
- Put no space between `#` and any preprocessor directive keyword, e.g. `#include`.
- Put one space after a preprocessor directive keyword before its operand, e.g. `#pragma once`.

## Vertical Alignment

Do not vertically align tokens across lines. As the sole exception, align a run of trailing `//` comments on consecutive lines in the same syntactic group when the aligned run fits within the line limit; otherwise use the ordinary two-space separation.

```cpp
struct Key {
    std::string shard;  // shard key
    int64_t id;         // primary key
};
```

## Line Hygiene

- Remove trailing whitespace from every line.
- Preserve the source [line-ending style](glossary.md#line-ending-style). For mixed line endings, use the current platform default.
- Use spaces for indentation and never emit tabs. `IndentWidth` in [config.md](config.md) selects the number of spaces per indentation level.
- Preserve comments in source order. A trailing comment stays trailing only when it was trailing in source. A standalone comment stays standalone.
- Preserve a source blank-line separator between declarations or statements at the same structural level, collapsing each run to one line.
- Do not emit empty lines at the beginning or end of a file or block.
- Apply the structured and raw replacement whitespace rules specified in [macro.md](macro.md).

## Mandatory Line Breaks

Mandatory line breaks are structural boundaries. The break is always taken before optional wrapping is considered.

- Break between complete statements and declarations, including after each statement-terminating semicolon, except inside a single-line function or lambda body.
- Remove structurally optional null declarations and statements, including first items in a scope, while preserving their comments. Null statements required as bodies remain subject to control-brace normalization.
- Put block-opening braces at the end of the introducing line, then break.
  - A trailing `/* ... */` header comment stays before the brace on that line.
  - For a non-empty block, if a multiline header ends at body indentation, put `{` on its own line at the block owner's indentation. A closing-delimiter line at owner indentation already provides this separation.
- Keep an empty code block as `{}` without a body break.
- Apply the closing-brace attachment rules under [Declaration And Control Headers](#declaration-and-control-headers).
- Break around preprocessor directives and apply the structured macro breaks and continuation lines specified in [macro.md](macro.md).
- Apply the mandatory separators specified under [Declaration Groups](#declaration-groups).

## Line Break Opportunities

Optional breaks within a [formatted segment](glossary.md#formatted-segment) occur:

- After assignment, binary, or ternary operators.
- After non-leading `::` in names. Qualification is left-associated; right components split first.
- After delimiter-group openers and before their matching closers.
- After a list's introducing `:`.
- After commas in any [list](glossary.md#list).
- Between a declaration type and its direct-initialized declarator value.
- After semicolons inside control-statement headers.
- At callable-structure boundaries and between adjacent string literals.
- After a [value-owning keyword](glossary.md#value-owning-keyword).

## Lists

Comma-separated delimiter groups, including list-like parenthesized comma expressions, use compact, packed split, or one-item-per-line layout. Both split forms break after the opener and before the closer, with items indented one level. Packed split keeps the items together on one line that fits.

<!-- .cpp-format
ColumnLimit: 34
-->
```cpp
void Example() {
    compact(first, second);
    packedSplit(
        firstValue, secondValue
    );
    oneItemPerLine(
        firstArgument,
        secondArgument
    );
}
```

Compact and packed lists may expand only the final item into multiple lines. In angle lists and multi-item designated initializers, multiline items require one-item-per-line layout. Nested delimiters follow [Indent Economy](#indent-economy).

<!-- .cpp-format
ColumnLimit: 35
-->
```cpp
auto result = call(first, Point{
    leftCoordinate, topCoordinate
});
```

Enum bodies always split one enumerator per line.

```cpp
enum class Mode {
    Read,
    Write,
};
```

## Indent Economy

Broken delimiter groups obey these rules, allowing nested groups to share one body indentation level:

- An opening line ends immediately after an opener. A line that starts with an opener contains only openers.
- Put combined nested closers at the start of their own line. Following syntax may stay attached.

<!-- .cpp-format
ColumnLimit: 26
-->
```cpp
auto r = render(transform(
    firstValue,
    secondValue
));
```

A direct close-comma-open boundary may share one line only when both neighboring items use split delimiter expansion.

<!-- .cpp-format
ColumnLimit: 25
-->
```cpp
Point points[] = {
    {
        rect.left,
        rect.top,
    }, {
        rect.right,
        rect.bottom,
    },
};
```

Intrinsic newlines in a literal do not force delimiter expansion. An otherwise compact final-item tail keeps its opener and closer attached to the literal's first and last lines.

```cpp
auto text = wrap(R"(first
second)");
```

## Operator Chains

Binary chain operators are the operators whose usual source meaning is a mostly associative aggregation or a a repeated separator sequence. Formatting them as a chain avoids implying that the first operand owns a subordinate "rest of expression" branch.

- Binary chains: `+`, `*`, `&`, `|`, `^`, `&&`/`and`, and `||`/`or`.
- Comma chains: commas in comma expressions.
- Stream chains: `<<` and `>>`.
- Member-call chains: `.` and `->`.
- Repeated call-application chains: an argument list applied to the result of another call.

Other operators, including `.*` and `->*`, are ordinary operators.

Ordinary binary and assignment breaks indent the right operand one level deeper, including inside parentheses and nested operators. This also applies to designated initializers and breaks between a condition declaration's type/pointer prefix and assigned declarator. Unary operators and declarator `*` or `&` do not introduce breaks.

<!-- .cpp-format
ColumnLimit: 28
-->
```cpp
bool same = firstValue ==
    secondValue;
```

Like [lists](#lists), chains stay compact on one line or split one item per line. Split items share the chain's base indentation, including any continuation indent from the surrounding context.

<!-- .cpp-format
ColumnLimit: 35
-->
```cpp
int total = (
    firstLongValue +
    secondLongValue
);
```

Compact chains may expand only the final operand. A break in an intermediate operand splits the whole chain.

<!-- .cpp-format
ColumnLimit: 50
-->
```cpp
int total = first + second + BuildValue(
    firstLongerArgument, secondLongerArgument
);
```

Logical chains break at `&&` or `||`. In `if` and `while`, parts stay at condition indentation; inside a split `for` header part, they use continuation indentation.

<!-- .cpp-format
ColumnLimit: 28
-->
```cpp
void Scan() {
    for (
        int i = 0;
        i < count &&
            IsReady(i);
        ++i
    ) {
        Visit(i);
    }
}
```

### Streams

Stream chains break before shift operators. A compact shifted tail may occupy one continuation line after the receiver. Otherwise each insertion starts a continuation line, except a string or character insertion binds to the following insertion when the pair fits, including punctuation attached by the enclosing expression. Configured manipulators bind to the following value.

<!-- .cpp-format
ColumnLimit: 45
StreamShift:
  ConfigurationMethods:
    - std::hex
-->
```cpp
void Print() {
    output
        << "first=" << std::hex << firstValue
        << ", second=" << secondValue;
}
```

### Member calls

Member calls have three forms: compact, receiver-separated with one break before the first member operator, or split before every member operator. In the first two forms, all top-level member operators share a physical line: only the receiver or final operand may expand.

<!-- .cpp-format
ColumnLimit: 32
-->
```cpp
auto result = Build(source)
    .Validate()
    .Transform()
    .Finish();
```

### Repeated call applications

A call whose result is immediately called again starts a repeated call-application chain. The first complete call is
the receiver; each following argument list is one chain item. An ordinary call with only one argument list is not a
chain.

Repeated call applications have three forms: compact, receiver-separated with one break before the first repeated
argument list, or split before every repeated argument list. In the first two forms, all repeated argument-list
openers share a physical line; only the receiver or final application may expand.

<!-- .cpp-format
ColumnLimit: 32
-->
```cpp
void Configure() {
    call(init)(next)(more);
}
```

<!-- .cpp-format
ColumnLimit: 23
-->
```cpp
void Configure() {
    call(initialValue)
        (next)(more);
}
```

<!-- .cpp-format
ColumnLimit: 20
-->
```cpp
void Configure() {
    call(init)
        (nextValue)
        (moreValue);
}
```

### Ternaries

Nested ternaries within one chain stay compact or break after every `:`, with flat indentation. A single ternary may break after `?`, `:`, both, or inside an attached branch.

<!-- .cpp-format
ColumnLimit: 45
-->
```cpp
auto key = firstCondition ? firstKey :
    secondCondition ? secondKey :
    fallbackKey;
```

### Parenthesized operator pieces

Parentheses always divide an operator chain into separately formatted pieces. This keeps explicit grouping visible rather than flattening it into the surrounding chain. Use parentheses when they expose structure beyond what the chain itself expresses. Plain non-call parentheses add one body-indentation level to their piece.

<!-- .cpp-format
ColumnLimit: 16
-->
```cpp
auto value =
    one + (
        two +
        three
    );
```

### String fragments

Adjacent strings form an implicit concatenation chain, subject to [Token Preservation](#token-preservation). Forced multiline fragments align at expression indentation in single-value contexts and one continuation level deeper in lists.

<!-- .cpp-format
ColumnLimit: 32
-->
```cpp
const char* messages[] = {
    "A longer message "
        "needs two lines.",
    "another message",
};
```

## Source-Controlled Expansion

A trailing comment on a list item or chain part, or a standalone comment between them, forces split form. A blank line between list items also forces split form. Standalone chain comments align with the following link.

Use end-of-line comments when expansion is essential for readability.

```cpp
auto result = call(
    first,  // keep vertical
    second
);
```

## Declaration Groups

In declaration scopes, group by declared entity: types (including concepts), callables, objects or fields, and type aliases. Declaration wrappers inherit the kind of the entity they introduce; type syntax does not change the kind.

- Separate different kinds, and each non-forward type declaration, with one empty line.
- Keep consecutive forward type declarations or callables together.
- Isolate an object or alias whose initializer or target takes more than one continuation line. A single continuation line does not separate fields; nested compound scopes do not affect this count.
- Access specifiers and leading standalone comments attach to the following group.

Access labels align with the class; members are indented one level.

```cpp
class Widget {
public:
    void Paint();

private:
    int value;
};
```

Namespaces add no indentation. Separate their contents from the opening and closing lines with one empty line. `extern "C"` blocks follow the same rule; conditional linkage guards and braces stay at column zero.

```cpp
namespace app {

void Paint();

}
```

## Declaration And Control Headers

Functions and lambdas share body layout: keep a single-statement body on the header line when the complete construct fits. Comments or statements containing a compound block prevent this form.

```cpp
int Next(int value) { return value + 1; }
```

When a qualified type and its declarator need separation, first break after the complete type, including declarator `*` or `&`, and indent the declarator one level. This boundary is preferred to every break inside the type, including template-list and non-leading `::` breaks. Type-internal breaks are available when the complete type does not fit by itself.

<!-- .cpp-format
ColumnLimit: 80
-->
```cpp
struct Dependencies {
    const ::loans::storages::CheckoutRemindersStorageComponent&
        checkout_reminders_storage_component_;
};
```

A function signature uses the same boundary between its return type and name. Split parameters may keep the return type and name together when the prefix fits.

<!-- .cpp-format
ColumnLimit: 40
-->
```cpp
Result
    Load(const Input& input, int mode);
```

Function-pointer aliases, typedefs, and declarations put a space between the return type and compact pointer declarator; long forms may break there before breaking inside the declarator.

```cpp
using Callback = Result (*)(const Request&);
```

Defaulted, deleted, and pure-virtual markers stay with the declaration tail. Member declarations use the same signature layout as free functions, including when the return type is unqualified; wrap the signature earlier when a marker such as `= 0` would exceed the line limit.

### Lambdas

Lambda [headers](glossary.md#callable-header) follow function-header placement; an owner prefix acts like a return-type prefix. Captures belong to the callable prefix and use [list layout](#lists), independently of parameters. When either break would suffice, split parameters before the prefix.

<!-- .cpp-format
ColumnLimit: 40
-->
```cpp
auto update = [context, options](
    First first, Second second
) {
    Update(first, second);
};
```

### Templates

A template prefix precedes the declaration on a separate line. Keep `requires` on the template line only when the complete prefix and compact clause fit; otherwise move it one indent deeper and wrap structurally. Constraint conjunctions and disjunctions use ordinary operator chains, so break after their logical operator before breaking inside either operand.

<!-- .cpp-format
ColumnLimit: 40
-->
```cpp
template <typename T>
    requires(Readable<T> && Writable<T>)
void Copy(T& value);
```

Concept definitions use assignment spacing, and `requires` expression bodies use block layout.

```cpp
template <typename T>
concept Sized = requires(T value) {
    value.size();
};
```

### Colon-prefixed lists

Comma-separated lists introduced by `:`, such as constructor initializers and base classes, use these layouts, in increasing line count:

- Compact: items follow `:` on the same line.
- Packed split: all items share one continuation line that fits.
- One item per line: each item starts on its own continuation line.

Both split forms indent items one level. Keep `:` attached to the preceding syntax. A comment between the preceding
syntax and `:` moves after the attached colon and before the first item. In constructors, keep `explicit` attached to
the declarator; initializer count alone does not force parameters to split.

<!-- .cpp-format
ColumnLimit: 48
-->
```cpp
Compact::Compact() : first_(1), second_(2) {}

struct PackedSplit :
    FirstLongBase, SecondLongBase {};

OnePerLine::OnePerLine() :
    firstLongValue_(1),
    secondLongValue_(2),
    thirdLongValue_(3) {}
```

### Control flow

Brace every [control-statement](glossary.md#control-statement) body. Collapse an `else` containing only an `if` into `else if`. An empty body ends its line before a following attachment keyword. Attributes between a header and an already-braced body stay there without another block.

```cpp
void Check() {
    if (ready) [[likely]] {
        Run();
    } else if (pending) {
        Wait();
    } else {
        Stop();
    }
}
```

Control headers use list layout, splitting at top-level semicolons before nested calls.

<!-- .cpp-format
ColumnLimit: 40
-->
```cpp
void Run() {
    if (
        auto item = FindItem(config);
        item && item->ready()
    ) {
        Use(item);
    }
}
```

Break after a non-empty statement or declaration block's closer unless followed by attached punctuation or `else`, `catch`, `finally`, or a do-while `while`.

```cpp
void PollUntilReady() {
    do {
        Poll();
    } while (!ready);
}
```

A declarator following its type-declaration body stays attached to the closing brace.

```cpp
struct Point {
    int x;
} origin{0};
```

## Labels And Switches

Switch labels are indented inside the switch, with statements one level deeper. A scoped case keeps `{` on the label line and aligns `}` with the label. Nested switches restore the enclosing case indentation.

```cpp
int Select(int value) {
    switch (value) {
        case 1: {
            int local = GetValue();
            return local;
        }
        default:
            return 0;
    }
}
```

## Break Selection

### Break Cost

An expansion's raw depth is its construct's structural depth in the formatted segment: zero at the root, increasing by one per nested level. The adjustments that turn raw depth into effective break cost are specified under [Break-Decision Trees](#break-decision-trees).

Each selected expansion with nonzero effective cost contributes one occurrence to the expansion-depth profile. Cost profiles compare greatest value first: at the greatest value whose occurrence count differs, prefer fewer occurrences. Independent parts combine by adding their counts.

### Optimization

Within each formatted segment, choose a layout satisfying all these rules, minimizing in order:

1. Overflow-size profile, with each overflowing physical line contributing its overflow beyond the column limit.
2. Expansion-depth profile.
3. Total physical line count.

Remaining ties prefer compact choices in source order.

#### Break-Decision Trees

The solver builds one tree per formatted segment. Raw depth starts at zero and increases at every construction layer. The dump retains tokens, decision nodes (`*`), and some grouping nodes; the trees below also show collapsed syntax layers (`-`) so raw depth can be counted.

##### Surcharges and Discounts

Surcharges and discounts express the relative cohesion of syntax constructs, so optimization favors the expected break locations.

Each adjustment changes only the effective costs of its specified decisions, never their indentation or permitted layouts. An adjustment applies recursively to a subtree where stated; a targeted adjustment changes only the named decisions.

`effective = raw + surcharge - discount`. A decision below is annotated `[raw + surcharge - discount = effective]`.

A **callable-prefix surcharge** is the smallest amount that places the prefix's shallowest decision one level deeper than its parameter-list decision. For the generic-return signature `Vector<int> func(int a);`, it makes the return type's internal choice deeper than the parameter-list choice:

```text
0 * FunctionSignature [0 + 0 - 0 = 0]
1 ├─ - return-type Sequence
2 │  ├─ - Token "Vector"
2 │  └─ - TemplateArgumentList syntax
3 │     └─ * Delimited(angle) "<int>" [3 + 2 - 0 = 5]
1 └─ - declarator branch
2    └─ - FunctionDeclarator Sequence
3       ├─ - Token "func"
3       └─ - ParameterList syntax
4          └─ * Delimited(paren) "(int a)" [4 + 0 - 0 = 4]
```

The angle list starts at raw depth 3 and the parameter list at 4. A surcharge of 2 moves the prefix's shallowest decision one level below the parameter decision, giving the angle list effective cost 5.

A **name-prefix surcharge** similarly places name-internal decisions one level deeper than the attached list; chain links retain their expression depth. In the statement segment `make<int>(value);`, both lists start at raw depth 2:

```text
0 - expression Sequence
1 ├─ - Token "make"
1 ├─ - TemplateArgumentList syntax
2 │  └─ * Delimited(angle) "<int>" [2 + 1 - 0 = 3]
1 └─ - ArgumentList syntax
2    └─ * Delimited(paren) "(value)" [2 + 0 - 0 = 2]
```

A **final-lambda discount** equals the body's current cost and applies to the body subtree; the header remains outside it. In `visit(items, [] { return Serialize(); });`:

```text
 0 - expression Sequence
 1 ├─ - Token "visit"
 1 └─ - ArgumentList syntax
 2    └─ * Delimited(paren) outer arguments [2 + 0 - 0 = 2]
 3       ├─ - first-item construction
 4       │  └─ - Token "items"
 3       └─ - final-item construction
 4          └─ * BodyHeader(lambda) [4 + 0 - 0 = 4]
 5             ├─ - header branch
 6             │  └─ - LambdaCaptureSpecifier syntax
 7             │     └─ * Delimited(bracket) capture [7 + 0 - 0 = 7]
 5             └─ - body branch
 6                └─ - CompoundStatement syntax
 7                   └─ * Delimited(brace) body [7 + 0 - 7 = 0]
 8                      └─ - body-item construction
 9                         └─ - ReturnStatement syntax
10                            └─ * Chain(return value) [10 + 0 - 7 = 3]
11                               └─ - ArgumentList syntax
12                                  └─ * Delimited(paren) call arguments [12 + 0 - 7 = 5]
```

## Include Sorting

With configured include groups, sort each include run lexicographically and case-sensitively within groups, order groups by priority, and separate them with one empty line. Preserve the include set, spelling, and comments; comments bound sortable runs.

Without groups, retain source order and blank-separated blocks. These rules also apply inside header guards. Blank lines before include blocks remain source-authored.

```cpp
#include <algorithm>
#include <vector>

#include "widget.h"
```

## Comma Normalization

For every non-empty comma-separated list inside `{ ... }`, omit the trailing comma in compact and packed layouts and add it in one-item-per-line layout. Remove trailing commas from lists with other delimiters. When conditional preprocessing selects the final list item, apply the same rule to every possible final branch.

<!-- .cpp-format
ColumnLimit: 30
-->
```cpp
auto values = Values{
    firstLongValue,
    secondLongValue,
};
```

## Token Preservation

Only spaces and line breaks change, except for:

- [Include sorting](#include-sorting).
- [Comma normalization](#comma-normalization).
- Control-brace normalization.
- Removal of optional null declarations and statements.
- Safe joining of adjacent ordinary string literals on the same physical line.

Join ordinary literals only when their encoding prefixes and suffixes are compatible and joining cannot extend an escape. Preserve the compatible prefix and suffix. Raw literals and literals separated by a selected break retain their boundaries. For example, `"\x1" "a"` stays separate.

A literal ending in escaped `\n` or `\r\n` forces a break before the next literal. Other unsafe-to-join fragments may share a line.

## Further reading

- [config.md](config.md) specifies formatter configuration and ignore files.
- [glossary.md](glossary.md) defines shared terminology.
- [known_issues.md](known_issues.md) tracks known limitations and planned work.
- [macro.md](macro.md) specifies macro configuration and macro formatting.
- [preprocessor.md](preprocessor.md) describes handling of preprocessor directives and conditional compilation.
- [syntax_ambiguities.md](syntax_ambiguities.md) specifies the treatment of C++ syntax ambiguities.
