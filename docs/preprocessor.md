# Preprocessor

This document describes handling of preprocessor directives, conditional compilation, and local includes by `strictfmt`.

The custom scanner owns the lexical distinction between directive-ending line breaks and ordinary line-break whitespace; see [scanner.md](scanner.md).

## Supported Conditional Compilation and Local Includes

- **Whole source items**: conditionals may select complete declarations, statements, switch `case` or `default` labels, field or method declarations, enum entries, macro definitions, includes, and other complete grammar items at the surrounding level.
- **Comma-separated list items**: conditionals may select complete function arguments, braced initializer items, subscript items, declaration parameters, template arguments, template parameters, and enum entries.
- **Declaration-prefix modifiers**: conditionals may select standalone modifiers or attributes that precede a declaration.
- **Conditional right-hand sides after `=`**: conditionals may select branch bodies for variable declarations, assignment statements, alias declarations, and concept definitions. Each branch body must supply its own terminating semicolon.
- **Local includes**: local `#include` directives may stand where the parser accepts them as complete items.

All other places (e.g. patching parts of expressions or arbitrary pieces of declarations) is not supported and may result in parsing errors or produce and misformatted output if the parser manages to recover without errors.

## Formatting rules

- Directive lines stay at column zero. Guarded code keeps the indentation it would have at that source location.
- Conditional declaration-prefix modifiers force a break before the rest of the declaration. Comments, attributes, and modifier lines inside the conditional use the indentation of the declaration that follows.
- For conditional right-hand sides after `=`, the formatter always breaks after the `=` and formats branch contents with one continuation indent relative to the line that contains the `=`.

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
    "four"
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
    StringLiteral(const char* literal) noexcept
        : zstring_view{literal} {}
};
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
