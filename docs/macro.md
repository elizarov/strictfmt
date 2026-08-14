# Macro formatting

This document specifies the macro configuration and macro formatting for `strictfmt`.

## Definition-side vs use-side 

Definition-side macro categories and use-side macro categories are independent:

- `RawMacroDefinitions` affects only how a `#define` replacement is parsed and printed. 
- `BareIdentifierMacros`, `DeclarationPrefixMacros`, `CallSyntaxMacros`, `SemicolonlessCallMacros`, `StatementArgumentMacros`, `TypeSpecifierMacros`, and `PreprocessorArgumentMacros` affect only how macro identifiers are parsed when they are used elsewhere in source.

## Macro Replacements

Structured macro definitions are the default. Their replacement parses as a structured token stream and parse tree, and the formatter owns replacement whitespace, continuation backslashes, continuation newlines, and line-limit wrapping. Macro replacement lists that form declaration fragments are recursively formatted before continuation backslashes are added.

The boundary between a structured macro definition header and its non-empty replacement is an optional owner/value break opportunity. The header and replacement are solved together with every ordinary break opportunity inside the replacement still available. If the owner/value boundary is selected, the replacement starts one continuation indentation level deeper. Replacement width, top-level element count, and replacement syntax class do not force that boundary independently of the solver.

Every non-final physical line of a structured macro definition ends in ` \`. Those two emitted columns are part of that line's overflow cost. The final replacement line has no continuation suffix.

For structured macro definitions, the original placement of continuation backslashes is semantically inert. A backslash-newline inside the replacement is treated as whitespace, just like ordinary source whitespace. The replacement ends at the first bare preprocessor directive newline after the macro value, and the pretty printer chooses the formatted line breaks and continuation backslashes.

The parser/scanner split that makes this possible is described in [scanner.md](scanner.md).

It is a parse error when a structured macro replacement cannot be parsed structurally. Add that macro identifier to `RawMacroDefinitions` only when the replacement intentionally is not a supported C++ fragment.

Raw macro definitions are the explicit exception. A macro whose identifier matches `RawMacroDefinitions` replacement is one raw string token instead of a structured replacement tree. The raw replacement printer collapses horizontal whitespace for single-line replacements. For multi-line replacements, it preserves the physical continuation-line structure, backslashes, and relative indentation, while rebasing the least-indented replacement line to one indentation level beyond `#define`. It also applies the same line-ending and trailing `//` comment-spacing normalization as other raw preprocessor text. This is the only macro-definition case where the formatter intentionally does not own all replacement whitespace and line breaks.

## Macro Categories

Macro category entries must be C/C++ identifiers. Add a trailing `*` to an entry when the role applies to every identifier with that prefix, such as `ATTRIBUTE*`; no other glob syntax is supported.

The macros that belong to different categories are configured in formatter configuration, see [config.md](config.md).

Runtime macro category lookup is implemented by the custom scanner; see [scanner.md](scanner.md).

### RawMacroDefinitions

`RawMacroDefinitions` names object-like or function-like `#define` identifiers whose replacement should be parsed as raw text. Use it for macro families whose replacement lists are not supported structured C++ fragments.

```cpp
#define UPROTO_ONEOF_HEADER(oneof_type)                                                   \
private:                                                                                  \
    enum { kCounterStart = __COUNTER__ + 1 }; /* An inline constant would violate odr. */ \
public:                                                                                   \
    using Base::Base;
```

```cpp
#define USERVER_IMPL_FORCE_INLINE [[gnu::always_inline]] inline
```

### DeclarationPrefixMacros

`DeclarationPrefixMacros` names macro identifiers that prefix a declaration, definition, class-field declaration, or declaration-like macro call as attribute, inline, export, or sanitizer-control tokens. A declaration-prefix modifier may be followed by a macro argument list when the macro spelling is function-like but the use-site role is still a modifier rather than a standalone macro call.

```cpp
ATTRIBUTE_NO_SANITIZE_UNDEFINED std::size_t AttributePrefixedFunction(const BoundsBlock& block, float value) noexcept;

GTEST_INTERNAL_DEPRECATE_AND_INLINE("Use NewApi() instead")
int OldApi();
```

### BareIdentifierMacros

`BareIdentifierMacros` names macro identifiers that the grammar consumes on the use side as bare tokens in non-call positions. This category owns calling-convention and post-type declarator annotations, complete declaration-level items, qualified-identifier prefixes, top-level call-statement suffix macros, declaration suffix macros, initializer macros, ordinary expression atoms, parameter-list items, and template-argument fragments. A configured token remains valid as an expression atom when the same project also passes it as a normal call argument or binary-expression operand.

**Calling-convention modifier:** the macro appears in a declarator where a platform calling-convention token is expected.

```cpp
typedef PDH_STATUS (WINAPI* PdhAddEnglishCounterAFn)(PDH_HQUERY, LPCSTR, DWORD_PTR, PDH_HCOUNTER*);
```

Post-type declarator annotation: the macro appears after the declared type and before the normal declarator or abstract type suffix.

```cpp
static const unsigned char ALIGN(16) lookup_table[];

auto value = static_cast<Functor USERVER_MOVE_ONLY_FUNCTION_INVOKE_QUALS>(*slot);
```

**Complete declaration-level item:** the macro stands as a full top-level declaration item, such as namespace wrappers.

```cpp
USERVER_NAMESPACE_BEGIN
namespace utils {
}  // namespace utils
USERVER_NAMESPACE_END
```

**Qualified-identifier prefix:** the macro supplies an optional namespace qualifier before an identifier.

```cpp
enum netrc_t {
    netrc_optional = CURL_8_13_NAMESPACE CURL_NETRC_OPTIONAL,
};
```

Function suffix macro: the macro appears after a function declarator where an attribute-like suffix is expected.

```cpp
Data& operator*() & FORMAT_USERVER_LIFETIME_BOUND {
    return data_;
}

void Verify() GTEST_LOCK_EXCLUDED_(mutex);
```

Parameter-list item: the macro appears as a complete parameter-list item, usually to inject an implementation-specific SFINAE or attribute parameter.

```cpp
explicit Value(T value, ENABLE_IF((std::is_integral_v<T>))) noexcept;
```

Template-argument fragment: the macro expands to one or more template arguments and any separators needed before the next visible argument.

```cpp
FlatTuple<GTEST_FLAT_TUPLE_INT256 int> tuple;
```

### CallSyntaxMacros

`CallSyntaxMacros` names macro identifiers whose supported use-side grammar roles start with a macro-style call but are not ordinary C++ expression calls in that placement. This category owns macro function definitions, macro function definitions with trailing C++ parameter lists, namespace-scope macro calls with optional configured bare-macro suffixes or chained `->` tails, declaration-prefixed macro calls, parameter-list items, and class-field method declaration macros.

Do not add a macro to `CallSyntaxMacros` merely because its spelling is uppercase or function-like. A macro invocation that looks like a plain function call and appears where a normal C++ function call expression is syntactically valid needs no special macro configuration. The regular C++ expression grammar handles those calls, including normal argument expressions, larger expressions that contain the call, and ordinary expression operators after the call.

Use `CallSyntaxMacros` when the invocation appears in an expression-like position but its argument list is not a C++ argument list, for example because it contains type fragments.

```cpp
if (CHECK_ASSIGNABLE(T, T&&, value = std::move(other))) {
    return true;
}
```

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

Namespace-scope macro call statement: the whole call, optional configured bare-macro suffix, and optional `->` chain are formatted as one declaration item.

```cpp
BENCHMARK_TEMPLATE(RecentPeriodOfPercentilesAccountBenchmark, DefaultClock)->ThreadRange(1, 16);
```

Declaration-prefixed macro call: declaration modifiers may precede a configured call-syntax macro when the macro itself supplies the declaration body.

```cpp
static DEFINE_MUTEX(global_mutex);
```

Class member declaration macro: the macro call expands to a method declaration inside class scope.

```cpp
MOCK_METHOD(void, SetValue, (std::string_view, std::string&&), (override));
```

### SemicolonlessCallMacros

`SemicolonlessCallMacros` names function-like macro invocations that occupy a complete physical line at namespace or block scope without a trailing semicolon. This narrow category is separate from `CallSyntaxMacros` because ordinary declaration-like call macros must not greedily consume later source lines as one run of semicolonless calls.

```cpp
GTEST_DISABLE_DEPRECATED_PUSH_(/* getenv: deprecated */)
UseDeprecatedApi();
GTEST_DISABLE_DEPRECATED_POP_()
```

### TypeSpecifierMacros

`TypeSpecifierMacros` names function-like macro identifiers that produce a C++ type specifier at the use site. They compose after declaration modifiers and after `typename` in a dependent type.

```cpp
typedef GTEST_REMOVE_REFERENCE_AND_CONST_(Container) RawContainer;
typedef typename GTEST_BIND_(Selector, Type) BoundTest;
```

### PreprocessorArgumentMacros

`PreprocessorArgumentMacros` names function-like macros whose arguments are preprocessing-token sequences rather than C++ expressions, types, declarations, or statements. Use it only when the invocation deliberately inspects or transforms its arguments as tokens, for example a test helper that stringizes an unexpanded macro invocation.

The outer call remains structured: top-level commas separate arguments and the formatter can split the argument list. The complete call composes in expression and type-specifier positions, allowing a configured token generator to occupy a template-argument slot. Within each argument, recursively nested parentheses are recognized, while the complete argument text is preserved as one formatter atom. This accepts operators, adjacent identifiers or calls, empty arguments, string and character literals, raw strings, comments, and other preprocessing tokens. Separator commas around empty arguments are semantic preprocessing syntax and are preserved, including a comma immediately before the closing parenthesis. In accordance with function-like macro invocation rules, only parentheses protect an inner comma from separating outer arguments; brackets, braces, and angle brackets do not.

```cpp
EXPECT_EXPANSION("+=", GMOCK_PP_CAT(+, =));
EXPECT_EXPANSION("1", GMOCK_PP_HAS_COMMA(, ));
EXPECT_EXPANSION("0", GMOCK_PP_IS_BEGIN_PARENS(sss() sss));
GMOCK_PP_HAS_COMMA(value, );
Test<GMOCK_PP_FOR_EACH(TYPE_ELEMENT, ~, (int, float))>;
```

Do not use this category for calls whose arguments are valid C++ syntax. Those calls should retain structural formatting through the ordinary expression grammar or another narrower use-side category.

### StatementArgumentMacros

`StatementArgumentMacros` names macro identifiers whose call syntax parses the first argument as a declaration, block, or statement sequence without requiring that argument to be a C++ expression. Declarations retain normal C++ initializer forms, including `=`, braced, and parenthesized direct initialization. Remaining arguments are parsed as ordinary macro arguments.

Macros that look like plain function calls and whose arguments are all normal expressions do not belong here. Use this category for assertion-style macros where the documented argument is a statement, such as throw, no-throw, death, or fatal-failure assertions.

```cpp
UEXPECT_THROW([[maybe_unused]] auto bytes_read = source.ReadSome(kBuffer, kDeadline), IoTimeout);

UASSERT_NO_THROW(ydb::TopicWriter writer("test-writer", MakeWriterSettings(topic)));

EXPECT_DEATH({ RunChildProcess(); }, "signal");
```
