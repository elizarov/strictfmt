# Known issues

This document tracks known limitations and planned work.

## Conditional leading commas are not supported in all lists (CONDITIONAL_LEADING_COMMAS)

Current behavior: Branch-owned leading separator commas are limited to the list positions specified in [preprocessor.md](preprocessor.md).

Planned work: Support branch-owned leading separator commas whenever a conditional branch follows an existing item in any supported comma-separated list.

## `clang-format` control comments are ignored (IGNORED_FORMAT_COMMENTS)

Current behavior: `// clang-format off` and `// clang-format on` comments remain in the source but do not affect formatting.

Planned work: Decide whether to honor these comments. If strictfmt does not honor them, remove them from formatted source so they do not misleadingly imply that formatting is disabled.

## Comma normalization remains an open issue (TRAILING_COMMA_NORMALIZATION)

Current behavior: Trailing commas in brace lists are normalized from the selected layout, which causes many changes in real code.

Planned work: Decide whether to further tune the rule or stop normalization and preserve trailing commas as written in the source.

## Keywords can occupy a separate line before their expressions (DETACHED_KEYWORD)

Current behavior: A [value-owning keyword](glossary.md#value-owning-keyword) can occupy its own line, keeping the following expression compact.

Planned work: Decide whether to keep this break opportunity to preserve a compact next line, or remove it and wrap the expression instead.

Current formatting (57-column limit):

<!-- .cpp-format
ColumnLimit: 57
-->
```cpp
auto f() {
    Prepare();
    return
        MakeValue(first_argument, second_argument, mode);
}
```

Alternative formatting:

```text
auto f() {
    Prepare();
    return MakeValue(
        first_argument, second_argument, mode
    );
}
```

## Function names can be indented below their return types (DECLARATOR_STAIRCASE)

Current behavior: Long function signatures may split after the return type, leaving the function name indented on the next line.

Planned work: Decide whether to keep this layout or prefer keeping the return type with the function name and wrapping parameters.

Current formatting (60-column limit):

<!-- .cpp-format
ColumnLimit: 60
-->
```cpp
Result
    BuildValue(const Input& input, const Settings& settings)
{
    Check(input);
    return Convert(input, settings);
}
```

Alternative formatting:

```text
Result BuildValue(
    const Input& input, const Settings& settings
) {
    Check(input);
    return Convert(input, settings);
}
```
