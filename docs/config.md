# Formatter Configuration

This document specifies formatter configuration and ignore files consumed by `strictfmt`.

Formatter configuration is intentionally narrow and does not expose style policy knobs. Layout decisions are fixed in formatter source and documented in [format.md](format.md).

## Discovery and inheritance

When `--style` is omitted, `strictfmt` searches upward from each formatted file for `.cpp-format`. For a file dump mode, discovery starts at the dumped source file. For `--stdin`, including stdin dump modes, discovery starts at the current working directory. `--style <path>` uses the provided formatter configuration path for every input. Formatting file paths are still checked against the nearest ignore file found by walking upward from each formatted file.

`Inherit: Parent` makes a `.cpp-format` file inherit from the next `.cpp-format` found by searching upward from the formatter configuration file's parent directory. Explicit `--style <path>` formatter configuration files use the same parent search rooted at the explicit file. If no parent `.cpp-format` exists, inheritance starts from built-in defaults. Local scalar keys override inherited scalar keys. Lists of names always merge with inherited entries, retaining exact duplicates once; this applies to every `MacroCategories` category and `StreamShift.ConfigurationMethods`. `IncludeCategories` replaces the inherited list. Nested maps inherit categories that are not specified locally.

## .cpp-format

The `.cpp-format` file uses the formatter's YAML-like subset: blank lines, `---`, `...`, and comments are ignored; comments start with `#` outside single or double quotes; scalars may be unquoted, single quoted, or double quoted; lists use indented `- value` entries. Unknown keys are ignored by the formatter.

Supported top-level keys:

- `Inherit`: optional `Parent`, enabling parent `.cpp-format` inheritance.
- `ColumnLimit`: integer target column for formatter-owned wrapping. The default is `120`.
- `IndentWidth`: integer spaces per indentation level. The default is `4`.
- `TabWidth`: integer tab display width. The default is `4`.
- `IncludeCategories`: optional ordered list of include groups. Each entry requires `Regex`, and may set `Priority`; priorities sort ascending and default to list order. Regexes match the normalized include target with delimiters, such as `'<vector>'` or `'"util/path.h"'`.
- `MainIncludeChar`: `Quote` (the default) considers quoted includes for main-header detection; `AngleBracket` considers angle-bracket includes.
- `IncludeIsMainRegex`: allowed suffix regex for [main-header detection](#main-header-detection). The default is `(Test)?$`.
- `MacroCategories`: macro and macro-like runtime parser roles. See [macro.md](macro.md) for details.
- `StreamShift`: stream insertion/extraction configuration.

Example:

```yaml
---
Inherit: Parent
ColumnLimit: 120
IndentWidth: 4
TabWidth: 4

IncludeCategories:
  - Regex: '^<.*>$'
    Priority: 1
  - Regex: '^".*"$'
    Priority: 2
MainIncludeChar: Quote
IncludeIsMainRegex: '(Test)?$'

MacroCategories:
  RawMacroDefinitions:
    - GENERATED_TABLE
  BareIdentifierMacros:
    - CALLBACK
  DeclarationPrefixMacros:
    - API_EXPORT
  CallSyntaxMacros:
    - TEST
  SemicolonlessCallMacros:
    - DIAGNOSTIC_PUSH
  StatementArgumentMacros:
    - EXPECT_THROW
  TypeSpecifierMacros:
    - TYPE_OF
  PreprocessorArgumentMacros:
    - EXPECT_EXPANSION

StreamShift:
  ConfigurationMethods:
    - std::boolalpha
    - std::setw
```

### Main-header detection

Only the first include run in files ending with `.c`, `.cc`, `.cpp`, `.c++`, `.cxx`, `.m`, or `.mm` is eligible. The first matching include with a positive or unmatched category receives priority `0`; configured nonpositive priorities are retained, and encountering priority `0` ends the search. Header files do not receive main-header priority.

Matching ignores directories and letter case. Append `IncludeIsMainRegex` to the escaped header stem and search the source stem, not the reverse: by default, `"widget.h"` is a main-header candidate in `widget.cpp` and `widgetTest.cpp`. Suffixes match partially unless anchored: `''` permits any suffix, `'$'` requires an exact stem, and `'(_test)?$'` also accepts `widget_test.cpp`. For compound source extensions, match the basename before the first non-leading dot (`widget.h` with `widget.cu.cc`); an exact full-stem match also accepts compound header names (`widget.proto.h` with `widget.proto.cc`).

### StreamShift

`StreamShift.ConfigurationMethods` lists manipulators that bind to the following shifted value. The formatter keeps the configured manipulator sequence and its value together instead of choosing a break between them.

<!-- .cpp-format
StreamShift:
  ConfigurationMethods:
    - std::boolalpha
    - std::setw
-->
```cpp
auto configured = stream << std::boolalpha << enabled << std::setw(8) << value;
```

## .cpp-format-ignore

`.cpp-format-ignore` is a plain line-based ignore file. Blank lines and comments are ignored with the same comment stripping as `.cpp-format`; backslashes are normalized to slashes; repeated leading `./` and trailing slashes are removed; matching is case-insensitive.

An entry without `/` matches any path component below the ignore file directory. An entry with `/` matches that relative path or any child path below it. Glob patterns and negation are not supported.

Example:

```text
vendor
build
```

`vendor` ignores any directory or file component named `vendor`.
