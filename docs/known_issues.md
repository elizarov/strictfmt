# Known issues

This document tracks known limitations and planned work.

- [Numeric table row grouping (TABLE_ROW_GROUPING)](#numeric-table-row-grouping-table_row_grouping)
- [Dense compact bodies (DENSE_COMPACT_BODIES)](#dense-compact-bodies-dense_compact_bodies)
- [Control-body braces across conditionals (CONDITIONAL_BODY_BRACES)](#control-body-braces-across-conditionals-conditional_body_braces)
- [Conditional leading commas (CONDITIONAL_LEADING_COMMAS)](#conditional-leading-commas-conditional_leading_commas)
- [Empty branch attachment (EMPTY_BRANCH_ATTACHMENT)](#empty-branch-attachment-empty_branch_attachment)
- [Detached keywords (DETACHED_KEYWORDS)](#detached-keywords-detached_keywords)
- [Function declarator indentation (DECLARATOR_INDENTATION)](#function-declarator-indentation-declarator_indentation)
- [Nested sequence indentation (SEQUENCE_INDENTATION)](#nested-sequence-indentation-sequence_indentation)

## Numeric table row grouping (TABLE_ROW_GROUPING)

Current behavior: The [list layout rules](format.md#lists) expand flat numeric tables to one element per line when they cannot fit on one line. This obscures rows and columns and greatly lengthens large tables.

Open question: Preserve authored table rows or introduce a packing policy for tabular data?

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

## Dense compact bodies (DENSE_COMPACT_BODIES)

Current behavior: The [compact-body rule](format.md#declaration-and-control-headers) puts an eligible body beside its header whenever both fit. With a long signature, the action can be hard to spot.

Open question: Limit compact bodies to shorter headers or preserve an authored break before the body statement?

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

## Control-body braces across conditionals (CONDITIONAL_BODY_BRACES)

Current behavior: The formatter reports an error when a control body cannot safely be enclosed in braces under [conditional compilation](preprocessor.md#supported-conditional-compilation-and-local-includes), preserving the input.

Planned work: Place closing braces in each alternative before the following items, preserving their scope. Support shared endings of nested blocks and resolve branch-dependent `else` binding before inserting enclosing braces.

Currently rejected; `Finish()` must remain outside both `if` statements:

```text
void Run() {
    if (outer)
        if (inner) {
            Work();
#if FEATURE
        }
    Finish();
#else
        }
#endif
}
```

## Conditional leading commas (CONDITIONAL_LEADING_COMMAS)

Current behavior: Leading commas inside conditional branches are supported only in the list positions specified in [preprocessor.md](preprocessor.md).

Planned work: Support leading commas whenever a conditional branch follows an existing item in any supported comma-separated list.

Currently rejected (conditional enum entry):

```text
enum Kind {
    First
#if EXTRA
    , Second
#endif
};
```

Supported form using trailing commas:

```cpp
enum Kind {
    First,
#if EXTRA
    Second,
#endif
};
```

## Empty branch attachment (EMPTY_BRANCH_ATTACHMENT)

Current behavior: An empty control-flow body stays as `{}`, with a following `else` or `catch` on a separate line.

Open question: Allow an empty body to expand when needed to keep a following `else` or `catch` attached?

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

## Detached keywords (DETACHED_KEYWORDS)

Current behavior: A [value-owning keyword](glossary.md#value-owning-keyword) can occupy its own line, keeping the following expression compact.

Open question: Keep this break to preserve a compact expression on the next line, or wrap the expression instead?

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

## Function declarator indentation (DECLARATOR_INDENTATION)

Current behavior: Long function signatures may split after the return type, leaving the function name indented on the next line.

Open question: Keep this layout or keep the return type with the function name and wrap the parameters?

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

## Nested sequence indentation (SEQUENCE_INDENTATION)

Current behavior: Within a list item, a broken sequence indents its continuations beyond its first element. This distinguishes the item from neighboring comma-separated items but obscures peer alignment in sums, string fragments, and repeated parenthesized macro tuples.

Open question: When a sequence occupies a complete list item and starts on its own line, should all its elements share the item's indentation? This would improve regularity and save width while relying more on commas to distinguish neighboring items.

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

Alternative formatting:

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
