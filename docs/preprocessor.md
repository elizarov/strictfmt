# Preprocessor

**This document has been maintained mostly by AI and has not been reviewed by a human for conciseness and clarity of presentation.**

This document describes handling of preprocessor directives, conditional compilation, and local includes by `strictfmt`.

The custom scanner owns the lexical distinction between directive-ending line breaks and ordinary line-break whitespace; see [scanner.md](scanner.md).

Macro definitions, `#undef`, `#pragma`, `#line`, `#error`, `#warning`, `#using`, and null directives may occur between C++ tokens. They remain structured directive nodes and do not replace required C++ punctuation. Definitions retain the replacement rules in [macro.md](macro.md).

## Supported Conditional Compilation and Local Includes

This is the closed list of placements that are explicitly supported.

- **Whole source items**: conditionals may select complete sibling [source items](glossary.md#source-item) that parse at the surrounding level. `#else` and `#elif` branches are supported when every branch contributes complete items for the same surrounding container.
- **Guarded namespace opener**: a conditional opener may contain complete source items followed by a namespace definition whose body continues after `#endif` through its closing `}`. Anonymous, named, inline, attributed, and nested namespace headers use ordinary namespace syntax. Branch alternatives are unsupported for this cross-directive grouping shape.
- **Comma-separated syntax**: conditionals may select complete [list](glossary.md#list) items. A selected item may own its trailing comma. Enum branches may contain multiple entries and nested conditionals; the final entry may omit its comma. Call argument, base-class, constructor initializer, and lambda capture branches may contain multiple entries. Lambda captures also support nested conditional groups and `#elif` alternatives. Call arguments, base-class entries, braced initializer entries, constructor field initializers, declaration parameters, and template parameters may also own the leading separator comma when they follow existing items. Other list positions do not support a branch-owned leading comma. Declaration and template parameters, including their branch-owned leading-comma form, support `#if`, `#ifdef`, or `#ifndef` without branch alternatives. Subscript items support `#if` with an optional `#else`. Arguments, base-class entries, initializer items (including a branch-owned leading comma), ordinary constructor field initializers, and enum entries support `#if`, `#ifdef`, or `#ifndef` with an optional `#else`; a branch-owned leading constructor comma supports `#if`, `#ifdef`, or `#ifndef` without an alternative. Base-class and braced initializer branches also support `#elif` alternatives.
- **Complete base-class and constructor initializer clauses**: conditionals may select entire base-class or constructor initializer lists including the opening colon, with optional `#else` or `#elif` alternatives.
- **Statement-prefix macros**: conditionals may select configured statement-prefix macros before a shared statement, including nested groups, empty branches, and `#else` or `#elif` alternatives.
- **Declaration-prefix modifiers**: conditionals may select standalone declaration modifiers before a declaration: `const`, `constexpr`, `consteval`, `static`, `extern`, `inline`, `__inline`, `__inline__`, `__forceinline`, one macro-shaped modifier line, or standalone attributes.
- **Modifiers after declaration types**: conditionals may select declaration modifiers between a type and its declarator, including nested groups and `#else` or `#elif` alternatives.
- **Function return types**: a [conditional opener](glossary.md#conditional-opener) with a required `#else` may select complete declaration-specifier sequences that form the return type of a function definition. Declaration modifiers before the conditional and the function declarator and body after it are shared by both branches. `#elif`, function declarations without a body, constructors, destructors, and conversion functions are unsupported for this placement.
- **Declaration-suffix modifiers**: `#ifdef` or `#ifndef` blocks may select standalone identifiers, configured bare macros used as function-suffix modifiers, or attributes after a complete declaration declarator list and before the terminating semicolon.
- **Template constraints**: conditionals may select a complete `requires` clause after a template parameter list, including nested groups and `#else` or `#elif` alternatives.
- **Function exception specifications**: conditionals may select `noexcept` or `throw` specifications after function parameters and qualifiers, including nested groups and `#else` or `#elif` alternatives.
- **Selected function headers**: conditionals may select complete ordinary function, constructor, or destructor headers (ordinary functions may include a language linkage specification), with `#else` or `#elif` alternatives, when the shared body follows `#endif`. Constructor initializer lists may precede that body; constructor and destructor declarations may instead share their terminating semicolon.
- **Selected common-body ordinary function starts**: a top-level conditional opener with a required `#else` may select complete ordinary function prefixes when the shared body continues after `#endif`. Each branch may contain local includes and an optional language linkage specification, and must end with the declaration specifiers, function declarator, and opening `{`. `#elif`, constructors, declarations other than local includes before the selected prefix, and selected prefixes below top level are unsupported.
- **Selected common-body macro-function starts**: an `#if`, `#ifdef`, or `#ifndef` with an optional `#else` may select complete macro-function or test prefixes when the shared body continues after the `#endif`. Each branch must consist only of the macro invocation and opening `{`. `#elif` and branch-local items before the selected macro-function start are unsupported.
- **Selected declaration initializer starts**: conditionals may select declaration specifiers, declarators, and an initializer's opening brace, with the initializer entries and closing brace shared after `#endif`. Nested groups, `#else` or `#elif` alternatives, direct-list and copy-list initialization, and other declarators in the same declaration are supported.
- **Selected call prefixes**: conditionals may select a callee and its opening parenthesis, with the arguments and closing parenthesis shared after `#endif`. Callees use ordinary expression syntax; nested conditional groups and `#else` or `#elif` alternatives are supported.
- **Selected expression operands**: conditionals may select complete expressions or braced initializers, with optional `#else` or `#elif` alternatives. Shared operators and the terminating semicolon may follow `#endif`.
- **Values with branch-owned semicolons**: conditionals may select values for variable declarations, assignments, returns (including `co_return` and `co_yield`), alias declarations, and concept definitions. A shared string-literal prefix may precede conditional string fragments in declarations, assignments, and returns. Each branch body must supply its own terminating semicolon; complete source items may follow it. Nested conditionals and `#else` or `#elif` alternatives are supported.
- **Selected `if` statements**: conditionals may select complete unbraced `if` headers before a shared statement after `#endif`. Branches may contain complete statements and declarations before the header, nested groups, and `#else` or `#elif` alternatives.
- **Selected common-body control starts**: conditionals may select `if` headers or `for` headers (classic, range-based, or `for each`) together with their opening `{`, followed by a shared body and closing `}`. Branches may contain complete statements and declarations before the selected header, nested conditional groups, and `#else` or `#elif` alternatives. A selected `if` body may be followed by a shared `else` clause.
- **Conditional `if constexpr`**: conditionals may guard `constexpr` between `if` and its condition, including nested groups and `#else` or `#elif` alternatives.
- **Guarded `if`/`else` prefixes**: conditionals may select an `if`/`else if` chain ending in `else`, with the final body shared after `#endif`. Branches may contain complete statements and declarations before the chain, nested conditional groups, and `#else` or `#elif` alternatives. Braced and unbraced shared bodies are supported; the opening brace may be selected with the prefix.
- **Conditional block endings**: conditionals may select the closing brace of a shared compound statement. Branches may contain statements and declarations before and after that brace, nested conditional groups, and `#else` or `#elif` alternatives.
- **Conditional branch transitions**: conditionals may close one `if` branch and open a subsequent `else if` or `else` branch, leaving the final body and closing brace shared. Transitions may repeat, contain nested conditional groups, and have `#else` or `#elif` alternatives.
- **Conditional `if` endings**: conditionals may select a shared consequence's closing brace together with a complete `else` clause. Branches may contain complete source items, nested conditional groups, and `#else` or `#elif` alternatives.
- **Conditional `do` endings**: conditionals may select the closing brace and `while` condition of a shared `do` body. Branches may contain complete source items, nested conditional groups, and `#else` or `#elif` alternatives.
- **Conditional `try` scopes**: conditionals may select `try` and its opening brace, or a shared body's closing brace together with its catch handlers. Branches may contain complete source items, nested conditional groups, and `#else` or `#elif` alternatives. Function and constructor try-block endings use the same handler rules.
- **Conditional `else` clauses**: a conditional-opener block may select a complete `else` clause, including an `else if` chain. Braced and unbraced bodies, nested conditional groups, and `#else`/`#elif` alternatives are supported.
- **Logical chain links**: a conditional-opener block may select a leading `&&` or `||` link in a shared logical expression, or the complete right operand after a shared `&&`. Operand selection may have an `#else`; `#elif` is unsupported.
- **Member-access chains**: conditionals may select continuations starting with `.` or `->`, including calls and subscripts. Branches may contain nested conditional groups and `#else` or `#elif` alternatives; the receiver precedes the conditional and further shared links may follow it.
- **Stream-shift chain links**: conditional-opener blocks with an optional `#else` may select complete leading links in a shared stream-shift chain. These blocks may nest, the receiver must precede the outer conditional, and further shared links and the terminating semicolon may follow it. `#elif` is unsupported in this placement.
- **Guarded namespace and language-linkage group delimiters**: conditionals may guard namespace or language-linkage openers, or their closing braces, as declaration-scope grouping items. Several nested scopes may share a group; nested conditionals and `#else` or `#elif` alternatives are supported. Namespace and linkage headers use their ordinary syntax.
- **Concatenated string fragments**: conditionals may select complete adjacent string-literal fragments inside a concatenated string literal, including an initializer that begins with a conditional, multiple conditional groups in one concatenation, `#elif` alternatives, and fragments interleaved with identifier-like or function-like string macros.
- **Include-supplied variable initializers**: a local `#include` directive may supply the complete token sequence after a variable declaration's `=`. The declaration's terminating semicolon follows the directive.
- **Include-supplied initializer-list fragments**: local `#include` directives may contribute entries and their separators inside braced initializer lists, including conditional branches and fragments interleaved with ordinary entries.
- **Local includes**: local `#include` directives may stand where the parser accepts them as complete items.

Enclosing control bodies are rejected when adding braces would change `else` ownership or span conditionally absent openers or branch-local items outside the controlled statement.

All other places are not supported and may result in parsing errors or produce misformatted output if the parser manages to recover without errors.

Specialized contextual placements do not support `#elifdef` or `#elifndef` alternatives. The generic whole-item preprocessor grammar retains those directives.

## Formatting rules

- Directive lines stay at column zero. Conditional compilation adds no indentation: branch contents and shared code after `#endif` follow the same scope, delimiter, and continuation indentation rules as unconditional code.
- A branch-owned leading comma stays on the same line as the selected braced initializer, constructor initializer, declaration parameter, or template parameter that follows it.
- Conditional declaration-prefix modifiers force a break before the rest of the declaration. Comments, attributes, and modifier lines inside the conditional use the indentation of the declaration that follows.
- Conditional function return types and their shared declarator each start on their own line. Selected return types use the function declaration's indentation.
- When a conditional right-hand side begins immediately after `=`, the formatter breaks after the `=` and formats the selected value with one continuation indent relative to the line that contains the `=`. Source items following its semicolon keep the surrounding scope's indentation.
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
  DeclarationModifierMacros:
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
