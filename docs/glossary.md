# Glossary

This document owns shared terminology used across the `strictfmt` documentation.

- **atomic group**: A formatter unit with no internal break opportunities: an empty delimiter pair, function-pointer declarator group, parenthesized callee, compiler declaration prefix, `__declspec` group, operator function name, or angle-bracket token that is not a template argument list.
- **break choice**: A compact or split layout decision selected by the break optimizer for one break model node.
- **break model**: The formatter-owned tree of possible compact and split layouts for one formatted segment. In code this is `FormatBreakModel`.
- **break optimizer**: The dynamic-programming solver that chooses break choices for a break model under the configured column limit. In code this is `SolveFormatBreaks`.
- **break solver**: The developer-facing implementation of the break optimizer in `src/format/impl/format_break_solver.h|cpp`; see [break_solver.md].
- **call-like syntax**: Syntax in which a function-like name is followed by an argument or parameter list.
- **callable header**: The signature portion of a function-like construct, excluding its body and any enclosing declaration or expression that owns it.
- **command-line input**: A source file, file list, recursive root, or stdin stream passed to the `strictfmt` executable.
- **complete item**: A syntactic item that can stand at the surrounding grammar level.
- **conditional compilation**: Source branches controlled by preprocessor conditional directives.
- **conditional opener**: A directive that starts conditional compilation: `#if`, `#ifdef`, or `#ifndef`.
- **control statement**: A conditional, loop, or switch statement whose body is subject to control-brace normalization.
- **declaration-like item**: A source item parsed and laid out as a declaration, including configured macro invocations that occupy declaration positions.
- **delimiter group**: A matched pair of formatter-owned parentheses, brackets, initializer braces, or parsed template angle brackets. Code-block braces are not delimiter groups.
- **format model**: The normalized formatter-owned representation built from the syntax tree. In code this is `FormatModel`, whose nodes are `SyntaxNode` values.
- **formatted segment**: A run of print tokens between mandatory line breaks that can be sent to the break optimizer.
- **formatted source**: The complete text emitted by the formatter.
- **formatter configuration**: The effective `FormatterConfig` loaded from `.cpp-format` files, inheritance, and built-in defaults.
- **ignore file**: A `.cpp-format-ignore` file that controls which paths the formatter skips.
- **include run**: A contiguous include area that the formatter may preserve or regroup according to formatter configuration.
- **line break opportunity**: An optional boundary where the break optimizer may choose a split layout.
- **line ending style**: The LF, CRLF, or CR newline spelling preserved for formatted source when the input uses one style.
- **list**: A formatter-owned grammar list whose separators, when present, are commas. Comma-operator chains are not lists.
- **macro category**: A formatter configuration list that tells the grammar which macro identifiers use special parsing roles.
- **mandatory line break**: A structural boundary that the formatter always emits before optional wrapping is considered.
- **pretty printer**: The formatter component that turns the format model and break choices into formatted source.
- **preprocessor directive**: A source line that begins with a recognized `#keyword` command. The directive keyword uses canonical spelling in formatted source.
- **print token**: The token stream derived from the format model for spacing, break model construction, and emission. In code this is `PrintToken`.
- **recursive discovery**: Directory traversal from `-r` or `--recursive` roots to find supported source files while applying ignore files.
- **source code module**: A paired `.cpp` and `.h` implementation unit, or a single `.cpp` or `.h` file when no pair exists.
- **source file**: One C or C++ file selected for formatting.
- **source item**: A neighboring grammar-level item in source order.
- **source layout**: The formatter-owned physical arrangement of source text.
- **source text**: The text read from one source file or stdin before formatting.
- **syntax tree**: The tree-sitter parse tree for source text.
- **value-owning keyword**: A keyword whose following value has an ordinary break opportunity: `return`, `co_return`, `throw`, or `co_yield`.
