# Glossary

This document owns shared terminology used across the `strictfmt` documentation.

- **break choice**: A compact or split layout decision selected by the break optimizer for one break model node.
- **break model**: The formatter-owned tree of possible compact and split layouts for one formatted segment. In code this is `FormatBreakModel`.
- **break optimizer**: The dynamic-programming solver that chooses break choices for a break model under the configured column limit. In code this is `SolveFormatBreaks`.
- **break solver**: The developer-facing implementation of the break optimizer in `src/format/impl/format_break_solver.h|cpp`; see [break_solver.md].
- **command-line input**: A source file, file list, recursive root, or stdin stream passed to the `strictfmt` executable.
- **complete item**: A syntactic item that can stand at the surrounding grammar level, such as a complete declaration, statement, enum entry, include, macro definition, or list item.
- **conditional compilation**: Preprocessor-controlled source branches introduced by directives such as `#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, and `#endif`.
- **format model**: The normalized formatter-owned representation built from the syntax tree. In code this is `FormatModel`, whose nodes are `SyntaxNode` values.
- **formatted segment**: A run of print tokens between mandatory line breaks that can be sent to the break optimizer.
- **formatted source**: The complete text emitted by the formatter after parsing, modeling, break optimization, and pretty printing.
- **formatter configuration**: The effective `FormatterConfig` loaded from `.cpp-format` files, inheritance, and built-in defaults.
- **ignore file**: A `.cpp-format-ignore` file that controls which paths the formatter skips.
- **include run**: A contiguous include area that the formatter may preserve or regroup according to formatter configuration.
- **line break opportunity**: An optional boundary where the break optimizer may choose a split layout.
- **line ending style**: The LF, CRLF, or CR newline spelling preserved for formatted source when the input uses one style.
- **macro category**: A formatter configuration list that tells the grammar which macro identifiers use special parsing roles.
- **mandatory line break**: A structural boundary that the formatter always emits before optional wrapping is considered.
- **pretty printer**: The formatter component that turns the format model and break choices into formatted source.
- **print token**: The token stream derived from the format model for spacing, break model construction, and emission. In code this is `PrintToken`.
- **recursive discovery**: Directory traversal from `-r` or `--recursive` roots to find supported source files while applying ignore files.
- **source code module**: A paired `.cpp` and `.h` implementation unit, or a single `.cpp` or `.h` file when no pair exists.
- **source file**: One C or C++ file selected for formatting.
- **source item**: A neighboring declaration, statement, preprocessor directive, comment, or other grammar-level item in source order.
- **source layout**: The whitespace, line break, indentation, wrapping, and blank-line arrangement of formatted source.
- **source text**: The text read from one source file or stdin before formatting.
- **syntax tree**: The tree-sitter parse tree for source text.
