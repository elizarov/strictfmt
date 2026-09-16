# Macro formatting

## Macro Categories

A macro's syntactic role cannot always be inferred without expansion, so arbitrary code with macros cannot be parsed reliably. Many macros [need no configuration](#macros-without-configuration); use a category when their intended role is not recognized:

- [DeclarationModifierMacros](#declarationmodifiermacros): annotations within declarations, as in `API_EXPORT int value;`.
- [StatementArgumentMacros](#statementargumentmacros): a statement or declaration in the first argument, as in `EXPECT_THROW(auto x = Read(), Error)`.
- [ItemMacros](#itemmacros): separate declarations, statements, or list fragments, as in `BEGIN_NAMESPACE`.
- [MethodDeclarationMacros](#methoddeclarationmacros): method-signature arguments, as in `MOCK_METHOD(void, Save, (T* value))`.
- [StatementPrefixMacros](#statementprefixmacros): a prefix attached to the next statement, as in `DISCARD_RESULT message.Parse();`.
- [ExpressionContinuationMacros](#expressioncontinuationmacros): a fragment attached to the preceding expression, as in `Register() OPTIONS`.
- [PreprocessorArgumentMacros](#preprocessorargumentmacros): arguments whose token spelling matters, as in `STRINGIZE(a*b)`.
- [TypeSpecifierMacros](#typespecifiermacros): a call supplying a type, as in `TYPE_OF(T)* value;`.

Categories apply at use sites, including inside macro replacements; they do not describe how a macro is defined. Configure names in [`.cpp-format`](config.md), optionally followed by `*` to match a prefix such as `ATTRIBUTE*`; no other glob syntax is supported. Macros may have argument lists regardless of their syntactic role.

The examples below show configured output and explain what changes without configuration.

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

Annotations on variables, parameters, and methods use the same category:

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

### StatementArgumentMacros

`StatementArgumentMacros` lets the first argument contain statements and declarations, allowing the last one to omit its semicolon. Remaining arguments use the shared call-argument grammar. Use it for assertion-style macros when a declaration fails to parse or is mistaken for an expression; ordinary expression arguments do not need it.

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

### ItemMacros

`ItemMacros` separates complete declarations or statements from neighboring code, and identifies fragments of enum, initializer, and template lists that supply their own separators. The same macro may still appear as an operand or argument.

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

Use this category for namespace wrappers or generated class members when they become attached to surrounding code. Without configuration, `BEGIN_NAMESPACE` below joins the test header as though it were a return type or annotation.

<!-- .cpp-format
MacroCategories:
  ItemMacros:
    - BEGIN_NAMESPACE
    - END_NAMESPACE
-->
```cpp
BEGIN_NAMESPACE

TEST(StoreTest, SavesValue) { SaveValue(); }

END_NAMESPACE
```

Configuration puts `ELEMENTS(X)` on its own line as a list fragment. Without it, the initializer below stays on one line because the call also parses as a single expression.

<!-- .cpp-format
MacroCategories:
  ItemMacros:
    - ELEMENTS
-->
```cpp
auto values = Pack{
    0,
    ELEMENTS(X)
};
```

### MethodDeclarationMacros

`MethodDeclarationMacros` names class-member macros taking a return type, method name, parenthesized parameter list, and optional parenthesized qualifier list, in that order. Configuration resolves ambiguous arguments according to these roles, including nested declarators.

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

### StatementPrefixMacros

`StatementPrefixMacros` attaches a modifier to the following complete statement or block. Several prefixes may nest. The prefix and its following statement form one control-flow body, including when braces are added to an enclosing control statement.

Configure macros that prefix unbraced statements: in `if (ready) FOR_EACH(items) Work();`, the macro and `Work()` must remain together inside the `if`. Braced calls follow the [unconfigured call-and-block rule](#macros-without-configuration).

In `DISCARD_RESULT *value;`, the prefix precedes a dereference expression. Without this category, `DISCARD_RESULT` is parsed as a type and `*` attaches to it as a pointer declarator.

<!-- .cpp-format
MacroCategories:
  StatementPrefixMacros:
    - DISCARD_RESULT
-->
```cpp
void Discard() { DISCARD_RESULT *value; }
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

### PreprocessorArgumentMacros

`PreprocessorArgumentMacros` preserves each argument as a preprocessing-token sequence instead of formatting it as C++ syntax. Use it when a macro depends on token spelling or grouping, such as stringizing its argument.

Without configuration, `a*b` below is formatted as `a * b`, changing the string produced by `STRINGIZE`.

<!-- .cpp-format
MacroCategories:
  PreprocessorArgumentMacros:
    - STRINGIZE
-->
```cpp
#define STRINGIZE(tokens) #tokens

const char* expression = STRINGIZE(a*b);
```

The outer argument list can still split across lines, but each argument remains one formatting unit. Only parentheses protect inner commas: `TOKENS(T<X, Y>, NEXT)` has three preprocessing arguments, whereas the default C++ parse keeps `T<X, Y>` together. This category controls arguments; combine it with `ItemMacros` or `ExpressionContinuationMacros` when the invocation also needs that role.

### TypeSpecifierMacros

`TypeSpecifierMacros` selects the type interpretation of a macro call where a declaration and an expression are both possible.

A macro call followed by `*` may declare a pointer or multiply expressions. Configuration makes `TYPE_OF(T)` the declared type below; without it, the formatter treats `*` as multiplication and puts spaces on both sides.

<!-- .cpp-format
MacroCategories:
  TypeSpecifierMacros:
    - TYPE_OF
-->
```cpp
void Declare() { TYPE_OF(T)* value; }
```

## Macros without configuration

Function and macro calls share one argument grammar, accepting expressions, types, function and template parameters, statement blocks or sequences, and empty or comment-only arguments. Parenthesized argument fragments use the same grammar recursively and can appear in adjacent sequences.

Calls also fit type-only positions, including aliases and function parameters.

An isolated identifier can supply a complete namespace or class item, or a template-list fragment, when it cannot form ordinary C++ syntax. Unknown modifiers are also accepted in class, struct, union, and template declaration headers, before constructor specifiers such as `explicit`, after configured declaration modifiers, and after function declarators or alias names. Configuration may still be needed to attach an identifier to the surrounding code.

Enum items may omit separating commas. Calls recognized as enum or braced initializer list fragments occupy separate lines, whether configured or not. Calls can also form statements without a trailing semicolon.

A macro call immediately followed by a `{ ... }` body is a statement prefix in statement positions and a function definition in declaration positions, including namespace and class scope. The body follows the corresponding statement or function layout rules. An inferred prefix and its block form one statement, including when braces are added to an enclosing control statement:

```cpp
TEST(StoreTest, SavesValue) { SaveValue(); }

void Visit(Items& items) {
    FOR_EACH(item, items) {
        Consume(item);
    }
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

## Macro Definitions

Do not add or remove braces anywhere inside a macro definition, including nested function and lambda bodies.

### Structured replacements

A structured replacement is a macro body parsed as syntax and formatted recursively, including type specifiers and declaration fragments. The formatter owns its complete layout and adds continuation backslashes after formatting.

Token pasting, nested macro calls whose arguments are preprocessing-token sequences, and balanced parenthesized preprocessing tokens remain explicit recursive grammar nodes. They may use token-level rather than C++ expression-level structure because macro expansion determines their eventual C++ role, but they must not be collapsed into an opaque formatter leaf.

A structured macro definition has two header-level forms. If the complete definition fits on one physical line, it stays on that line. Otherwise, the formatter breaks after the complete definition header and starts the replacement one continuation indentation level beyond its column-zero header, independently of the enclosing C++ scope.

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

Between tokens, original continuation-line boundaries do not constrain the structured layout. The replacement ends at the first unspliced directive newline; the formatter chooses new line breaks and continuation backslashes.

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
