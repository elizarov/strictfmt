# Preprocessor

This document describes handling of preprocessor directives, conditional compilation, and local includes by `strictfmt`.

The custom scanner owns the lexical distinction between directive-ending line breaks and ordinary line-break whitespace; see [scanner.md](scanner.md).

## Supported Conditional Compilation and Local Includes

This is the closed list of placements that are explicitly supported.

- **Whole source items**: conditionals may select complete sibling [source items](glossary.md#source-item) that parse at the surrounding level. `#else` and `#elif` branches are supported when every branch contributes complete items for the same surrounding container.
- **Guarded anonymous namespace opener**: an `#if` may contain `namespace {` and all namespace items, with `#endif` immediately before the namespace's closing `}`. `#ifdef`, `#ifndef`, inline or named namespaces, namespace attributes, and branch alternatives are unsupported for this cross-directive grouping shape.
- **Comma-separated syntax**: conditionals may select complete [list](glossary.md#list) items. A selected item may own its trailing comma; conditional enum entries must own it. Constructor field initializers, declaration parameters, and template parameters may also own the leading separator comma when they follow existing items. Other list positions do not support a branch-owned leading comma. Declaration and template parameters, including their branch-owned leading-comma form, support `#if`, `#ifdef`, or `#ifndef` without branch alternatives. Subscript items support `#if` with an optional `#else`. Arguments, initializer items, ordinary constructor field initializers, and enum entries support `#if`, `#ifdef`, or `#ifndef` with an optional `#else`; a branch-owned leading constructor comma is supported only by `#if` without an alternative.
- **Declaration-prefix modifiers**: conditionals may select standalone declaration modifiers before a declaration: `const`, `constexpr`, `consteval`, `static`, `extern`, `inline`, `__inline`, `__inline__`, `__forceinline`, one macro-shaped modifier line, or standalone attributes.
- **Function return types**: a [conditional opener](glossary.md#conditional-opener) with a required `#else` may select complete declaration-specifier sequences that form the return type of a function definition. Declaration modifiers before the conditional and the function declarator and body after it are shared by both branches. `#elif`, function declarations without a body, constructors, destructors, and conversion functions are unsupported for this placement.
- **Declaration-suffix modifiers**: `#ifdef` or `#ifndef` blocks may select standalone identifiers, configured bare macros used as function-suffix modifiers, or attributes after a complete declaration declarator list and before the terminating semicolon.
- **Selected common-body ordinary function starts**: a top-level conditional opener with a required `#else` may select complete ordinary function prefixes when the shared body continues after `#endif`. Each branch may contain local includes and must end with the declaration specifiers, function declarator, and opening `{`. `#elif`, constructors, declarations other than local includes before the selected prefix, and selected prefixes below top level are unsupported.
- **Selected common-body macro-function starts**: an `#if` with an optional `#else` may select complete configured macro-function or test prefixes when the shared body continues after the `#endif`. Each branch must consist only of the macro invocation and opening `{`. `#ifdef`, `#elif`, and branch-local items before the selected macro-function start are unsupported.
- **Conditional right-hand sides after `=`**: conditionals may select branch bodies for variable declarations, assignment statements, alias declarations, and concept definitions. Each branch body must supply its own terminating semicolon.
- **Selected `if` statements**: a single conditional-opener block with an optional `#else` branch may select complete unbraced `if` headers when the following statement starts after the `#endif`.
- **Conditional `else if` branches**: conditionals may select complete `else if` branches inside an `if`/`else if` chain.
- **Stream-shift chain links**: conditional-opener blocks with an optional `#else` may select complete leading links in a shared stream-shift chain. These blocks may nest, the receiver must precede the outer conditional, and further shared links and the terminating semicolon may follow it. `#elif` is unsupported in this placement.
- **Guarded `extern "C"` group delimiters**: conditional-opener blocks may guard an `extern "C" {` opener or its matching closing brace as file-scope grouping items.
- **Concatenated string fragments**: conditionals may select complete adjacent string-literal fragments inside a concatenated string literal, including an initializer that begins with a conditional, multiple conditional groups in one concatenation, `#elif` alternatives, and fragments interleaved with identifier-like or function-like string macros.
- **Include-supplied variable initializers**: a local `#include` directive may supply the complete token sequence after a variable declaration's `=`. The declaration's terminating semicolon follows the directive.
- **Local includes**: local `#include` directives may stand where the parser accepts them as complete items.

All other places are not supported and may result in parsing errors or produce misformatted output if the parser manages to recover without errors.

Specialized contextual placements do not support `#elifdef` or `#elifndef` alternatives. The generic whole-item preprocessor grammar retains those directives.

## Formatting rules

- Directive lines stay at column zero. Guarded code keeps the indentation it would have at that source location.
- A branch-owned leading comma stays on the same line as the selected constructor initializer, declaration parameter, or template parameter that follows it.
- Conditional declaration-prefix modifiers force a break before the rest of the declaration. Comments, attributes, and modifier lines inside the conditional use the indentation of the declaration that follows.
- Conditional function return types and their shared declarator each start on their own line. Selected return types use the function declaration's indentation.
- For conditional right-hand sides after `=`, the formatter always breaks after the `=` and formats branch contents with one continuation indent relative to the line that contains the `=`.
- For an include-supplied variable initializer, the formatter breaks after `=` and places the terminating semicolon one continuation level beyond the declaration's indentation.
- A conditional stream-shift chain separates its receiver from the shifted tail.

## Examples

Whole-item conditionals:

```cpp
void NormalizeSocketFlags(int& flags) {
#ifdef SOCK_CLOEXEC
    flags &= ~SOCK_CLOEXEC;
#endif
}
```

Comma-separated list items:

```cpp
std::vector<std::string> list{
    "one",
    "two",
#if MORE
    "three",
#endif
#if EVEN_MORE
    "four",
#endif
};
```

Declaration-prefix modifiers:

```cpp
class StringLiteral : public zstring_view {
public:
#if defined(__clang__) && __clang_major__ < 18
    // clang-16 and below lose the pointer to `literal` with consteval.
    constexpr
#else
    consteval
#endif
    StringLiteral(const char* literal) noexcept : zstring_view{literal} {}
};
```

Selected common-body ordinary function start with a branch-local include:

<!-- .cpp-format
MacroCategories:
  DeclarationPrefixMacros:
    - API_EXPORT
-->
```cpp
#ifdef PLATFORM_WINDOWS
#include <tchar.h>

API_EXPORT int PlatformMain(int argc, TCHAR** argv) {
#else
API_EXPORT int PlatformMain(int argc, char** argv) {
#endif
    return Run(argc, argv);
}
```

Conditional right-hand sides:

```cpp
void SelectStatus(Status& status) {
    status =
#if USE_FACTORY
        MakeStatus();
#else
        Status{};
#endif
}
```

Local includes:

```cpp
void RegisterGeneratedMetrics() {
#include "generated_metrics.inc"
    CommitGeneratedMetrics();
}
```

Include-supplied variable initializer:

```cpp
constexpr auto kEmbeddedSchema =
#include "embedded_schema.inc"
    ;
```
