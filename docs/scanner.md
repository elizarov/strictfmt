# Custom Scanner

This document owns the architecture of the custom tree-sitter scanner used by `strictfmt`.

## Purpose

Tree-sitter generates the C++ parser from `vendor/tree-sitter/tree-sitter-cpp/grammar.js`. The custom scanner in `vendor/tree-sitter/tree-sitter-cpp/src/scanner.c` supplies only the external tokens that cannot be expressed safely as static generated-lexer rules.

The scanner is part of the grammar target. Normal builds compile it together with `vendor/tree-sitter/tree-sitter-cpp/src/parser.c`.

## Architecture

`grammar.js` declares scanner-owned tokens in its `externals` list. During parsing, tree-sitter calls `tree_sitter_cpp_external_scanner_scan` with a `valid_symbols` array that says which external tokens are accepted in the current parser state. The scanner either returns one of those tokens or declines and lets the generated lexer continue.

The scanner uses four inputs:

- The source stream exposed by tree-sitter's `TSLexer`.
- The parser-state-specific `valid_symbols` array.
- Small scanner payload state for raw string delimiters.
- Formatter macro category configuration, exposed through `strictfmt_tree_sitter_cpp_macro_category_matches`.

`src/format/impl/format_model_parse.cpp` owns the callback bridge from parser to formatter configuration. `ParseFormatModel` installs a thread-local `FormatterConfig` for the parse, and the scanner calls back into that config when it needs to know whether an identifier belongs to `RawMacroDefinitions`, `BareIdentifierMacros`, or `CallSyntaxMacros`.

The scanner checks higher-risk stateful tokens before it skips scanner-local whitespace. In particular, preprocessor directive endings must be recognized before any whitespace skipping, otherwise a bare newline that should end a directive can disappear as ordinary whitespace.

## External Tokens

### Raw Strings

`raw_string_delimiter` and `raw_string_content` implement C++ raw string literals. The scanner records the opening delimiter, scans content until the matching closing delimiter, and serializes the delimiter payload through tree-sitter's external scanner API.

A generated token rule cannot express "remember this delimiter and later stop only at the same delimiter" as one static regular expression in the grammar.

### Macro Category Identifiers

The scanner owns these identifier tokens:

- `raw_macro_definition_identifier`
- `bare_macro_identifier`
- `suffix_macro_identifier`
- `call_syntax_macro_identifier`

The scanner reads a normal C/C++ identifier and then asks the formatter configuration whether the identifier belongs to the relevant macro category. This keeps macro categories runtime-configurable while the generated parser stays static.

The grammar uses these tokens to separate definition-side macro behavior from use-side macro behavior, as specified in [macro.md](macro.md).

### Conditional Macro Function Headers

`conditional_macro_function_header` recognizes the supported conditional macro-function header shape where preprocessor branches select complete call-syntax macro headers before one shared function body. The scanner can consume the guarded header as one external token while still using configured `CallSyntaxMacros` to validate the branch headers.

This token is intentionally narrow. General conditional compilation structure is still represented by grammar productions and formatted according to [preprocessor.md](preprocessor.md).

### Preprocessor Directive Newlines

`_preproc_directive_end` and `_line_break_whitespace` split bare line breaks into two roles:

- `_preproc_directive_end` is returned when the parser is currently ending a preprocessor directive.
- `_line_break_whitespace` is returned as hidden whitespace everywhere else.

Backslash-newline remains ordinary grammar `extras` whitespace. That is what makes structured macro continuation placement inert: inside a structured macro replacement, a continuation backslash does not create a syntax node, and the replacement ends only at the first bare preprocessor directive newline.

Raw macro replacements are the exception. They are scanned as raw replacement text by grammar token rules and then printed by the raw macro formatter, preserving the original physical continuation-line structure as specified in [macro.md](macro.md).

## Why Not Only the Generated Scanner?

The tree-sitter generated lexer is excellent for static token rules, but these scanner features are not static:

- Macro categories depend on the active `.cpp-format` configuration, so identifier classification requires a runtime callback.
- Raw string parsing needs scanner payload state shared between delimiter and content tokens.
- Preprocessor line breaks must be hidden whitespace in normal code but visible directive-ending tokens in specific parser states.
- Conditional macro-function headers combine preprocessor line scanning with runtime-configured call-syntax macro identifiers.

Encoding those cases as generated grammar tokens would either lose runtime configurability, duplicate scanner-like state in grammar productions, or force layout artifacts such as macro continuation gaps into the syntax tree. `strictfmt` avoids that. The scanner owns lexical facts; the pretty printer owns structured macro whitespace and continuation placement.

## Maintenance Rules

- Add scanner tokens only for lexical or runtime-configured facts that cannot be expressed as static grammar tokens.
- Do not represent structured macro continuation gaps as syntax. Backslash-newline inside structured macro replacements is whitespace.
- Keep raw macro preservation in the raw macro path, not in structured macro parsing.
- Keep scanner token names private with a leading underscore when the format model should not expose them.
- After changing `grammar.js` or scanner external tokens, regenerate the generated parser with `python3 tools/regenerate_tree_sitter_grammar.py`.
- Add focused parser/formatter tests for every scanner behavior that can affect formatting or parse recovery.
