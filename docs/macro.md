# Macro formatting

## Macro Categories

A macro's syntactic role cannot always be inferred from its use, so arbitrary code with macros cannot be parsed reliably. Some macros therefore need configuration for correct parsing and formatting.

- [DeclarationModifierMacros](#declarationmodifiermacros) identifies annotations and qualifiers within declarations and types.
- [StatementPrefixMacros](#statementprefixmacros) identifies modifiers attached to the following statement.
- [MethodDeclarationMacros](#methoddeclarationmacros) assigns return-type, name, parameter-list, and qualifier-list roles to method-declaration arguments.
- [ItemMacros](#itemmacros) identifies complete declaration or statement items and list fragments.
- [ExpressionContinuationMacros](#expressioncontinuationmacros) identifies fragments appended to an expression, such as operators or a supplied argument list.
- [TypeSpecifierMacros](#typespecifiermacros) identifies calls that supply a type specifier.
- [PreprocessorArgumentMacros](#preprocessorargumentmacros) preserves arguments as preprocessing-token sequences.
- [StatementArgumentMacros](#statementargumentmacros) parses the first argument as a sequence of statements or declarations.

Categories apply at use sites, including inside other macro replacements. Configure them in [`.cpp-format`](config.md) using C/C++ identifiers, optionally followed by `*` to match a prefix such as `ATTRIBUTE*`; no other glob syntax is supported. The [custom scanner](scanner.md) performs category lookup.

Macros may have argument lists regardless of their syntactic role.

### Macros without configuration

Function and macro calls share one argument grammar, accepting expressions, types, function and template parameters, statement sequences, and empty or comment-only arguments. Parenthesized argument fragments use the same grammar recursively and can appear in adjacent sequences. Their commas follow the same [comma normalization](format.md#comma-normalization) rules:

```cpp
void Check() {
    call(value);
    call(value, );
}
```

Calls also fit type-only positions, including aliases and function parameters.

An isolated identifier can supply a complete namespace or class item, or a template-list fragment, when it cannot form ordinary C++ syntax. Unknown modifiers are also accepted in class, struct, union, and template declaration headers and after function declarators or alias names. Configuration may still be needed to attach an identifier to the surrounding code.

Enum items may omit separating commas. Calls can supply enum or braced initializer list fragments, form statements without a trailing semicolon, or introduce a `{ ... }` body without configuration, as in tests or loops:

```cpp
TEST(StoreTest, SavesValue) { SaveValue(); }

void Visit(Items& items) {
    FOR_EACH(item, items) { Consume(item); }
}
```

A macro call may also be followed by C++ parameters before its body:

```cpp
BENCHMARK_DEFINE_F(StoreFixture, Save)(benchmark::State& state) { RunBenchmark(state); }
```

At namespace scope, a call and its following `->` chain stay together as one declaration:

```cpp
BENCHMARK_REGISTER_F(StoreFixture, Save)->Threads(4);
```

### DeclarationModifierMacros

`DeclarationModifierMacros` names annotations, qualifiers, and calling conventions within declarations or types. They can precede a declaration, occur between its type and declarator, or follow a declarator; one configuration covers all these positions.

A macro before a declaration may supply an annotation or a separate declaration. Configuration keeps `API_EXPORT` attached below; without it, the macro occupies its own line.

<!-- .cpp-format
MacroCategories:
  DeclarationModifierMacros:
    - API_EXPORT
-->
```cpp
API_EXPORT int value;
```

**Parse failure:** without configuration, a calling convention between a return type and function name can be mistaken for the declarator itself.

<!-- .cpp-format
MacroCategories:
  DeclarationModifierMacros:
    - CALLBACK
-->
```cpp
int CALLBACK Callback(int value) { return value; }
```

The same category covers declarator suffixes and annotations with arguments:

<!-- .cpp-format
MacroCategories:
  DeclarationModifierMacros:
    - ALIGN
    - LIFETIME_BOUND
    - LOCK_EXCLUDED
-->
```cpp
static const unsigned char ALIGN(16) lookup_table[];

struct View {
    Data& Borrow(Data& value LIFETIME_BOUND);

    void Verify() LOCK_EXCLUDED(mutex);
};
```

### StatementPrefixMacros

`StatementPrefixMacros` names modifiers that precede a complete statement. Several prefixes may nest. The prefix and its following statement form one control-flow body, including when braces are added to an enclosing control statement.

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

**Parse failures:** a prefix before a member call, streamed expression, or block may not fit ordinary C++ syntax. Each function below fails to parse without this category.

<!-- .cpp-format
MacroCategories:
  StatementPrefixMacros:
    - DISCARD_RESULT
    - RAISE
    - DEFER
-->
```cpp
void Discard(Message& message) { DISCARD_RESULT message.Parse(); }

void Fail() { RAISE Error() << "failure"; }

void Use() {
    DEFER {
        Cleanup();
    }
}
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

### ItemMacros

`ItemMacros` names complete declaration or statement items, or fragments of enum, initializer, and template lists that supply their own separators. An item can also serve as an expression atom when used as an operand or argument.

A following expression may start another statement or continue the macro invocation. Configuration separates both forms of `EMIT_EVENT` below; without it, they become an addition expression and a chained call, respectively.

<!-- .cpp-format
MacroCategories:
  ItemMacros:
    - EMIT_EVENT
-->
```cpp
void Emit() {
    EMIT_EVENT
    +value;
    EMIT_EVENT(x)
    (++count);
}
```

Namespace wrappers and generated class members also belong here:

<!-- .cpp-format
MacroCategories:
  ItemMacros:
    - BEGIN_NAMESPACE
    - END_NAMESPACE
    - GENERATED_MEMBERS
-->
```cpp
BEGIN_NAMESPACE
struct Record {
    GENERATED_MEMBERS
    int value;
};
END_NAMESPACE
```

### ExpressionContinuationMacros

`ExpressionContinuationMacros` names fragments appended to an expression, such as operators and operands, a member-call chain, or a supplied call argument list. Several continuations can follow one another. They stay attached to the expression rather than forming separate items.

Without configuration, `MORE_OPTIONS` below becomes a separate namespace item:

<!-- .cpp-format
MacroCategories:
  ExpressionContinuationMacros:
    - MORE_OPTIONS
-->
```cpp
REGISTER_BENCHMARK(Run) MORE_OPTIONS;
```

**Parse failure:** an argument-list macro after a qualified function name cannot form an ordinary C++ call without configuration.

<!-- .cpp-format
MacroCategories:
  ExpressionContinuationMacros:
    - ARGUMENTS
-->
```cpp
constexpr auto value = ns::Build ARGUMENTS;
```

Continuations compose with arbitrary expressions and may take arguments themselves:

<!-- .cpp-format
MacroCategories:
  ExpressionContinuationMacros:
    - ADD
    - PLUS_ONE
-->
```cpp
#define ADD(value) +(value)
#define PLUS_ONE +1

int total = 1 ADD(2) PLUS_ONE;
```

### TypeSpecifierMacros

`TypeSpecifierMacros` names macro calls that produce a C++ type specifier at the use site. They compose after declaration modifiers and after `typename` in a dependent type.

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

`PreprocessorArgumentMacros` names macros whose arguments are preprocessing-token sequences rather than C++ syntax. Use it only when the invocation deliberately inspects or transforms its arguments as tokens, for example a test helper that stringizes an unexpanded macro invocation.

The outer call remains a structured list that the formatter can split. The complete call composes in expression and type-specifier positions as well as namespace and class items, including inside structured macro replacements. `ItemMacros` additionally fixes its complete-item or list-fragment role. Within each argument, recursively nested parentheses are recognized while the complete preprocessing-token sequence is preserved as one formatter atom. Only parentheses protect an inner comma from separating outer arguments.

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

The first argument may be a declaration that also parses as a chained call. In `ASSERT_NO_THROW` below, `Result (function)(Argument)` declares a function returning `Result`; without this category, it formats as a chained call with no space after `Result`.

<!-- .cpp-format
MacroCategories:
  StatementArgumentMacros:
    - ASSERT_NO_THROW
-->
```cpp
void Check() { ASSERT_NO_THROW(Result (function)(Argument)); }
```

**Parse failures:** an initialized variable declaration need not fit the shared call-argument grammar. Each invocation below fails to parse without this category.

<!-- .cpp-format
MacroCategories:
  StatementArgumentMacros:
    - EXPECT_THROW
-->
```cpp
void Check() {
    EXPECT_THROW(auto value = Read(), Error);
    EXPECT_THROW(ns::Value value(input), Error);
}
```

The first argument can also be a complete block:

<!-- .cpp-format
MacroCategories:
  StatementArgumentMacros:
    - EXPECT_DEATH
-->
```cpp
void CheckChild() {
    EXPECT_DEATH(
        {
            RunChildProcess();
        },
        "signal"
    );
}
```

## Macro Definitions

### Structured replacements

Structured replacements are parsed and formatted recursively, including type specifiers and declaration fragments. The formatter owns their complete layout and adds continuation backslashes after formatting.

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
