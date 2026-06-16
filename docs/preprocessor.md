# Preprocessor

This document describes handing of preprocessor directives and conditional compilation by `strictfmt`.

## Empty lines

Put one empty line after `#pragma once` when another source item follows. Put one empty line before and after each `#undef` when it separates `#undef` from a neighboring source item.

## Conditional Compilation And Local Includes

Conditional compilation is accepted when each branch contributes complete grammar items at the surrounding level: complete declarations, complete statements, switch `case` or `default` labels, field or method declarations, enum entries, macro definitions, includes, or similar syntax that already has a mandatory structural line break. Conditional declaration-prefix modifiers are also accepted for standalone modifiers and for attributes that precede a declaration. The conditional directive lines stay at column zero, and the guarded code keeps the indentation it would have at that source location.

Conditional compilation may also patch complete expression or declaration items in comma-separated lists. This is accepted for function arguments, braced initializer items, subscript items, declaration parameters, template arguments, template parameters, and enum entries. A conditional in one of these lists makes the guarded item use split-list indentation: directive lines stay at column zero, and branch items are indented as list items. Conditional expression items do not contain statement-terminating semicolons; conditional right-hand sides use the separate `=` rule below. Conditional list items use the same comma normalization as ordinary list items, so final items lose trailing commas except in enum bodies.

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

using ValueTypes = ::testing::Types<
#if HAS_ARRAY_VALUE
    std::array<int, 4>,
#endif
    std::string
>;
```

Conditional compilation is not accepted in base-class lists or constructor initializer lists. Those lists do not have a stable trailing-comma form in the supported style, so patching items there remains an unsupported preprocessor placement.

Conditional declaration-prefix modifiers format as a forced break before the rest of the declaration. The directive lines stay at column zero, while comments, attributes, and modifier lines inside the conditional use the indentation of the declaration that follows.

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

Conditional right-hand sides after `=` are accepted for variable declarations, assignment statements, alias declarations, and concept definitions. The formatter always breaks after the `=`, keeps directive lines at column zero, and formats branch contents with one continuation indent relative to the line that contains the `=`. Each branch body supplies its own terminating semicolon, so there is no extra semicolon after `#endif`.

```cpp
template <typename T>
concept IsFromCharsCorrectlySupported =
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE < 13
    // libstdc++ before 13.1 parse long double incorrectly
    !std::same_as<T, long double>;
#else
    true;
#endif

void SelectStatus(Status& status) {
    status =
#if USE_FACTORY
        MakeStatus();
#else
        Status{};
#endif
}

void HandleSocketError(int error) {
    switch (error) {
        case Temporary:
#if TEMPORARY_AGAIN_IS_DISTINCT
        case TemporaryAgain:
#endif
            Retry();
            break;
    }
}
```

Local `#include` directives follow the same boundary rule: they may stand where the surrounding grammar accepts a complete declaration, statement, member declaration, enum entry, or directive, but not inside another expression or declaration. The include directive line stays at column zero.

```cpp
void RegisterGeneratedMetrics() {
#include "generated_metrics.inc"
    CommitGeneratedMetrics();
}
```

Conditional directives are rejected below the complete-item, declaration-prefix modifier, conditional-right-hand-side, or list-item boundary, such as inside an expression or statement header. A conditional block whose branches do not end as complete items and that is followed by an expression-continuation operator is also rejected, independent of the specific operator token. The formatter reports every offending `#if`, `#ifdef`, `#ifndef`, or `#include` line as `unsupported preprocessor placement`.

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
