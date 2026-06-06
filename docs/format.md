# Source Formatting

This document specifies the source layout produced by `strictfmt`.

## Spacing Rules

- Put one space between a control keyword and `(`, e.g. `if (`.
- Put no space between a call, macro-like call, declaration name, constructor, destructor, or operator name and `(`, e.g. `Run(`.
- Put no padding inside parentheses, brackets, template angle brackets, or one-line initializer braces, e.g. `call(value)`.
- Keep empty braces as `{}`, e.g. `return {};`.
- Put one space before a code-block `{`, e.g. `if (ok) {`.
- Put spaces around lambda trailing-return arrows, e.g. `[]() -> int`.
- Put one space before trailing function qualifiers, e.g. `Run() const`.
- Put one space between `template` and `<`, e.g. `template <typename T>`.
- Keep declaration modifiers compact and separate them from the modified type with one space, e.g. `alignas(8) int`.
- Separate a declaration type from its declarator with one space, e.g. `int value`.
- Put no space between a string or character literal prefix and the literal, e.g. `L"text"`.
- Put no space between a numeric literal and its user-defined literal suffix, e.g. `100ms`.
- Put no padding before braced initializer braces, e.g. `std::string{}`.
- Put one space after commas and no space before commas, e.g. `a, b`.
- Put one space after non-empty `for` header semicolons. Put no space before semicolons. Keep `for (;;)` compact, e.g. `for (int i = 0; i < n; ++i)`.
- Put spaces around binary and ternary operators, e.g. `a + b`.
- Put no spaces around unary operators, e.g. `!ok`.
- Bind type declarator symbols to the type, e.g. `int* value`.
- Treat `operator` plus a following symbolic operator as one function name, e.g. `operator==(`.
- Put one space after `operator` for conversion, allocation, and deallocation operators, e.g. `operator bool(` and `operator new(`.
- Treat destructor `~` plus the following type name as one function name, e.g. `~Widget(`.
- Put no space between a C-style cast and the expression it prefixes, e.g. `(void)value`.
- Put spaces around range-for and constructor-initializer colons, e.g. `for (auto item : items)`.
- Put no space before access-specifier, label, or `case` colons, e.g. `public:`.
- Put no spaces around namespace, member-access, or pointer-member-access operators, e.g. `std::string`.
- Put two spaces before a trailing `//` comment after code, e.g. `value;  // note`.
- Put one space after a preprocessor directive keyword before its operand, e.g. `#pragma once`.

## Parenthesized Initializer Ambiguity

C++ block-scope declarations can use a token shape that is both a parenthesized direct initializer and a function declaration without type information.

```cpp
int product(a * b, c & d);
ResultType local(FirstType* first, SecondType& second);
```

The formatter has no symbol table and follows the parser's declaration interpretation for that ambiguous shape. Parenthesized initialization is rare in project code, so the formatter keeps the generic declaration rule and leaves source disambiguation to the author. Prefer braced initialization by default. Use parenthesized initialization only when braces have different C++ semantics, such as vector length construction:

```cpp
std::vector<int> values(count);
std::vector<int> oneValue{count};
```

When a parenthesized direct initializer needs expression operands that could parse as declarators, add extra parentheses around those operands. This is the preferred fix:

```cpp
int product((a * b), (c & d));
```

## Expression Template Ambiguity

The formatter has no symbol table and cannot distinguish every expression from every template-id. It follows deterministic syntax-only parser behavior:

- Callable template-id shapes parse as template calls: `name<args>(...)`, `qualified::name<args>(...)`, and nested template arguments.
- Relational chains that do not form a callable template-id parse as expressions, e.g. `value < min || value > max` and `a < b > c`.
- Template argument lists prefer type-like arguments when a name could be either a type or a value.

If a non-type template argument expression can parse as a type-like argument, parenthesize the value argument. For example, `Size(A*B)` inside a template argument list parses as a type-like function declarator and formats with type-declarator spacing. Write the value expression as:

```cpp
using X = Box<(Size(A * B))>;
```

Parenthesize expression chains that look like callable template-ids:

```cpp
return (a < b) > (c);
```

Without those parentheses, `a < b > (c)` is parsed as the template call `a<b>(c)`.

## Line Hygiene

- Remove trailing whitespace from every line.
- Preserve LF, CRLF, or CR line endings when the source uses one style. For mixed line endings, use the current platform default.
- Use 4 spaces per indent level. Do not emit tabs.
- Preserve comments in source order. A trailing comment stays trailing only when it was trailing in source. A standalone comment stays standalone.
- Preserve one source blank line when it separates already closed declarations or statements at the same structural level. Collapse multiple blank lines to one.
- Treat empty lines only as separators between neighboring source items. Do not emit empty lines at the beginning or end of a file.
- Drop blank lines at the beginning of a block and immediately before a closing brace.
- Insert required structural blank lines only when they separate neighboring source items.
- Remove trailing commas except in enum bodies.

## Mandatory Line Breaks

Mandatory line breaks are structural boundaries. The break is always taken before optional wrapping is considered.

- Break between complete statements and declarations, including after each statement-terminating semicolon.
- The single-statement lambda is an exception only when the whole lambda stays on one physical line.
- Put block-opening braces at the end of the introducing line, then break.
- Break after a code-block closing brace unless the following token is `else`, `catch`, `finally`, or the `while` that closes a do-while statement.
- Treat a standalone braced statement block as a block. Its closing brace does not attach to the following statement.
- Break around preprocessor directives and macro continuation lines.
- Break between enum enumerators; enum bodies keep one enumerator per line.
- Break between declaration groups where declaration-scope separation rules require a blank line.
- Break multi-statement lambda bodies after `{`, format each body statement with normal mandatory statement breaks, and put the closing `}` on its own line.
- Preserve a standalone line comment on its own line.

```cpp
if (ready) {
    return;
} else {
    Reset();
}

do {
    Poll();
} while (running);

{
    const Lock lock(mutex);
    value = next;
}

if (value != next) {
    Update(value);
}
```

## Line Break Opportunities

Line break opportunities are optional boundaries that the optimizer may take when formatting one segment between mandatory breaks. The formatter sends these opportunities to a dynamic-programming optimization solver; see **Break Selection Algorithm** for the solver objective and tie-break rules.

- After assignment operators and after binary or ternary operators.
- After delimiter openers and before matching closers for `()`, `[]`, `{}`, and template `<>`.
- After commas in lists, including call arguments, declaration parameters, template arguments, braced initializer elements, subscript lists, base-class lists, and enum bodies.
- Between a declaration type and its direct-initialized declarator value, whether the initializer is braced or parenthesized and whether the declaration is local or a field.
- After semicolons inside `for` and control headers.
- Around lambda captures, lambda parameter lists, lambda bodies, constructor initializer lists, and adjacent string literal sequences.

## Indent Economy

Indent economy lets nested delimiter groups share one body indentation level when their opener and closer placement stays visually unambiguous. It applies to broken `()`, `[]`, `{}`, and parsed template `<>` delimiter groups. It is a legality rule for candidate layouts; the optimizer still chooses among legal layouts with the normal dynamic-programming objective.

For any broken delimiter stack:

- The opening line ends with an opener or opener sequence, so the body break is immediately after an opener.
- If the opening line starts with an opener or opener sequence after indentation, the whole line contains only openers.
- The closing line starts with all closers for the stack combined. Syntax that belongs after the stack, such as `;`, `,`, another closer, or the next opener in a list boundary, may follow those closers.

This allows wrapper and nested delimiter groups to share indentation:

```cpp
render(transform(
    first,
    second
));

Widget rows[] = {{
    first,
    second
}};
```

A broken delimiter body cannot put item content after a line-start opener:

```cpp
POINT points[] = {
    {rect.left,
        rect.top
    }
};
```

The formatter produces a separate opener line instead:

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

Lists use compact or split form.

```cpp
call(first, second, third);

call(
    first,
    second,
    third
);
```

The rule applies to function arguments, template arguments, braced initializer lists, subscript lists, declaration parameter lists, base-class lists, enum bodies, and similar comma-separated syntax.

Compact comma-separated lists may keep leading items on the opener line while the final item uses an indent-economy delimiter expansion. The final item may be any expression, such as a braced initializer or call. If any earlier item splits, or if the final item only splits at an operator, the whole list uses split form.

```cpp
call(first, second, [](int value) {
    return value + 1;
});
```

When a template list wraps, `<` stays with the owner and each top-level argument occupies one line.

Nested braced initializer and braced constructor elements are independent structural parts. Each nested element uses the same compact-or-split optimization as any other segment and follows indent-economy delimiter placement.

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

Enum bodies always split one enumerator per line and keep a trailing comma on the final enumerator.

```cpp
enum class ValueFormat : std::uint8_t {
    String,
    Integer,
    FloatingPoint,
};
```

When a class, struct, or enum body is followed by a declarator for the declared type, keep the declarator attached to the closing brace.

```cpp
struct Context {
    const Config* config = nullptr;
    size_t count = 0;
} context{&config, 0};
```

## Operator Chains

Binary chain operators are the operators whose usual source meaning is an associative, mostly commutative aggregation. Formatting them as a chain avoids implying that the first operand owns a subordinate "rest of expression" branch. Stream shifts and comma expressions are also chain-shaped because their syntax is a repeated separator sequence, but they do not use the associativity rationale.

- Chain operators are token-class defined.
- Binary chain operators are `+`, `*`, `&`, `|`, `^`, `&&`, and `||`.
- Comma is a chain operator only in comma expressions.
- Stream-shift chain operators are `<<` and `>>`.
- Operators outside the chain-operator token class are ordinary operators. Examples include `==`, `-`, `/`, `%`, and comparisons.
- Chain classification is independent of operand count. A chain with two operands is still a chain.
- Chains use compact or split form.
- Compact chains may keep leading operands on one line while the final operand uses an indent-economy delimiter expansion.
- A final operand that only splits at another operator does not qualify for compact chain form.
- Split chains take every top-level chain opportunity.
- Chain parts use the chain item indentation, not an additional continuation indentation.
- If an outer context applies continuation indentation, that context defines the chain's base indentation.

```cpp
int total = (
    firstLongValue +
    secondLongValue
);

bool ready = (
    firstCondition &&
    secondCondition
);

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
- Configured stream methods bind to the following shifted value; no `<<` or `>>` break is taken between a configured manipulator run and that value.
- Adjacent string literals are an implicit concatenation chain.
- When a call argument string sequence stays split, the first literal uses expression indentation and later fragments use one additional indent.
- When a forced multi-line string-fragment sequence is the direct initializer in an assignment or declaration, the assignment breaks first and all fragments align at the assignment continuation indentation.
- Line-fragment strings ending with escaped `\n` or `\r\n` stay physically split.
- A boundary such as `"\xB0" "C"` stays token-separated to preserve escape parsing, but it may remain on one physical line when the adjacent-literal chain fits.
- Ternary chains are flat chains.
- A nested ternary chain either breaks after every `:` or stays compact.
- A single ternary may break after `?`, after `:`, after both, or inside either branch while keeping the selected branch attached to its `?` or `:` marker.

```cpp
const char* key = firstCondition ? firstKey :
    secondCondition ? secondKey :
    fallbackKey;
```

- Ordinary binary operators use continuation indentation for the right operand when they split, including inside `(...)`.

```cpp
bool installed = (
    RegEnumKeyExA(key, index, name, &nameLength, nullptr, nullptr, nullptr, nullptr) ==
        ERROR_SUCCESS
);

int ratio = (
    firstLongValue /
        secondLongValue
);
```

- Plain non-call parentheses contain one expression group.
- A plain non-call parenthesis group adds only body indentation.
- Lists and formatter-owned chain parts inside that group keep their elements at that body level.
- Nested ordinary binary operators still introduce continuation indentation.
- Unary operators and declarator `*` or `&` are token facts, not chain break points.
- An end-of-line comment attached to one chain part forces the chain into split form.

## Break Selection Algorithm

For each parsed segment between mandatory line breaks, the formatter uses a tree-sitter-derived layout model built directly from the parsed syntax tree. Tree nodes represent text leaves, sequences, delimiter groups, lists, operator structures, adjacent string literal sequences, lambda headers and bodies, and comments. The formatter rejects inputs whose tree-sitter parse contains errors or missing nodes; formatter-supported syntax is added to the grammar instead of falling back to token-span recovery.

Each node exposes its legal compact and split layouts. The optimizer chooses which optional breaks to take with dynamic programming:

- Minimize the largest overflow beyond the configured column limit; layouts with no overflowing physical line have zero overflow.
- On equal maximum overflow, minimize the number of physical lines that overflow.
- On equal overflow cost, minimize the physical line count.
- On equal line count, prefer the layout whose deepest taken break renders at the shallower indentation level; if still tied, prefer the structurally shallower deepest taken break, then source-order-stable compact behavior.

The optimizer treats the column limit as bounded input and caches each subproblem by node and normalized layout context, including indentation, prefix, suffix, and continuation mode.

Delimiter-group legality is defined by indent economy. The legality rules restrict candidate layouts without forcing a local break choice; the optimizer chooses among the legal compact, split, and indent-economy layouts.

Function signatures may break after the complete return type before breaking inside the return type. The function name is indented one continuation level. Split parameters may keep the return type and function name together when that line fits. Functions and lambdas deliberately share one callable-header model. Function definitions whose return-type prefix is split away from the function name start the body `{` on its own line at declaration indentation. Assigned lambdas whose assignment prefix is split away from the lambda header expose both attached and declaration-indented body-header layouts to the optimizer. A callable whose only header continuation is a split parameter list must keep `) {` together.

```cpp
std::vector<std::string>
    ParseItems(const std::vector<ConfigLine>& lines, size_t& index);

std::vector<std::string>
    ParseItems(
        const std::vector<ConfigLine>& lines,
        size_t& index
    )
{
    return {};
}

std::set<std::string> RequireSuffixGroup(
    const std::map<std::string, std::set<std::string>>& suffixGroups,
    std::string_view configPath,
    std::string_view groupName
) {
    return {};
}

render(
    first,
    transform(
        veryLongInputA,
        veryLongInputB
    ),
    third
);
```

Do not split inside empty delimiter pairs, function-pointer declarator groups, parenthesized callees, compiler declaration prefix groups, `__declspec` groups, operator function names, or template-angle tokens that are not template argument lists.

Function-pointer aliases keep a space between the return type and a compact `(*)` declarator. Long aliases may break at that return-type/declarator space before breaking inside the function-pointer declarator group.

```cpp
using AuthCheckerFactoryFactory = utils::UniqueRef<AuthCheckerFactoryBase> (*)(const components::ComponentContext&);

using GenericPrepareUnaryCall = std::unique_ptr<grpc::ClientAsyncResponseReader<grpc::ByteBuffer>>
    (grpc::GenericStub::*)(grpc::ClientContext*, const grpc::string&);
```

Defaulted, deleted, and pure-virtual method markers stay with the declaration tail.

An end-of-line comment attached to one list element forces the owning list into split form. A source blank line or standalone comment between list elements also forces the owning list into split form. Lists still split all top-level comma opportunities together, and a single empty line directly before or after a standalone list comment is preserved.

```cpp
update(
    first,
    second,  // note
    third
);
```

## Declaration Groups

Declaration separation applies only in declaration scopes.

- Separate top-level logical groups with one empty line.
- Separate neighboring type declarations from siblings with one empty line.
- Separate neighboring declarations of different kinds with one empty line.
- Keep consecutive fields grouped when wrapping only moves an initializer to a continuation line.
- Separate a field or type alias from neighbors when its initializer or alias target owns a multi-line delimiter list whose closing delimiter returns to declaration indentation.
- Separate fields from neighboring methods with one empty line.
- Keep consecutive method declarations in one method group.

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

class Widget {
public:
    void Paint();
};

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

Function and method declarations use list layout for parameters.

```cpp
void Func(int x) {
    Run(x);
}

void FuncLong(
    LongTypeA veryLongA,
    LongTypeB veryLongB
) {
    Run(veryLongA, veryLongB);
}
```

Template prefixes are emitted before the introduced declaration. A `requires` clause stays on the same line as `template <...>` only when the complete template prefix and compact clause fit on that line. Otherwise, including when the clause owns a forced break, the `requires` clause moves to a subordinate line and wraps structurally.

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

Constructor initializer lists use compact or split form. A long initializer list keeps `) :` on the header line, or `) noexcept :` when a trailing qualifier is present. Initializer count alone does not force the constructor parameter list to split. Non-empty bodies keep the opening body brace with the initializer list when the complete line fits; otherwise the brace moves to its own line after the initializer list. Empty bodies keep `{}` compact.

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

Control-brace normalization makes every `if`, `else`, `for`, `while`, `do`, and `switch` body a braced block. It also emits an `else` block whose only statement is an `if` statement as a direct `else if` chain. Compact empty control bodies stay `{}` and finish their own control-body line before a following block-attachment keyword.

```cpp
if (ready) {
    return;
} else if (pending) {
    Queue();
} else {
    Reset();
}
```

Control headers use list layout. Headers with init-statements split at top-level semicolons before nested calls.

```cpp
for (
    int index = 0;
    index < limit;
    ++index
) {
    Run(index);
}

if (
    const auto current = FindCurrentValue(config);
    current.has_value() && *current != nullptr
) {
    Use(*current);
}
```

`else`, `catch`, `finally`, and do-while `while` attach to the preceding closing block brace.

```cpp
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
```

## Labels And Switches

Switch labels are inside the switch block. Statements under a `case` or `default` label are indented one level deeper. A scoped case keeps `{` on the label line and aligns `}` with the label.

```cpp
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
```

Nested switches restore the enclosing switch case indentation after the inner switch closes.

## Lambdas

Lambdas intentionally format like functions. A lambda is a callable for all header/body placement decisions: the capture list, parameter list, and optional trailing return type form the callable header, and an assignment prefix such as `const auto name =` behaves like a function return-type prefix.

Single-statement lambda bodies may keep their braces and statement compact only when the complete lambda capture, parameter list, optional trailing return type, and body fit on one physical line. The compact single-statement form is limited to statements whose subtree contains no compound block, so a statement such as `if (condition) { work(); }` uses the same broken-body form as other block-bearing lambda bodies. If a lambda breaks anywhere, its body breaks after `{`, formats the body one indentation step deeper than the declaration, and closes on its own line. Multi-statement lambda bodies always use that broken-body form. Any delimited list containing a broken lambda body splits like any other list containing a multi-line item.

When an assigned lambda keeps the assignment prefix and lambda header together, a split parameter list must keep the body opener attached as `) {`, matching function definitions whose only header continuation is a split parameter list.

Lambda captures and lambda parameters are separate break opportunities. Captures and parameters use the same compact-or-split optimization as other delimiter groups.

When an assigned lambda splits the assignment prefix away from the lambda header and the lambda body is broken, the body opener starts on its own line at declaration indentation so body statements are one indentation step deeper than the declaration.

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

Multi-parameter lambda parameter lists and capture lists split all-or-nothing.

## Preprocessor And Macros

Put one empty line after `#pragma once` when another source item follows. Put one empty line before and after each `#undef` when it separates `#undef` from a neighboring source item.

Macro continuation backslashes, spaces before continuation backslashes, and continuation newlines are formatter-owned. A multi-line macro definition is parsed as one replacement list, then emitted with continuation backslashes on all continued macro lines.

Macro replacement lists that form declaration fragments are recursively formatted before continuation backslashes are added.

Macro category roles and runtime scanner ownership are described in [Formatter Configuration](#formatter-configuration).

```cpp
#define CASEDASH_METRIC_DISPLAY_STYLE_ITEMS(X) \
    X(Scalar, "scalar") \
    X(Percent, "percent") \
    X(Memory, "memory")
```

## Conditional Compilation And Local Includes

Conditional compilation is accepted when each branch contributes complete grammar items at the surrounding level: complete declarations, complete statements, field or method declarations, enum entries, macro definitions, includes, or similar syntax that already has a mandatory structural line break. The conditional directive lines stay at column zero, and the guarded code keeps the indentation it would have at that source location.

Conditional compilation may also patch complete items in comma-separated lists. This is accepted for function arguments, braced initializer items, subscript items, declaration parameters, template parameters, and enum entries. A conditional in one of these lists makes the guarded item use split-list indentation: directive lines stay at column zero, and branch items are indented as list items. Conditional list items use the same comma normalization as ordinary list items, so final items lose trailing commas except in enum bodies.

```cpp
void NormalizeSocketFlags(int& flags) {
#ifdef SOCK_CLOEXEC
    flags &= ~SOCK_CLOEXEC;
#endif
}

struct ConnectionOptions {
#ifdef FORMAT_USERVER_HAS_SOCKET_MARK
    int socket_mark = 0;
#endif

#ifndef FORMAT_USERVER_DISABLE_TLS
    void EnableTls();
#endif
};

#if FORMAT_USERVER_LEGACY_FMT
#define FORMAT_USERVER_CONST
#else
#define FORMAT_USERVER_CONST const
#endif
```

```cpp
std::vector<std::string> list{
    "one",
    "two",
#if MORE
    "three",
#endif
#if EVEN_MORE
    "four"
#endif
};
```

Conditional compilation is not accepted in base-class lists or constructor initializer lists. Those lists do not have a stable trailing-comma form in the supported style, so patching items there remains an unsupported preprocessor placement.

Local `#include` directives follow the same boundary rule: they may stand where the surrounding grammar accepts a complete declaration, statement, member declaration, enum entry, or directive, but not inside another expression or declaration. The include directive line stays at column zero.

```cpp
void RegisterGeneratedMetrics() {
#include "generated_metrics.inc"
    CommitGeneratedMetrics();
}
```

Conditional directives are rejected below the complete-item or list-item boundary, such as inside an expression, declaration, or statement header. The formatter reports every offending `#if`, `#ifdef`, `#ifndef`, or `#include` line as `unsupported preprocessor placement`.

Do not patch one operand into an expression:

```cpp
constexpr int kOptmask =
    ARES_OPT_FLAGS | ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES | ARES_OPT_DOMAINS |
#if ARES_VERSION < 0x011400
    ARES_OPT_SOCK_STATE_CB |
#endif
    ARES_OPT_LOOKUPS;
```

Prefer a separately declared compatibility constant, or a conditionally declared macro when the surrounding language position cannot name a constant.

```cpp
#if ARES_VERSION < 0x011400
constexpr int kAresOptSockStateCbCompat = ARES_OPT_SOCK_STATE_CB;
#else
constexpr int kAresOptSockStateCbCompat = 0;
#endif

constexpr int kOptmask =
    ARES_OPT_FLAGS |
    ARES_OPT_TIMEOUTMS |
    ARES_OPT_TRIES |
    ARES_OPT_DOMAINS |
    kAresOptSockStateCbCompat |
    ARES_OPT_LOOKUPS;
```

Do not patch declaration modifiers directly into the declaration:

```cpp
#ifndef FORMAT_USERVER_CLANG
[[gnu::visibility("default")]] [[gnu::externally_visible]]
#endif
int FormatUserverExternAttribute();
```

Prefer a conditionally defined modifier macro and use that macro in the declaration. Configure the macro as `BareIdentifierMacros` so the declaration grammar can consume it as a modifier token.

```cpp
#ifndef FORMAT_USERVER_CLANG
#define FORMAT_USERVER_EXTERN_ATTRIBUTES [[gnu::visibility("default")]] [[gnu::externally_visible]]
#else
#define FORMAT_USERVER_EXTERN_ATTRIBUTES
#endif

FORMAT_USERVER_EXTERN_ATTRIBUTES int FormatUserverExternAttribute();
```

## Include Sorting

Include sorting is enabled when include groups are configured. Sorting may move `#include` lines within sortable include blocks. It does not add includes, remove includes, rewrite include spelling, or move comments.

When include groups are absent, include sorting is disabled. Include directives are normalized and emitted in source order, and blank-separated include blocks are preserved.

Opening include blocks follow the same include-run formatting path for `#pragma once` headers and `#ifndef`/`#define` guarded headers, so configured include groups control sorting or preservation for both forms.

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
- safe adjacent ordinary string-literal concatenation

String literals ending with escaped `\n` or `\r\n` are line-fragment boundaries and are not joined with the following literal. A trailing escape such as `\xB0` that would consume the next fragment's first character after textual joining also prevents joining.

Outside the listed changes, the formatter changes only spaces and line breaks.

Token spelling is preserved even when the parser grammar treats multiple spellings as the same syntax node. For example, Win32 `TRUE` and `FALSE` remain uppercase. Parser nodes that share a name with a keyword token still keep their child tokens; for example, `decltype(function)` remains a full `decltype` expression instead of being reduced to the `decltype` keyword.

## Tooling Ownership

- `strictfmt` owns explicit stdin input passed with `--stdin`, direct file arguments, recursive roots passed with `-r <path>` or `--recursive <path>` for `.c`, `.cc`, `.cpp`, `.cxx`, `.c++`, `.h`, `.hh`, `.hpp`, `.hxx`, `.h++`, `.ipp`, `.inl`, and `.tpp` files, newline file lists passed with `--files <path>`, `-i`, `--dry-run`, `--style <path>`, `--concurrency <n>` handling, parsing, parse-error rejection, checking, fixing, ignore-file filtering, and stdout rendering. Running `strictfmt` without inputs prints command-line help instead of reading stdin. Interactive file runs show completed file count and elapsed time while work is active, and final summaries report completed files, LOC, and elapsed time.
- `strictfmt::cli` exposes the same CLI implementation through `RunStrictfmtCli(argc, argv)` for embedding in host tools. Wrapper scripts own their own mode selection and changed or staged file-list preparation.
- `src\tools\format_cli.cpp` owns formatter command orchestration, `src\tools\format.cpp` owns source-text formatting, and the internal `src\tools\impl\format_*` formatter modules own parser setup, model definitions, tree-sitter model builder helpers, preprocessor model helpers, and pretty printing.
- `format_model_dump` owns ad hoc formatter model debugging. It takes one source file path and writes YAML to stdout with each `SyntaxNode` represented by `kind`, plus `text` only when node text is non-empty and `children` only when child nodes exist; text values up to 60 bytes are YAML-quoted, and longer text values are emitted as their byte length.
- `.cpp-format`, `tests/format/.cpp-format`, `tests/format/.cpp-format-userver`, and `.cpp-format-ignore` own the formatter configuration data described in [Formatter Configuration](#formatter-configuration).
- `vendor\tree-sitter\` owns vendored tree-sitter grammar inputs and generated parser sources.
- `tools\regenerate_tree_sitter_grammar.py` owns parser regeneration.

The formatter uses tree-sitter core from vcpkg and the vendored C++ grammar under `vendor\tree-sitter\tree-sitter-cpp\`. The C grammar under `vendor\tree-sitter\tree-sitter-c\` is kept for provenance and regeneration.

Regenerate parser outputs only after editing vendored grammar source:

```bat
python tools\regenerate_tree_sitter_grammar.py
```

The regeneration tool runs the pinned tree-sitter CLI and updates generated files under `vendor\tree-sitter\tree-sitter-cpp\src\`. Pass `--tree-sitter-cli <path>` to use an existing CLI. Otherwise it downloads the pinned Windows CLI under `build\`. Macro category entries in `.cpp-format` are runtime scanner inputs and do not require parser regeneration.

## Formatter Configuration

Configuration is intentionally narrow and does not expose style policy knobs. Brace behavior, wrapping behavior, spacing, alignment behavior, and other layout decisions are fixed in formatter source.

When `--style` is omitted, `strictfmt` searches upward from each formatted file for `.cpp-format`. For `--stdin`, discovery starts at the current working directory. `--style <path>` uses the provided config path for every input. Formatting file paths are still checked against the nearest `.cpp-format-ignore` found by walking upward from each formatted file.

`Inherit: Parent` makes a `.cpp-format` file inherit from the next `.cpp-format` found by searching upward from the config file's parent directory. Explicit `--style <path>` configs use the same parent search rooted at the explicit config file. If no parent config exists, inheritance starts from built-in defaults. Local scalar keys override inherited scalar keys. Local list keys replace inherited lists. Nested maps, such as `MacroCategories` and `StreamShift`, inherit by category, and a local category replaces only that inherited category.

The `.cpp-format` file uses the formatter's YAML-like subset: blank lines, `---`, `...`, and comments are ignored; comments start with `#` outside single or double quotes; scalars may be unquoted, single quoted, or double quoted; lists use indented `- value` entries. Unknown keys are ignored by the native formatter.

Supported top-level keys:

- `Inherit`: optional `Parent`, enabling parent `.cpp-format` inheritance.
- `ColumnLimit`: integer target column for formatter-owned wrapping. The default is `120`.
- `IndentWidth`: integer spaces per indentation level. The default is `4`.
- `TabWidth`: integer tab display width. The default is `4`.
- `IncludeCategories`: optional ordered list of include groups. Each entry requires `Regex`, and may set `Priority`; priorities sort ascending and default to list order. Regexes match the normalized include target with delimiters, such as `'<vector>'` or `'"util/path.h"'`.
- `MainIncludeChar`: `Quote` makes the main include detection consider quoted includes.
- `IncludeIsMainRegex`: regex suffix appended to the current source file stem for main-include detection. The default is `(Test)?$`, so `widget.cpp` treats `"widget.h"` and `"widgetTest.h"` as main include candidates.
- `MacroCategories`: macro and macro-like runtime parser roles. The custom tree-sitter scanner reads these categories from the active config while parsing.
- `StreamShift`: stream insertion/extraction configuration.

Example:

```yaml
---
Inherit: Parent
ColumnLimit: 120
IndentWidth: 4
TabWidth: 4

IncludeCategories:
  - Regex: '^<.*>$'
    Priority: 1
  - Regex: '^".*"$'
    Priority: 2
MainIncludeChar: Quote
IncludeIsMainRegex: '(Test)?$'

MacroCategories:
  BareIdentifierMacros:
    - CALLBACK
  CallSyntaxMacros:
    - TEST

StreamShift:
  ConfigurationMethods:
    - std::boolalpha
    - std::setw
```

### StreamShift

`StreamShift.ConfigurationMethods` lists manipulators that bind to the following shifted value. The formatter keeps the configured manipulator run and its value together instead of choosing a break between them.

```cpp
stream << std::boolalpha << enabled << std::setw(8) << value;
```

### .cpp-format-ignore

`.cpp-format-ignore` is a plain line-based ignore file. Blank lines and comments are ignored with the same comment stripping as `.cpp-format`; backslashes are normalized to slashes; repeated leading `./` and trailing slashes are removed; matching is case-insensitive.

An entry without `/` matches any path component below the ignore file directory. An entry with `/` matches that relative path or any child path below it. Glob patterns and negation are not supported.

Example:

```text
vendor
build
```

`vendor` ignores any directory or file component named `vendor`.

### Macro Categories

All `MacroCategories` entries are read from the active `.cpp-format` at formatting time. These categories are runtime parser inputs: the custom tree-sitter scanner classifies identifiers into the fixed grammar token roles only when the active config contains the identifier or a matching trailing-`*` prefix. Changing these category entries requires rerunning the formatter with the new config, but does not require parser regeneration.

Macro category entries must be C/C++ identifiers. Add a trailing `*` to an entry when the role applies to every identifier with that prefix, such as `ATTRIBUTE*`; no other glob syntax is supported.

#### RawMacroFunctionDefinitions

`RawMacroFunctionDefinitions` names `#define` function-like macro identifiers that should be parsed as one raw preprocessor function definition. Use it for macro families whose replacement lists are not normal C++ fragments.

Raw replacement-list definition: the macro body is a declaration-fragment DSL that should remain one raw preprocessor definition.

```cpp
#define UPROTO_ONEOF_HEADER(oneof_type)                                                   \
private:                                                                                  \
    enum { kCounterStart = __COUNTER__ + 1 }; /* An inline constant would violate odr. */ \
public:                                                                                   \
    using Base::Base;
```

#### BareIdentifierMacros

`BareIdentifierMacros` names macro identifiers that the grammar consumes as bare tokens in non-call positions or as assertion-style statement-call names. This category owns calling-convention modifiers, declaration-prefix modifiers, complete declaration-level items, qualified-identifier prefixes, and statement-call macros whose argument is parsed as a statement without its trailing semicolon.

Calling-convention modifier: the macro appears in a declarator where a platform calling-convention token is expected.

```cpp
typedef PDH_STATUS (WINAPI* PdhAddEnglishCounterAFn)(PDH_HQUERY, LPCSTR, DWORD_PTR, PDH_HCOUNTER*);
```

Declaration-prefix modifier: the macro appears before a declaration as an attribute, inline, or sanitizer-control token.

```cpp
ATTRIBUTE_NO_SANITIZE_UNDEFINED std::size_t AttributePrefixedFunction(const BoundsBlock& block, float value) noexcept;
```

Complete declaration-level item: the macro stands as a full top-level declaration item, such as namespace wrappers.

```cpp
USERVER_NAMESPACE_BEGIN
namespace utils {
}  // namespace utils
USERVER_NAMESPACE_END
```

Qualified-identifier prefix: the macro supplies an optional namespace qualifier before an identifier.

```cpp
enum netrc_t {
    netrc_optional = CURL_8_13_NAMESPACE CURL_NETRC_OPTIONAL,
};
```

Statement-call macro: the macro parses its first argument as a declaration or expression statement.

```cpp
UEXPECT_THROW([[maybe_unused]] auto bytes_read = source.ReadSome(kBuffer, kDeadline), IoTimeout);
```

#### CallSyntaxMacros

`CallSyntaxMacros` names macro identifiers whose supported grammar roles start with a macro-style call. This category owns macro function definitions, macro function definitions with trailing C++ parameter lists, top-level free-token call statements with optional chained `->` tails, simple name macro calls, class-field method declaration macros, and type-specifier macro calls.

Macro function definition: the macro call header is followed by a compound statement body.

```cpp
UTEST_MT(FormatterMacroFixture, KeepsThreads, 2) {
    RunThreadedTest();
}
```

Macro function definition with trailing C++ parameters: the macro call is followed by a normal parameter list before the body.

```cpp
BENCHMARK_DEFINE_F(FormatterBenchmark, Inline)(benchmark::State& state) {
    UseBenchmarkState(state);
}
```

Top-level free-token call statement: the whole call and optional `->` chain are formatted as one statement.

```cpp
BENCHMARK_TEMPLATE(RecentPeriodOfPercentilesAccountBenchmark, DefaultClock)->ThreadRange(1, 16);
```

Simple name macro call: the call has a single identifier argument and is treated as one standalone macro item.

```cpp
RET_NAME(kNullValue)
```

Class member declaration macro: the macro call expands to a method declaration inside class scope.

```cpp
MOCK_METHOD(void, SetValue, (std::string_view, std::string&&), (override));
```

Type-specifier macro call: the macro call appears where a type specifier is expected.

```cpp
using CertStack = STACK_OF(X509);
```
