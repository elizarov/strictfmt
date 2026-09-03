# Macro formatting

This document specifies the macro configuration and macro formatting for `strictfmt`.

## Definition-side vs use-side

Definition-side and use-side macro categories are independent. `RawMacroDefinitions` affects only how a `#define` replacement is parsed and printed; every other macro category affects only how macro identifiers are parsed when used elsewhere in source.

## Macro Arguments

Macro argument lists permit empty and comment-only arguments in any position. Separator commas are preserved, including a comma immediately before the closing parenthesis.

## Macro Replacements

Structured macro definitions are the default. Their replacement parses as a structured token stream and parse tree, and the formatter owns its complete layout. Macro replacement lists that form declaration fragments are recursively formatted before continuation backslashes are added.

Token pasting, nested macro calls whose arguments are preprocessing-token sequences, and balanced parenthesized preprocessing tokens remain explicit recursive grammar nodes. They may use token-level rather than C++ expression-level structure because macro expansion determines their eventual C++ role, but they must not be collapsed into an opaque formatter leaf.

A structured macro definition has two header-level forms. If the complete definition fits on one physical line, it stays on that line. Otherwise, the formatter breaks after the complete definition header and starts the replacement one continuation indentation level deeper.

A replacement parsed as two or more top-level macro call units is a statement-like item sequence even when the calls have no separating commas or semicolons.

```cpp
#define FORMAT_FIXTURE_ITEMS(X) \
    X(Alpha, "alpha") \
    X(Beta, "beta") \
    X(Gamma, "gamma")
```

```cpp
#define FORMAT_FIXTURE_COMMENT_CONTINUATION(callback) \
    callback(); \
    /* cold testing path: */ \
    callback();
```

Every non-final physical line of a structured macro definition ends in ` \`. The final replacement line has no continuation suffix.

For structured macro definitions, the original placement of continuation backslashes is semantically inert. A backslash-newline inside the replacement is treated as whitespace, just like ordinary source whitespace. The replacement ends at the first bare preprocessor directive newline after the macro value, and the pretty printer chooses the formatted line breaks and continuation backslashes.

The parser/scanner split that makes this possible is described in [scanner.md](scanner.md).

It is a parse error when a structured macro replacement cannot be parsed structurally. Add that macro identifier to `RawMacroDefinitions` only when the replacement intentionally is not a supported C++ fragment.

Raw macro definitions are the explicit exception. A macro whose identifier matches `RawMacroDefinitions` replacement is one raw string token instead of a structured replacement tree. The raw replacement printer collapses horizontal whitespace for single-line replacements. For multi-line replacements, it preserves the physical continuation-line structure, backslashes, and relative indentation, while rebasing the least-indented replacement line to one indentation level beyond `#define`. It also applies the same line-ending and trailing `//` comment-spacing normalization as other raw preprocessor text. This is the sole opaque-source exception specified by [architecture.md](architecture.md#structural-genericity).

## Macro Categories

Macro category entries must be C/C++ identifiers. Add a trailing `*` to an entry when the role applies to every identifier with that prefix, such as `ATTRIBUTE*`; no other glob syntax is supported.

The macros that belong to different categories are configured in formatter configuration, see [config.md](config.md).

Runtime macro category lookup is implemented by the custom scanner; see [scanner.md](scanner.md).

### RawMacroDefinitions

`RawMacroDefinitions` accepts both object-like and function-like `#define` identifiers.

<!-- .cpp-format
MacroCategories:
  RawMacroDefinitions:
    - UPROTO_ONEOF_HEADER
-->
```cpp
#define UPROTO_ONEOF_HEADER(oneof_type)                                                   \
    private:                                                                                  \
        enum { kCounterStart = __COUNTER__ + 1 }; /* An inline constant would violate odr. */ \
    public:                                                                                   \
        using Base::Base;
```

<!-- .cpp-format
MacroCategories:
  RawMacroDefinitions:
    - USERVER_IMPL_FORCE_INLINE
-->
```cpp
#define USERVER_IMPL_FORCE_INLINE [[gnu::always_inline]] inline
```

### DeclarationPrefixMacros

`DeclarationPrefixMacros` names macro identifiers used as modifiers before [declaration-like items](glossary.md#declaration-like-item). A declaration-prefix modifier may have a macro argument list when its spelling is function-like but its use-site role is still a modifier rather than a standalone macro call.

<!-- .cpp-format
MacroCategories:
  DeclarationPrefixMacros:
    - ATTRIBUTE_NO_SANITIZE_UNDEFINED
-->
```cpp
ATTRIBUTE_NO_SANITIZE_UNDEFINED std::size_t AttributePrefixedFunction(const BoundsBlock& block, float value) noexcept;
```

<!-- .cpp-format
MacroCategories:
  DeclarationPrefixMacros:
    - GTEST_INTERNAL_DEPRECATE_AND_INLINE
-->
```cpp
GTEST_INTERNAL_DEPRECATE_AND_INLINE("Use NewApi() instead") int OldApi();
```

### BareIdentifierMacros

`BareIdentifierMacros` names macro identifiers used as bare tokens in supported non-call positions. A configured token remains valid as an expression atom when the same project also passes it as a normal call argument or binary-expression operand.

**Calling-convention modifier:** the macro appears in a declarator where a platform calling-convention token is expected.

<!-- .cpp-format
MacroCategories:
  BareIdentifierMacros:
    - WINAPI
-->
```cpp
typedef PDH_STATUS (WINAPI* PdhAddEnglishCounterAFn)(PDH_HQUERY, LPCSTR, DWORD_PTR, PDH_HCOUNTER*);
```

Post-type declarator annotation: the macro appears after the declared type and before the normal declarator or abstract type suffix.

<!-- .cpp-format
MacroCategories:
  BareIdentifierMacros:
    - ALIGN
    - USERVER_MOVE_ONLY_FUNCTION_INVOKE_QUALS
-->
```cpp
static const unsigned char ALIGN(16) lookup_table[];

auto value = static_cast<Functor USERVER_MOVE_ONLY_FUNCTION_INVOKE_QUALS>(*slot);
```

**Complete declaration-level item:** the macro stands as a full top-level declaration item, such as namespace wrappers.

<!-- .cpp-format
MacroCategories:
  BareIdentifierMacros:
    - USERVER_NAMESPACE_BEGIN
    - USERVER_NAMESPACE_END
-->
```cpp
USERVER_NAMESPACE_BEGIN
void UseNamespace();
USERVER_NAMESPACE_END
```

**Qualified-identifier prefix:** the macro supplies an optional namespace qualifier before an identifier.

<!-- .cpp-format
MacroCategories:
  BareIdentifierMacros:
    - CURL_8_13_NAMESPACE
-->
```cpp
enum netrc_t {
    netrc_optional = CURL_8_13_NAMESPACE CURL_NETRC_OPTIONAL,
};
```

Function suffix macro: the macro appears after a function declarator where an attribute-like suffix is expected.

<!-- .cpp-format
MacroCategories:
  BareIdentifierMacros:
    - FORMAT_USERVER_LIFETIME_BOUND
    - GTEST_LOCK_EXCLUDED_
-->
```cpp
class DataView {
    Data& operator*() & FORMAT_USERVER_LIFETIME_BOUND;

    void Verify() GTEST_LOCK_EXCLUDED_(mutex);
};
```

Parameter-list item: the macro appears as a complete parameter-list item, usually to inject an implementation-specific SFINAE or attribute parameter.

<!-- .cpp-format
MacroCategories:
  BareIdentifierMacros:
    - ENABLE_IF
-->
```cpp
class Value {
    explicit Value(T value, ENABLE_IF(std::is_integral_v<T>)) noexcept;
};
```

Template-argument fragment: the macro expands to one or more template arguments and any separators needed before the next visible argument.

<!-- .cpp-format
MacroCategories:
  BareIdentifierMacros:
    - GTEST_FLAT_TUPLE_INT256
-->
```cpp
FlatTuple<GTEST_FLAT_TUPLE_INT256 int> tuple;
```

### CallSyntaxMacros

`CallSyntaxMacros` names macro identifiers used as macro-style calls where an ordinary C++ expression call does not fit. Configure them for syntax differences such as argument lists containing type fragments, not merely for uppercase or function-like spelling.

<!-- .cpp-format
MacroCategories:
  CallSyntaxMacros:
    - CHECK_ASSIGNABLE
-->
```cpp
bool assignable = CHECK_ASSIGNABLE(T, T&&, value = std::move(other));
```

Macro function definition: the macro call header is followed by a compound statement body.

<!-- .cpp-format
MacroCategories:
  CallSyntaxMacros:
    - UTEST_MT
-->
```cpp
UTEST_MT(FormatterMacroFixture, KeepsThreads, 2) { RunThreadedTest(); }
```

Macro function definition with trailing C++ parameters: the macro call is followed by a normal parameter list before the body.

<!-- .cpp-format
MacroCategories:
  CallSyntaxMacros:
    - BENCHMARK_DEFINE_F
-->
```cpp
BENCHMARK_DEFINE_F(FormatterBenchmark, Inline)(benchmark::State& state) { UseBenchmarkState(state); }
```

Namespace-scope macro call statement: the whole call, optional configured bare-macro suffix, and optional `->` chain are formatted as one declaration item.

<!-- .cpp-format
MacroCategories:
  CallSyntaxMacros:
    - BENCHMARK_TEMPLATE
-->
```cpp
BENCHMARK_TEMPLATE(RecentPeriodOfPercentilesAccountBenchmark, DefaultClock)->ThreadRange(1, 16);
```

Declaration-prefixed macro call: declaration modifiers may precede a configured call-syntax macro when the macro itself supplies the declaration body.

<!-- .cpp-format
MacroCategories:
  DeclarationPrefixMacros:
    - API_EXPORT
  CallSyntaxMacros:
    - DEFINE_MUTEX
-->
```cpp
API_EXPORT DEFINE_MUTEX(global_mutex);
```

Class member declaration macro: the macro call expands to a method declaration inside class scope.

<!-- .cpp-format
MacroCategories:
  CallSyntaxMacros:
    - MOCK_METHOD
-->
```cpp
class MockValue {
    MOCK_METHOD(void, SetValue, (std::string_view, std::string&&), (override));
};
```

### SemicolonlessCallMacros

`SemicolonlessCallMacros` names function-like macro invocations that occupy a complete physical line at namespace or block scope without a trailing semicolon. This narrow category is separate from `CallSyntaxMacros` because ordinary declaration-like call macros must not greedily consume later source lines as one run of semicolonless calls.

<!-- .cpp-format
MacroCategories:
  SemicolonlessCallMacros:
    - GTEST_DISABLE_DEPRECATED_PUSH_
    - GTEST_DISABLE_DEPRECATED_POP_
-->
```cpp
void UseDeprecated() {
    GTEST_DISABLE_DEPRECATED_PUSH_(/* getenv: deprecated */)
    UseDeprecatedApi();
    GTEST_DISABLE_DEPRECATED_POP_()
}
```

### TypeSpecifierMacros

`TypeSpecifierMacros` names function-like macro identifiers that produce a C++ type specifier at the use site. They compose after declaration modifiers and after `typename` in a dependent type.

<!-- .cpp-format
MacroCategories:
  TypeSpecifierMacros:
    - GTEST_REMOVE_REFERENCE_AND_CONST_
    - GTEST_BIND_
-->
```cpp
typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
typedef typename GTEST_BIND_(Selector, Type) BoundTest;
```

### PreprocessorArgumentMacros

`PreprocessorArgumentMacros` names function-like macros whose arguments are preprocessing-token sequences rather than C++ syntax. Use it only when the invocation deliberately inspects or transforms its arguments as tokens, for example a test helper that stringizes an unexpanded macro invocation.

The outer call remains a structured list that the formatter can split. The complete call composes in expression and type-specifier positions. Within each argument, recursively nested parentheses are recognized while the complete preprocessing-token sequence is preserved as one formatter atom. Only parentheses protect an inner comma from separating outer arguments.

<!-- .cpp-format
MacroCategories:
  PreprocessorArgumentMacros:
    - EXPECT_EXPANSION
    - GMOCK_PP_HAS_COMMA
    - GMOCK_PP_FOR_EACH
-->
```cpp
void CheckExpansions() {
    EXPECT_EXPANSION("+=", GMOCK_PP_CAT(+, =));
    EXPECT_EXPANSION("1", GMOCK_PP_HAS_COMMA(, ));
    EXPECT_EXPANSION("0", GMOCK_PP_IS_BEGIN_PARENS(sss() sss));
    GMOCK_PP_HAS_COMMA(value, );
}

using Types = Test<GMOCK_PP_FOR_EACH(TYPE_ELEMENT, ~, (int, float))>;
```

### StatementArgumentMacros

`StatementArgumentMacros` names macro identifiers whose call syntax parses the first argument as a [source-item](glossary.md#source-item) sequence rather than requiring a C++ expression. Remaining arguments are parsed as ordinary macro arguments.

Macros that look like plain function calls and whose arguments are all normal expressions do not belong here. Use this category for assertion-style macros whose documented argument is a statement.

<!-- .cpp-format
MacroCategories:
  StatementArgumentMacros:
    - UEXPECT_THROW
    - UASSERT_NO_THROW
    - EXPECT_DEATH
-->
```cpp
void CheckReads() {
    UEXPECT_THROW([[maybe_unused]] auto bytes_read = source.ReadSome(kBuffer, kDeadline), IoTimeout);

    UASSERT_NO_THROW(ydb::TopicWriter writer("test-writer", MakeWriterSettings(topic)));

    EXPECT_DEATH(
        {
            RunChildProcess();
        },
        "signal"
    );
}
```
