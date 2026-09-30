# Known issues

This document tracks known limitations and planned work.

## Numeric tables lose row grouping (TABULAR_DATA_EXPANSION)

Current behavior: The [list layout rules](format.md#lists) expand flat numeric tables to one element per line when they cannot fit on one line. This obscures rows and columns and greatly lengthens large tables.

Open question: Decide whether to preserve authored table rows or introduce a packing policy for tabular data.

Current formatting (40-column limit):

<!-- .cpp-format
ColumnLimit: 40
-->
```cpp
const int table[] = {
    0x00,
    0x01,
    0x02,
    0x03,
    0x10,
    0x11,
    0x12,
    0x13,
};
```

Alternative formatting:

```text
const int table[] = {
    0x00, 0x01, 0x02, 0x03,
    0x10, 0x11, 0x12, 0x13,
};
```

## Compact bodies can be hard to scan (DENSE_COMPACT_BODIES)

Current behavior: The [compact-body rule](format.md#declaration-and-control-headers) puts an eligible body beside its header whenever both fit. With a long signature, the action can be hard to spot.

Open question: Decide whether to limit compact bodies to shorter headers or preserve an authored break before the body statement.

Current formatting:

```cpp
void Save(const Input& input, const Settings& settings) { WriteToDisk(input, settings); }
```

Alternative formatting:

```text
void Save(const Input& input, const Settings& settings) {
    WriteToDisk(input, settings);
}
```

## Adding control-body braces across conditionals (CONDITIONAL_BODY_BRACES)

Current behavior: The restriction on enclosing control bodies under [conditional compilation](preprocessor.md#supported-conditional-compilation-and-local-includes) is reported as an error, preserving the input.

Planned work: Place new closing braces in each alternative before the following items, preserving their scope and supporting the resulting shared endings of nested blocks. Resolve branch-dependent `else` binding before inserting enclosing braces.

## Conditional leading commas are not supported in all lists (CONDITIONAL_LEADING_COMMAS)

Current behavior: Branch-owned leading separator commas are limited to the list positions specified in [preprocessor.md](preprocessor.md).

Planned work: Support branch-owned leading separator commas whenever a conditional branch follows an existing item in any supported comma-separated list.

## Empty branches detach following keywords (EMPTY_BRANCH_ATTACHMENT)

Current behavior: An empty control-flow branch is kept as `{}` and ends its line before a following attachment keyword such as `else` or `catch`.

Open question: Decide whether the mandatory “keep `{}`” rule needs a targeted exclusion for branches followed by an attachment keyword, allowing the empty body to expand so the keyword remains attached.

Current formatting:

```cpp
void Check() {
    if (ready) {}
    else {
        Wait();
    }
}
```

Alternative formatting:

```text
void Check() {
    if (ready) {
    } else {
        Wait();
    }
}
```

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

## Nested sequences use staircase indentation (SEQUENCE_STAIRCASE)

Current behavior: Within a list item, a broken sequence indents its continuations beyond its first element. This distinguishes the item from neighboring comma-separated items, but obscures peer alignment in sums, string fragments, and repeated parenthesized macro tuples.

Open question: When a sequence occupies a complete list item and starts on its own line, should all its elements share the item's indentation? This would improve regularity and save width, while relying more on commas to distinguish neighboring items.

Current formatting (40-column limit):

<!-- .cpp-format
ColumnLimit: 40
-->
```cpp
void Example() {
    Use(
        mode,
        first_component +
            second_component +
            third_component,
        flags
    );
}
```

Alternative formatting (not yet decided):

```text
void Example() {
    Use(
        mode,
        first_component +
        second_component +
        third_component,
        flags
    );
}
```
