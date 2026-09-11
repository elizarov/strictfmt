# Macro formatting

This document specifies the macro configuration and macro formatting for `strictfmt`.

Macro categories configure how identifiers are parsed at use sites, including uses inside other macro replacements.

## Macro Arguments

Calls that do not fit ordinary C++ argument syntax accept structured macro fragments, including types, parameter lists, statement sequences, and empty or comment-only arguments. Ordinary C++ interpretations take precedence when both fit. Macro argument separators are preserved, including a comma immediately before the closing parenthesis.

```cpp
bool assignable = CHECK_ASSIGNABLE(T, T&&, value = std::move(other));
```

A macro call may introduce a block body, including a loop body inside a function.

```cpp
UTEST_MT(FormatterMacroFixture, KeepsThreads, 2) { RunThreadedTest(); }
```

A macro header may also have trailing C++ parameters.

```cpp
BENCHMARK_DEFINE_F(FormatterBenchmark, Inline)(benchmark::State& state) { UseBenchmarkState(state); }
```

Namespace-scope calls, including an optional configured bare-macro suffix or `->` chain, form one declaration item.

```cpp
BENCHMARK_TEMPLATE(RecentPeriodOfPercentilesAccountBenchmark, DefaultClock)->ThreadRange(1, 16);
```

## Macro Replacements

Structured macro replacements are parsed and formatted recursively, including type specifiers and declaration fragments. The formatter owns their complete layout and adds continuation backslashes after formatting.

Token pasting, nested macro calls whose arguments are preprocessing-token sequences, and balanced parenthesized preprocessing tokens remain explicit recursive grammar nodes. They may use token-level rather than C++ expression-level structure because macro expansion determines their eventual C++ role, but they must not be collapsed into an opaque formatter leaf.

A structured macro definition has two header-level forms. If the complete definition fits on one physical line, it stays on that line. Otherwise, the formatter breaks after the complete definition header and starts the replacement one continuation indentation level deeper.

A replacement parsed as two or more top-level macro call units is a statement-like item sequence even when the calls have no separating commas or semicolons.

```cpp
#define FORMAT_FIXTURE_ITEMS(X) \
    X(Alpha, "alpha")           \
    X(Beta, "beta")             \
    X(Gamma, "gamma")
```

```cpp
#define FORMAT_FIXTURE_COMMENT_CONTINUATION(callback) \
    callback();                                       \
    /* cold testing path: */                          \
    callback();
```

Every non-final physical line of a structured macro definition ends in a continuation backslash. Within each macro definition, continuation backslashes align one space after the longest nonempty continuation line that fits within `ColumnLimit`, including the space and backslash. Blank continuation lines use the same column. Lines that exceed the limit with that suffix do not determine the alignment column and keep one space before their backslash. Alignment uses the final content widths after comment alignment. The final replacement line has no continuation suffix and does not determine the alignment column. Splices inside literals or token spellings retain their original spacing.

For structured macro definitions, the original placement of continuation backslashes is semantically inert. A backslash-newline inside the replacement is treated as whitespace, just like ordinary source whitespace. The replacement ends at the first bare preprocessor directive newline after the macro value, and the pretty printer chooses the formatted line breaks and continuation backslashes.

The parser/scanner split that makes this possible is described in [scanner.md](scanner.md).

### Raw replacements

A raw replacement is a macro body preserved as text when it has no complete structured parse. This is the sole opaque-source exception specified by [architecture.md](architecture.md#structural-genericity).

The grammar tries structured and raw alternatives in the same parse, preferring a complete structured replacement. The raw fallback ends at the directive boundary and does not repair syntax outside the replacement. Unterminated literals and comments remain parse errors.

Single-line raw replacements collapse horizontal whitespace. Multi-line raw replacements preserve continuation lines and relative indentation, rebasing the least-indented replacement line to one indentation level beyond `#define`. Their backslashes follow the alignment rule above.

Replacements containing raw or physically continued literals, or splices within token spellings, retain their original indentation. Literal contents and splices within tokens are preserved verbatim. Other raw text uses the same line-ending and trailing `//` comment-spacing normalization as raw preprocessor text.

Examples of incomplete C++ fragments preserved as raw replacements:

```cpp
#define UPROTO_ONEOF_HEADER(oneof_type)                                                       \
    private:                                                                                  \
        enum { kCounterStart = __COUNTER__ + 1 }; /* An inline constant would violate odr. */ \
    public:                                                                                   \
        using Base::Base;
```

```cpp
#define USERVER_IMPL_FORCE_INLINE [[gnu::always_inline]] inline
```

## Macro Categories

Macro category entries must be C/C++ identifiers. Add a trailing `*` to an entry when the role applies to every identifier with that prefix, such as `ATTRIBUTE*`; no other glob syntax is supported.

The macros that belong to different categories are configured in formatter configuration, see [config.md](config.md).

Runtime macro category lookup is implemented by the custom scanner; see [scanner.md](scanner.md).

`BareIdentifierMacros` and `SemicolonlessCallMacros` may also continue an expression by supplying operators and operands after its visible prefix. Several such expansions can follow one another; calls retain their configured argument syntax. Existing complete-item and list-fragment roles take precedence when both interpretations fit.

### DeclarationPrefixMacros

`DeclarationPrefixMacros` names macro identifiers used as modifiers before [declaration-like items](glossary.md#declaration-like-item). A declaration-prefix modifier may have a macro argument list when its spelling is function-like but its use-site role is still a modifier rather than a standalone macro call.

A macro before a declaration may expand to an annotation or a separate declaration. This category identifies `API_EXPORT` as a modifier, keeping it attached to `int value;`. Without configuration, this example fails to parse.

<!-- .cpp-format
MacroCategories:
  DeclarationPrefixMacros:
    - API_EXPORT
-->
```cpp
API_EXPORT int value;
```

<!-- .cpp-format
MacroCategories:
  DeclarationPrefixMacros:
    - GTEST_INTERNAL_DEPRECATE_AND_INLINE
-->
```cpp
GTEST_INTERNAL_DEPRECATE_AND_INLINE("Use NewApi() instead") int OldApi();
```

Declaration-prefixed macro call: declaration modifiers may precede a macro when the macro itself supplies the declaration body.

<!-- .cpp-format
MacroCategories:
  DeclarationPrefixMacros:
    - API_EXPORT
-->
```cpp
API_EXPORT DEFINE_MUTEX(global_mutex);
```

### StatementPrefixMacros

`StatementPrefixMacros` names modifiers that precede a complete statement. A prefix may have macro arguments, and several prefixes may nest. The prefix and its following statement form one control-flow body, including when braces are added to an enclosing control statement.

In `DISCARD_RESULT *value;`, the prefix precedes a dereference expression. Without this category, `DISCARD_RESULT` is parsed as a type and `*` attaches to it as a pointer declarator.

<!-- .cpp-format
MacroCategories:
  StatementPrefixMacros:
    - DISCARD_RESULT
    - FOR_EACH_VALUE
-->
```cpp
void Discard() { DISCARD_RESULT *value; }

void Exercise(bool enabled) {
    if (enabled) {
        DISCARD_RESULT Run();
    }
    FOR_EACH_VALUE(values, value) {
        Consume(value);
    }
}
```

### BareIdentifierMacros

`BareIdentifierMacros` names macro identifiers used as bare tokens in supported non-call positions. A bare token may supply a fragment of an enum or braced initializer list. A configured token remains valid as an expression atom when the same project also passes it as a normal call argument or binary-expression operand.

A bare macro may complete a statement where an ordinary identifier would remain an expression operand. Configuration separates `EMIT_EVENT` from the unary expression `+value;` below; without it, both form one addition expression on the same line.

<!-- .cpp-format
MacroCategories:
  BareIdentifierMacros:
    - EMIT_EVENT
-->
```cpp
void Emit() {
    EMIT_EVENT
    +value;
}
```

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

**Complete declaration-level item:** the macro stands as a full declaration item at namespace or class scope, such as namespace wrappers or generated members. A class-scope item may include a caller-written semicolon.

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

Declarator suffix macro: the macro appears after a declarator where an attribute-like suffix is expected, including parameter, field and function declarators.

<!-- .cpp-format
MacroCategories:
  BareIdentifierMacros:
    - FORMAT_USERVER_LIFETIME_BOUND
    - GTEST_LOCK_EXCLUDED_
-->
```cpp
class DataView {
    Data& operator*() & FORMAT_USERVER_LIFETIME_BOUND;

    Data& Borrow(Data& value FORMAT_USERVER_LIFETIME_BOUND);

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

### MethodDeclarationMacros

`MethodDeclarationMacros` names class-member macros taking a return type, method name, parameter list, and optional qualifier list. Configuration assigns these argument roles, including nested declarators, instead of treating every argument as an expression.

`Context* context` can also mean multiplication. Configuration selects the parameter interpretation below; without it, the argument formats as `(Context * context)`.

<!-- .cpp-format
MacroCategories:
  MethodDeclarationMacros:
    - MOCK_METHOD
-->
```cpp
class MockStore {
    MOCK_METHOD(void, Save, (Context* context), (ref(&), override));
};
```

### SemicolonlessCallMacros

`SemicolonlessCallMacros` names function-like macro invocations that form complete declaration or statement items without requiring a trailing semicolon, or supply fragments of enum and braced initializer lists. Each invocation remains one item, including when adjacent to another item or a control-body delimiter. Configured calls retain their category inside structured macro replacements.

A following parenthesized expression may start another statement or continue a chained call. Configuration separates `EMIT_EVENT(x)` from `(++count);` below; without it, they format as one chained call.

<!-- .cpp-format
MacroCategories:
  SemicolonlessCallMacros:
    - EMIT_EVENT
-->
```cpp
void Emit() {
    EMIT_EVENT(x)
    (++count);
}
```

### TypeSpecifierMacros

`TypeSpecifierMacros` names function-like macro identifiers that produce a C++ type specifier at the use site. They compose after declaration modifiers and after `typename` in a dependent type.

A macro call followed by `*` may declare a pointer or multiply expressions. Configuration makes `TYPE_OF(T)` the declared type below; without it, the formatter treats `*` as multiplication and puts spaces on both sides.

<!-- .cpp-format
MacroCategories:
  TypeSpecifierMacros:
    - TYPE_OF
    - GTEST_BIND_
-->
```cpp
void Declare() { TYPE_OF(T)* value; }

typedef typename GTEST_BIND_(Selector, Type) BoundTest;
```

### PreprocessorArgumentMacros

`PreprocessorArgumentMacros` names function-like macros whose arguments are preprocessing-token sequences rather than C++ syntax. Use it only when the invocation deliberately inspects or transforms its arguments as tokens, for example a test helper that stringizes an unexpanded macro invocation.

The outer call remains a structured list that the formatter can split. The complete call composes in expression and type-specifier positions. When the macro is also listed in `SemicolonlessCallMacros`, its token arguments are preserved in complete declaration and statement items, including inside structured macro replacements. Within each argument, recursively nested parentheses are recognized while the complete preprocessing-token sequence is preserved as one formatter atom. Only parentheses protect an inner comma from separating outer arguments.

Angle brackets may enclose C++ template arguments or remain ordinary preprocessing tokens. With `ColumnLimit: 20`, `TOKENS` splits its three preprocessing arguments below; without this category, `T<X, Y>` stays together as one C++ argument.

<!-- .cpp-format
ColumnLimit: 20
MacroCategories:
  PreprocessorArgumentMacros:
    - TOKENS
-->
```cpp
void CheckTokens() {
    TOKENS(
        T<X,
        Y>,
        NEXT
    );
}
```

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

The first argument may be a declaration that also parses as a chained call. In `UASSERT_NO_THROW` below, `Result (function)(Argument)` declares a function returning `Result`; without this category, it formats as a chained call with no space after `Result`.

<!-- .cpp-format
MacroCategories:
  StatementArgumentMacros:
    - UEXPECT_THROW
    - UASSERT_NO_THROW
    - EXPECT_DEATH
-->
```cpp
void CheckReads() {
    UASSERT_NO_THROW(Result (function)(Argument));

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
