# Syntax ambiguities

This document specifies the treatment of C++ syntax ambiguities by `strictfmt`. The formatter has no symbol table and must resolve all ambiguities syntactically.

## Parenthesized Initializer Ambiguity

C++ block-scope declarations can use a token shape that is both a parenthesized direct initializer and a function declaration without type information.

```cpp
int product(a * b, c & d);  // direct initializer
ResultType local(FirstType* first, SecondType& second);  // function declaration
```

Parenthesized initialization is rare in project code, so the formatter defaults the declaration parse. Prefer braced initialization to remove ambiguity:

```cpp
int product{a * b, c & b};  // preferred direct initializer
```

Use parenthesized initialization only when braces have different C++ semantics, such as vector length construction:

```cpp
std::vector<int> values(count);  // vector with count values
std::vector<int> oneValue{count}; // vector with one value
```

When a parenthesized direct initializer needs expression operands that could parse as declarators, add extra parentheses around those operands. This is the preferred fix:

```cpp
std::vector<int> values((n * m));  // use iff parenthesized initialization must be used
```

## Expression Template Ambiguity

C++ expressions and templates can have the same token shape: 

```cpp
value < (min)(a, b) && value > (max)(a, b);  // expression
function<(min)(a, b) && flag>(max)(a, b);  // template function call 
```

Formatter parses expressions and templates using the following rules:

- Callable template shapes parse as template calls: `name<args>(...)`, `qualified::name<args>(...)`.
- Relational chains that do not form a callable template parse as expressions, e.g. `value < min || value > max` and `a < b > c`.
- Template argument lists prefer type-like arguments when a name could be either a type or a value.

If a non-type template argument expression can parse as a type-like argument, parenthesize the value argument. For example, `Size(A * B)` inside a template argument list parses as a type-like function declarator and formats with type-declarator spacing:

```cpp
using X = Box<Size(A* B)>;  // type-like function declaration as template argument
```

Write the value expression as:

```cpp
using X = Box<(Size(A * B))>;  // value as template argument
```

Parenthesize expression chains that look like callable templates:

```cpp
return (a < b) > (c);  // expression
```

Without those parentheses, `a < b > (c)` is parsed as the template call `a<b>(c)`.

## Conditional List Item Ambiguity

A conditional block inside a comma-separated list can contain either complete list items or an incomplete expression prefix that continues after `#endif`. Complete items use the structured conditional-list grammar. The recovery fallback for an incomplete argument expression is limited to branch code lines whose last significant token is a continuation operator, so it cannot shadow a formatted complete list item merely because that item is indented.

```cpp
Use(
#if ENABLE_FAST
    FastValue()
#else
    SlowValue()
#endif
);
```

An operator-terminated branch remains an unsupported expression fragment and may parse through the fallback:

```cpp
Use(
#if ENABLE_EXTRA
    kExtra |
#endif
    kBase
);
```
