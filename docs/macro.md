# Macro formatting

This document specifies the macro configuration and macro formatting for `strictfmt`.

## General

Macro continuation backslashes, spaces before continuation backslashes, and continuation newlines are formatter-owned. A multi-line macro definition is parsed as one replacement list, then emitted with continuation backslashes on all continued macro lines.

Macro replacement lists that form declaration fragments are recursively formatted before continuation backslashes are added.

## Object-line and function-line macro 

Object-like `#define NAME replacement` definitions are always preserved as raw preprocessor definitions. Their replacement lists are treated as arbitrary tokens and the formatter does not reinterpret them as C++ expressions or declarations.

Funline-line `#define NAME(PARAMS...) replacement` definitiion are treated depending on their category.

## Marco categories

Macro category entries must be C/C++ identifiers. Add a trailing `*` to an entry when the role applies to every identifier with that prefix, such as `ATTRIBUTE*`; no other glob syntax is supported.

The macros that belong to different categories are configured in configuration file, see [config.md](config.md).

### RawMacroFunctionDefinitions

`RawMacroFunctionDefinitions` names `#define` function-like macro identifiers that should be parsed as one raw preprocessor function definition. Use it for macro families whose replacement lists are not normal C++ fragments.

```cpp
#define UPROTO_ONEOF_HEADER(oneof_type)                                                   \
private:                                                                                  \
    enum { kCounterStart = __COUNTER__ + 1 }; /* An inline constant would violate odr. */ \
public:                                                                                   \
    using Base::Base;
```

### BareIdentifierMacros

`BareIdentifierMacros` names macro identifiers that the grammar consumes as bare tokens in non-call positions or as assertion-style statement-call names. This category owns calling-convention modifiers, declaration-prefix modifiers, complete declaration-level items, qualified-identifier prefixes, top-level call-statement suffix macros, declaration suffix macros, initializer macros, and statement-call macros whose argument is parsed as a statement without its trailing semicolon.

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

### CallSyntaxMacros

`CallSyntaxMacros` names macro identifiers whose supported grammar roles start with a macro-style call. This category owns macro function definitions, macro function definitions with trailing C++ parameter lists, top-level macro call statements with optional chained `->` tails, simple name macro calls, class-field method declaration macros, and type-specifier macro calls.

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

Top-level macro call statement: the whole call, optional configured bare-macro suffix, and optional `->` chain are formatted as one statement. 

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
