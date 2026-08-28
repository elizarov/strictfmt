# Source Formatting

This document specifies the source layout produced by `strictfmt`.

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
- Put no space between a literal and its user-defined literal suffix, e.g. `100ms` or `R"(value)"sv`.
- Put no padding before braced initializer braces, e.g. `std::string{}`.
- Put one space after commas and no space before commas, e.g. `a, b`.
- Put one space after non-empty `for` header semicolons. Put no space before semicolons. Keep `for (;;)` compact, e.g. `for (int i = 0; i < n; ++i)`.
- Put spaces around binary and ternary operators, e.g. `a + b`.
- Put no spaces around unary operators, e.g. `!ok`.
- Keep the reflection operator and splice delimiters tight, e.g. `^^T` and `value.[:member:]`.
- Put one space between a structured-binding pack ellipsis and its identifier, e.g. `[first, ... rest]`.
- Bind type declarator symbols to the type, e.g. `int* value`.
- Treat `operator` plus a following symbolic operator as one function name, e.g. `operator==(`.
- Put one space after `operator` for conversion, allocation, and deallocation operators, e.g. `operator bool(` and `operator new(`.
- Treat destructor `~` plus the following type name as one function name, e.g. `~Widget(`.
- Put no space between a C-style cast and the expression it prefixes, e.g. `(void)value`.
- Put spaces around range-for and constructor-initializer colons, e.g. `for (auto item : items)`.
- Put no space before access-specifier, label, or `case` colons, e.g. `public:`.
- Put no spaces around qualification or member-access operators, e.g. `std::string`.
- Put two spaces before a trailing `//` comment after code, e.g. `value;  // note`.
- Put no space between `#` and any preprocessor directive keyword, e.g. `#if` and `#include`.
- Put one space after a preprocessor directive keyword before its operand, e.g. `#pragma once`.

## Line Hygiene

- Remove trailing whitespace from every line.
- Preserve the source [line-ending style](glossary.md#line-ending-style). For mixed line endings, use the current platform default.
- Use spaces for indentation and never emit tabs. `IndentWidth` in [config.md](config.md) selects the number of spaces per indentation level.
- Preserve comments in source order. A trailing comment stays trailing only when it was trailing in source. A standalone comment stays standalone.
- Preserve a source blank-line separator between declarations or statements at the same structural level, collapsing each run to one line.
- Do not emit empty lines at the beginning or end of a file or block.
- Remove trailing commas except in enum bodies. When the final item is selected by a conditional preprocessor branch, remove the branch-owned terminal comma from every final branch as part of the same normalization.
- Apply the structured and raw replacement whitespace rules specified in [macro.md](macro.md).

## Mandatory Line Breaks

Mandatory line breaks are structural boundaries. The break is always taken before optional wrapping is considered.

- Break between complete statements and declarations, including after each statement-terminating semicolon. The only exception is an eligible compact single-statement lambda body described under [Lambdas](#lambdas).
- Put block-opening braces at the end of the introducing line, then break.
  - For every non-empty code block, regardless of its owner, the opener must visually separate a multiline introducing header from the block contents: if the header's last line is at the same indentation as the block contents, put `{` on its own line at the block owner's indentation. An owner-indented line containing only closing delimiters before `{`, such as `) {`, already provides that separation.
- Keep an empty code block as `{}` without a body break.
- Apply the closing-brace attachment rules under [Declaration And Control Headers](#declaration-and-control-headers).
- Break around preprocessor directives and apply the structured macro breaks and continuation lines specified in [macro.md](macro.md).
- Apply the mandatory separators specified under [Declaration Groups](#declaration-groups).

## Line Break Opportunities

Line break opportunities are optional boundaries that the break optimizer may take when formatting one formatted segment between mandatory line breaks. See **Break Selection** for the optimization objective and constraints.

- After assignment operators, qualification operators, and binary or ternary operators.
- After delimiter-group openers and before their matching closers.
- After commas in any [list](glossary.md#list).
- Between a declaration type and its direct-initialized declarator value.
- After semicolons inside control-statement headers.
- At callable-structure boundaries and between adjacent string literals.

## Qualified Names

A nested qualified name containing at least two non-leading `::` operators makes each of those operators an
after-operator break opportunity. A selected break keeps `::` on the owning line and puts its subordinate right
component one continuation indentation level deeper. A leading global-scope `::` is part of the first component and
is not a break opportunity. The scope portion of a pointer-to-member declarator, such as `Type::*`, is not a
qualified-name break opportunity. A single qualification, such as `std::string`, stays cohesive.

Qualification is left-associated for layout selection, independently of the parser's tree shape. The final `::` is
therefore the structurally shallowest qualification break. When overflow, physical-line count, and the other earlier
cost components are equal, the normal structural cost prefers a later `::`. Each qualification boundary makes an
independent compact-or-split choice, so an exceptionally long qualified name may take more than one break.

```cpp
void Convert(
    const experiments3::cargo_pricing_batched_order_route_price_correction::
        BatchedOrderRoutePriceCorrectionRequirementNames& value
);
```

## Indent Economy

Indent economy lets nested delimiter groups share one body indentation level when their opener and closer placement stays visually unambiguous. It applies to every broken delimiter group. It is a legality rule for candidate layouts; the break optimizer still chooses among legal layouts with the normal break-selection objective.

For any broken delimiter stack:

- The opening line ends with an opener or opener sequence, so the body break is immediately after an opener.
- If the opening line starts with an opener or opener sequence after indentation, the whole line contains only openers.
- The closing line starts with all closers for the stack combined. Syntax that belongs after the stack, such as `;`, `,`, another closer, or the next opener in a list boundary, may follow those closers.
- For every delimiter pair that selects broken layout, its closer starts a closing delimiter line. A closer for a broken pair must not be attached to body text; placements like `value)` are forbidden for that broken pair.
- Transparent single-item parentheses do not have separate placement rules. Each parenthesis pair independently selects compact or broken layout. A pair may stay compact only when the expression it encloses has no selected breaks. If the enclosed expression breaks, that pair is broken and follows the same opener and closer placement rules as any other broken delimiter group.

Newlines contained within a multiline literal token do not by themselves put an enclosing delimiter group into split form. When the literal lies in the otherwise compact tail of the final item, keep the group opener on its first physical line and the closer on its last physical line.

This allows wrapper and nested delimiter groups to share indentation:

<!-- .cpp-format
ColumnLimit: 26
-->
```cpp
auto r = render(transform(
    first,
    second
));
```

<!-- .cpp-format
ColumnLimit: 32
-->
```cpp
Widget rows[] = {{
    firstLongValue,
    secondLongValue
}};
```

A broken delimiter body puts a line-start opener on a separate line before item content:

<!-- .cpp-format
ColumnLimit: 25
-->
```cpp
POINT points[] = {
    {
        rect.left,
        rect.top
    }, {
        rect.right,
        rect.bottom
    }
};
```

Delimiter item boundaries may coalesce generically only for direct close-comma-open boundaries where both neighboring items use split delimiter expansions, such as `}, {` or `), (`. The boundary line starts with the combined closers for the previous item, then keeps the comma and the next item's opener before the next body break.

## Lists

A list uses compact or split form.

```cpp
auto compact = call(first, second, third);
```

<!-- .cpp-format
ColumnLimit: 30
-->
```cpp
auto split = call(
    first,
    second,
    third
);
```

Parenthesized comma expressions that represent list-like syntax, such as macro signature arguments, use the same single compact-or-split list decision.

Compact comma-separated lists may keep leading items on the opener line while the final item uses an indent-economy delimiter expansion. The final item may be any expression. If any earlier item splits, or if the final item only splits at an operator, the whole list uses split form.

A multi-item designated-initializer list may stay compact only when none of its items contains a selected line break; otherwise, the list uses split form.

A final lambda may use this exception only with an unbroken callable header; otherwise the list splits first. Outer item boundaries preserve more structure than breaks inside a callable header.

Angle-delimited lists do not use this exception; top-level type structure should stay visible.

<!-- .cpp-format
ColumnLimit: 60
-->
```cpp
auto result = call(first, second, [](int value) {
    return value + 1;
});
```

<!-- .cpp-format
ColumnLimit: 34
-->
```cpp
Widget rows[] = {
    {first, second},
    {third, fourth}
};

Widget rows[] = {
    {
        veryLongFirst,
        veryLongSecond
    }, {
        veryLongThird,
        veryLongFourth
    }
};
```

Enum bodies always split one enumerator per line. This layout and the enum exception under **Line Hygiene** are independent of the enumerator's internal expression shape, including macro-call syntax.

```cpp
enum class ValueFormat : std::uint8_t {
    String,
    Integer,
    FloatingPoint,
};
```

When a type-declaration body is followed by a declarator for the declared type, keep the declarator attached to the closing brace.

```cpp
struct Context {
    const Config* config = nullptr;
    size_t count = 0;
} context{&config, 0};
```

### Deliberate list expansion

A list that fits on one line may become dense even when a vertical layout would be easier to read. This is intentional, since the formatter minimizes the number of lines. When vertical layout is absolutely needed for readability, attach an end-of-line comment to one item. An end-of-line comment attached to any list element forces the list into split form. A source blank line or standalone comment between list elements also forces split form.

```cpp
auto result = update(
    first,
    second,  // keep vertical
    third
);
```

## Operator Chains

Binary chain operators are the operators whose usual source meaning is an associative, mostly commutative aggregation. Formatting them as a chain avoids implying that the first operand owns a subordinate "rest of expression" branch. Stream shifts and comma expressions are also chain-shaped because their syntax is a repeated separator sequence, but they do not use the associativity rationale.

- Binary chain operators are `+`, `*`, `&`, `|`, `^`, `&&`, and `||`.
- Comma is a chain operator only in comma expressions.
- Stream-shift chain operators are `<<` and `>>`.
- Member-call chain operators are `.` and `->`. Pointer-to-member operators `.*` and `->*` are not chain operators.
- Operators outside these chain-operator groups are ordinary operators. Examples include `==`, `-`, `/`, `%`, and comparisons.
- Chain classification is independent of operand count. A chain with two operands is still a chain.
- Chains use compact or split form.
- Compact chains may keep leading operands on one line while the final operand uses an indent-economy delimiter expansion.
- A final operand that only splits at another operator does not qualify for compact chain form.
- Split chains take every top-level chain opportunity.
- Chain parts use the chain item indentation, not an additional continuation indentation.
- If an outer context applies continuation indentation, that context defines the chain's base indentation.

<!-- .cpp-format
ColumnLimit: 35
-->
```cpp
int total = (
    firstLongValue +
    secondLongValue
);
```

<!-- .cpp-format
ColumnLimit: 36
-->
```cpp
bool ready = (
    firstCondition &&
    secondCondition
);
```

<!-- .cpp-format
ColumnLimit: 50
-->
```cpp
int total = first + second + BuildValue(
    firstLongArgument,
    secondLongArgument
);
```

- Logical chains split by `&&` or `||`.
- Inside `if` and `while`, split logical chain parts stay at condition indentation.
- Inside a split `for` header, a wrapped logical chain inside one semicolon part uses continuation indentation.
- Stream-shift chains split before `<<` or `>>`.
- A stream-shift chain may split once between the receiver and a compact shifted tail.
- If the compact shifted tail does not fit, each continued shift segment starts a continuation line.
- Configured stream methods bind to the following shifted value; no `<<` or `>>` break is taken between a configured manipulator sequence and that value.
- Member-call chains have three forms: compact form breaks before no `.` or `->`; receiver-separated form breaks once before the first member operator and keeps the member tail compact; split form breaks before every member operator.
- In compact and receiver-separated forms, every top-level member operator must occur on the same physical line. The receiver may expand before the first operator, and the final chain operand may expand after the last operator. An intermediate operand may not expand because that would place the operators before and after it on different lines.

<!-- .cpp-format
ColumnLimit: 60
-->
```cpp
auto expectation = EXPECT_CALL(mock, GetSize())
    .WillOnce(Return(0))
    .WillOnce(Return(1))
    .WillOnce(Return(0));
```

<!-- .cpp-format
ColumnLimit: 45
-->
```cpp
auto failure = internal::GetUnitTestImpl()
    ->current_test_result()
    ->HasNonfatalFailure();
```

<!-- .cpp-format
ColumnLimit: 90
-->
```cpp
auto x = RenderPoint{firstCoordinate + secondCoordinate + thirdCoordinate, y}
    .OffsetBy(deltaX, deltaY).x;
```

- Adjacent string literals are an implicit concatenation chain. Adjacent ordinary literals selected onto the same physical line are joined when allowed under [Token Preservation](#token-preservation). A line-fragment boundary is mandatory; other unsafe-to-join fragment boundaries may remain on one physical line.
- The boundary between a [value-owning keyword](glossary.md#value-owning-keyword) and its value is an ordinary break opportunity selected by the optimizer.
- When a forced multi-line string-fragment sequence stays split, it follows the same indentation rule as other chains. In single-value contexts, fragments align at the expression indentation. In list contexts, continued fragments use one continuation indentation level so the string chain stays visually separate from neighboring elements.
- Ternary chains are flat chains.
- A nested ternary chain either breaks after every `:` or stays compact.
- A single ternary may break after `?`, after `:`, after both, or inside either branch while keeping the selected branch attached to its `?` or `:` marker.

<!-- .cpp-format
ColumnLimit: 70
-->
```cpp
const char* key = firstCondition ? firstKey :
    secondCondition ? secondKey :
    fallbackKey;
```

- Ordinary binary operators use continuation indentation for the right operand when they split, including inside `(...)`.

<!-- .cpp-format
ColumnLimit: 95
-->
```cpp
bool installed = (
    RegEnumKeyExA(key, index, name, &nameLength, nullptr, nullptr, nullptr, nullptr) ==
        ERROR_SUCCESS
);
```

<!-- .cpp-format
ColumnLimit: 30
-->
```cpp
int ratio = (
    firstLongValue /
        secondLongValue
);
```

- Plain non-call parentheses form one expression delimiter group and add only body indentation. Nested lists and formatter-owned chain parts keep their elements at that body level.
- Assignment continuations are not flattened. A break after an assignment operator, including a designated-initializer assignment, or between a condition declaration's type/pointer prefix and its assigned declarator, adds one continuation indentation level.
- Nested ordinary binary operators still introduce continuation indentation.
- Unary operators and declarator `*` or `&` are token facts, not chain break points.
- An end-of-line comment attached to one chain part, or a standalone comment between chain links, forces the chain into split form. A standalone chain comment uses the chain-item indentation of the following link.

## Break Selection

For each formatted segment between mandatory line breaks, strictfmt chooses the legal layout with the best break-selection cost. Dynamic programming is used to find that layout.

A legal layout must satisfy all constraints in this document:

- Preserve source token order, supported comments, and the file line-ending style.
- Take all mandatory line breaks.
- Take optional line breaks only at listed line break opportunities.
- Obey all applicable structural rules.
- Obey indent-economy legality for delimiter groups.

Among legal layouts, the break optimizer chooses the layout with the best cost:

- Minimize the largest overflow beyond the configured column limit; layouts with no overflowing physical line have zero overflow.
- On equal maximum overflow, minimize the number of physical lines that overflow.
- On equal overflow cost, minimize the physical line count.
- On equal line count, prefer the structurally shallower deepest taken break.
- If common deeper breaks make those costs equal, minimize the sum of structural depths of all taken breaks so a distinguishing break closer to the expression root wins, then use source-order-stable compact behavior.

Function signatures may break after the complete return type before breaking inside the return type. The function name is indented one continuation level. Split parameters may keep the return type and function name together when that line fits.

<!-- .cpp-format
ColumnLimit: 80
-->
```cpp
std::vector<std::string>
    ParseItems(const std::vector<ConfigLine>& lines, size_t& index);
```

<!-- .cpp-format
ColumnLimit: 80
-->
```cpp
std::set<std::string> RequireSuffixGroup(
    const std::map<std::string, std::set<std::string>>& suffixGroups,
    std::string_view configPath,
    std::string_view groupName
) {
    return {};
}
```

<!-- .cpp-format
ColumnLimit: 40
-->
```cpp
auto result = render(
    first,
    transform(
        veryLongInputA,
        veryLongInputB
    ),
    third
);
```

Do not split inside [atomic groups](glossary.md#atomic-group).

Function-pointer aliases, typedefs, and declarations keep a space between the return type and a compact `(*)` declarator. Long forms may break at that return-type/declarator space before breaking inside the function-pointer declarator group.

```cpp
using AuthCheckerFactoryFactory = utils::UniqueRef<AuthCheckerFactoryBase> (*)(const components::ComponentContext&);

using GenericPrepareUnaryCall = std::unique_ptr<grpc::ClientAsyncResponseReader<grpc::ByteBuffer>>
    (grpc::GenericStub::*)(grpc::ClientContext*, const grpc::string&);
```

Defaulted, deleted, and pure-virtual method markers stay with the declaration tail.

## Declaration Groups

Declaration separation applies only in declaration scopes.

The declaration grouping kinds are type declarations, callable declarations or definitions, object or field declarations, and type aliases. Concept declarations map to the type-declaration group. Classification follows the declared entity rather than incidental type syntax: an elaborated type specifier with a declarator is an object declaration, while a callable that returns a function pointer is a callable declaration. A declaration wrapper, such as a template or friend declaration, has the kind of the single declaration it introduces. Access specifiers and leading standalone comments attach to the following member group.

Grouping determines where structural empty-line separators are required. It does not remove an existing source empty-line separator preserved by **Line Hygiene**.

- Separate top-level logical groups with one empty line.
- Keep consecutive forward type declarations in one group.
- Separate every other type declaration from its declaration siblings with one empty line.
- Separate neighboring declarations of different kinds with one empty line.
- Keep consecutive fields grouped when wrapping only moves an initializer to a continuation line.
- Separate an object declaration or type alias from neighbors when its solver-selected initializer or alias target needs more than one continuation line, regardless of the expression shape that creates those lines.
- Separate fields from neighboring methods with one empty line.
- Keep consecutive method declarations in one method group.
- A declaration inside a nested compound scope does not affect continuation-size isolation of the declaration item that owns that scope.

Access specifiers are class-level labels. Members under them stay one indent level deeper.

```cpp
class Widget {
public:
    void Paint();

private:
    int value;
};
```

Namespaces are grouping syntax, not indentation syntax. Declarations inside a namespace stay at the same indentation level as the namespace declaration. Separate the namespace opening line and closing brace from contained declarations with one empty line.

```cpp
namespace app {

void PaintWidget();

}
```

`extern "C"` linkage blocks follow the same indentation rule as namespace declarations. The linkage block does not consume an extra indent level, so declarations inside it stay at the enclosing declaration indentation. The conditional C++ guard form keeps the directives and linkage braces at column zero.

```cpp
#ifdef __cplusplus
extern "C" {
#endif

int RuntimeEntryPoint(int value);

#ifdef __cplusplus
}
#endif
```

## Declaration And Control Headers

Template prefixes are emitted before the introduced declaration, and the introduced declaration always starts on a separate physical line from the template prefix. A `requires` clause stays on the same line as `template <...>` only when the complete template prefix and compact clause fit on that line. Otherwise, including when the clause owns a forced break, the `requires` clause moves to a subordinate line and wraps structurally.

```cpp
template <typename T> requires(HasValue<T>)
void Use(T& value);

template <typename Callable>
    requires(
        !std::is_same_v<std::remove_cvref_t<Callable>, FunctionRef> &&
        std::is_invocable_r_v<Result, Callable&&, Args...>
    )
FunctionRef(Callable&& callable);
```

Concept definitions use the same assignment spacing as variable templates. A `requires` expression body formats like a block: the opening brace stays on the concept line, requirements are indented one level, and the closing brace is followed by the concept semicolon.

```cpp
template <typename T>
concept HasNonEmptyName = requires {
    requires !std::string_view{T::kName}.empty();
};
```

Constructor initializer lists use compact or split form. An `explicit` specifier stays attached to the constructor declarator. A long initializer list keeps `) :` on the header line, or `) noexcept :` when a trailing qualifier is present. Initializer count alone does not force the constructor parameter list to split.

<!-- .cpp-format
ColumnLimit: 80
-->
```cpp
Widget::Widget(int value) : value_(value) {}

DashboardApp::DashboardApp(
    const DiagnosticsOptions& diagnosticsOptions,
    bool bringToFrontOnRun
) :
    renderer_(trace_),
    diagnosticsOptions_(diagnosticsOptions),
    bringToFrontOnRun_(bringToFrontOnRun)
{
    renderer_.SetLiveAnimationEnabled(true);
}
```

Control-brace normalization makes every [control-statement](glossary.md#control-statement) body a braced block. It also emits an `else` block whose only statement is an `if` statement as a direct `else if` chain. An empty control body finishes its own control-body line before a following block-attachment keyword.

An attribute sequence between a control header and a compound statement belongs to that already-braced body. Keep the attributes between the header and `{` without adding a redundant outer block.

```cpp
void CheckValue(int value) {
    if (value < 0) [[unlikely]] {
        ReportFailure();
    }
    switch (value) [[likely]] {
        default:
            break;
    }
}
```

```cpp
void HandleState() {
    if (ready) {
        return;
    } else if (pending) {
        Queue();
    } else {
        Reset();
    }
}
```

Control headers use list layout. Headers with init-statements split at top-level semicolons before nested calls.

<!-- .cpp-format
ColumnLimit: 40
-->
```cpp
void RunItems() {
    for (
        int index = 0;
        index < limit;
        ++index
    ) {
        Run(index);
    }
}
```

<!-- .cpp-format
ColumnLimit: 80
-->
```cpp
void UseCurrent() {
    if (
        const auto current = FindCurrentValue(config);
        current.has_value() && *current != nullptr
    ) {
        Use(*current);
    }
}
```

Break after a non-empty statement or declaration block's closing brace. The attachment exceptions are `else`, `catch`, `finally`, and the `while` that closes a do-while statement.

```cpp
void RunSafely() {
    try {
        Run();
    } catch (const std::exception& exception) {
        Report(exception);
    } finally {
        Cleanup();
    }

    do {
        Poll();
    } while (running);
}
```

## Labels And Switches

Switch labels are inside the switch block. Statements under a `case` or `default` label are indented one level deeper. A scoped case keeps `{` on the label line and aligns `}` with the label.

```cpp
int Select(int value) {
    switch (value) {
        case 1:
            return one;
        case 2: {
            int local = two;
            return local;
        }
        default:
            return fallback;
    }
}
```

Nested switches restore the enclosing switch case indentation after the inner switch closes.

## Lambdas

Lambdas intentionally format like functions. The complete [callable header](glossary.md#callable-header) follows the same placement as a function header, and an owner prefix behaves like a function return-type prefix.

Single-statement lambda bodies may stay compact only when the complete lambda fits on one physical line. The compact form is limited to statements whose subtree contains no compound block, so a statement such as `if (condition) { work(); }` uses the same broken-body form as other block-bearing lambda bodies. If a lambda breaks anywhere, its body breaks after `{`, formats the body one indentation step deeper than the lambda header's render base, and aligns the closing brace with that base. Multi-statement lambda bodies always use that broken-body form.

Lambda captures are part of the callable prefix. Captures and parameters are separate break opportunities and use the same compact-or-split layouts as other delimiter groups. Breaks inside the callable prefix are structurally deeper than parameter-list breaks, so equal-cost wrapping splits parameters first.

<!-- .cpp-format
ColumnLimit: 72
-->
```cpp
const auto updateKey = [&](
    const std::string& sectionName,
    const std::string& key,
    const std::string& value
) {
    Update(sectionName, key, value);
};

const auto findValue =
    [&config](std::string_view name) -> std::optional<Value>
{
    return LookupValue(config, name);
};
```

## Include Sorting

Include sorting is enabled when include groups are configured. It only reorders `#include` lines within sortable include blocks, preserving the include set, spelling, and comments.

When include groups are absent, include sorting is disabled. Include directives are normalized and emitted in source order, and blank-separated include blocks are preserved.

Opening include blocks follow the same include run formatting path at file scope and inside `#ifndef`/`#define` guarded headers, so configured include groups control sorting or preservation for both forms. A blank line before an opening include block is source-authored rather than inferred from the spelling of the preceding directive.

Comments inside an include area bound the sortable include run around them.

Include blocks are regrouped by configured group priority, sorted case-insensitively inside each group, and separated by one empty line between groups.

```cpp
#include "package/source_file.h"

#include <winsock2.h>

#include <windows.h>

#include <algorithm>
#include <vector>

#include "vendor/package/header.h"

#include "package/other_header.h"
#include "util/text_format.h"
```

## Token Preservation

The formatter preserves source token order except for:

- include sorting when include groups are configured
- trailing-comma normalization
- control-brace normalization
- safe joining of adjacent ordinary string-literal tokens selected onto the same physical line

This joining exception removes the closing quote, inter-token whitespace, and next opening quote while preserving the combined literal's compatible encoding prefix and user-defined suffix. It applies only to ordinary literals in compact layout; raw literals and literals separated by a selected line break retain their token boundary. Joining is forbidden when the prefixes or suffixes are incompatible or when removing the seam could extend an escape at the end of the first body. In particular, a hexadecimal escape such as `\xB0` remains separated when the next body starts with a hexadecimal digit, and a one- or two-digit octal escape remains separated when the next body starts with an octal digit.

String literals ending with escaped `\n` or `\r\n` are line-fragment boundaries: they are not joined with the following literal, and their adjacent-literal boundary is mandatory. Other unsafe-to-join token-separated fragments may remain on one physical line.

Outside the listed changes, the formatter changes only spaces and line breaks.

## Further reading

- [config.md](config.md) specifies formatter configuration and ignore files.
- [glossary.md](glossary.md) defines shared terminology.
- [known_issues.md](known_issues.md) tracks known limitations and planned work.
- [macro.md](macro.md) specifies macro configuration and macro formatting.
- [preprocessor.md](preprocessor.md) describes handling of preprocessor directives and conditional compilation.
- [syntax_ambiguities.md](syntax_ambiguities.md) specifies the treatment of C++ syntax ambiguities.
