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

`src/format/impl/format_model_parse.cpp` owns the callback bridge from parser to formatter configuration. `ParseFormatModel` installs a thread-local `FormatterConfig` for the parse, and the scanner calls back into that config when it needs to know whether an identifier belongs to `RawMacroDefinitions`, `BareIdentifierMacros`, `DeclarationPrefixMacros`, `CallSyntaxMacros`, `SemicolonlessCallMacros`, `StatementArgumentMacros`, `TypeSpecifierMacros`, or `PreprocessorArgumentMacros`.

The scanner checks higher-risk stateful tokens before it skips scanner-local whitespace. In particular, preprocessor directive endings must be recognized before any whitespace skipping, otherwise a bare newline that should end a directive can disappear as ordinary whitespace.

## External Tokens

### Raw Strings

`raw_string_delimiter` and `raw_string_content` implement C++ raw string literals. The scanner records the opening delimiter, scans content until the matching closing delimiter, and serializes the delimiter payload through tree-sitter's external scanner API.

A generated token rule cannot express "remember this delimiter and later stop only at the same delimiter" as one static regular expression in the grammar.

### Macro Category Identifiers

The scanner owns these identifier tokens:

- `raw_macro_definition_identifier`
- `raw_macro_replacement`
- `bare_macro_identifier`
- `declaration_prefix_macro_identifier`
- `call_syntax_macro_identifier`
- `semicolonless_call_macro_identifier`
- `statement_argument_macro_identifier`
- `type_specifier_macro_identifier`
- `preprocessor_argument_macro_identifier`

The scanner reads a normal C/C++ identifier and then asks the formatter configuration whether the identifier belongs to the relevant macro category. This keeps macro categories runtime-configurable while the generated parser stays static.

`raw_macro_replacement` captures the rest of a configured raw macro definition once the grammar has accepted the raw macro name and parameters. This is scanner-owned so the raw macro path can preserve a continuation backslash that appears immediately after the macro name or parameter list, before ordinary structured-macro continuation whitespace can consume it.

The scanner classifies identifiers by configured macro category. [macro.md](macro.md) specifies the categories and their supported grammar uses. For `PreprocessorArgumentMacros`, the scanner owns only the configured identifier token; the grammar recursively balances the invocation's parentheses and separates its preprocessing-token arguments.

### Token-Paste Prefixes

`macro_token_paste_identifier_prefix` and `macro_token_paste_number_prefix` recognize a normal
identifier or preprocessing number only when the next preprocessing operator is `##`. Each token
ends before that operator; the grammar still represents every `##` and every pasted operand as
separate structured syntax. This narrow lexical lookahead distinguishes a pasted function name such
as `Get##name##Location` from an ordinary macro-generated declarator such as
`CONCAT_TOKEN(a, b)(...)`. Requiring the same lookahead for a numeric prefix prevents a C++
user-defined literal such as `10ms` from becoming an incomplete paste candidate. Neither token makes
the whole pasted name opaque to the formatter.

### Whitespace And Preprocessor Directive Newlines

`_preproc_directive_end` and `_line_break_whitespace` split scanner-visible whitespace into two roles:

- `_preproc_directive_end` is returned when the parser is currently ending a preprocessor directive.
- `_line_break_whitespace` is returned as hidden whitespace everywhere else.

When a configured macro identifier follows leading line indentation in a parser state that also accepts `_line_break_whitespace`, that token supplies a narrow horizontal-only boundary. This prevents the generated lexer from consuming an ordinary identifier before the runtime scanner can classify the macro. The same boundary is allowed after non-leading horizontal whitespace for configured bare-identifier, declaration-prefix, type-specifier, and preprocessing-token-argument macros, plus configured call-syntax and statement-argument macros with a following argument list. Bare identifiers must remain classifiable in post-type, declarator-suffix, initializer, alias-suffix, and calling-convention positions; declaration-prefix macros can follow attributes or class keys; call-syntax macro calls can appear after another macro or inside a larger expression; type-specifier macros need the boundary after declaration keywords or modifiers; preprocessing-token calls must remain classifiable as expression atoms; statement-argument macros need it in unbraced and other same-line statement positions. Requiring a following argument list prevents a configured call-syntax spelling used merely as another macro's identifier argument from being misclassified. Preprocessor definition-name and replacement states are excluded, and every other horizontal gap remains owned by the generated lexer. The separate token boundary also ensures that skipped spaces do not become part of the external identifier's source range. Leading indentation needs explicit handling for the first macro in a preprocessor branch because the directive-ending token owns the preceding newline but leaves the branch body's indentation for the next scan. Semicolonless-call macros also use this leading boundary because their physical-line placement is part of their configured syntactic role.

Backslash-newline remains ordinary grammar `extras` whitespace. That is what makes structured macro continuation placement inert: inside a structured macro replacement, a continuation backslash does not create a syntax node, and the replacement ends only at the first bare preprocessor directive newline.

Raw macro replacements are the exception. They are scanned as raw replacement text and then printed by the raw macro formatter, preserving the original physical continuation-line structure as specified in [macro.md](macro.md).

## Why Not Only the Generated Scanner?

The tree-sitter generated lexer is excellent for static token rules, but these scanner features are not static:

- Macro categories depend on the active `.cpp-format` configuration, so identifier classification requires a runtime callback.
- A token-paste prefix requires lookahead past an identifier or preprocessing number while keeping the following `##` as a separate grammar token.
- Raw string parsing needs scanner payload state shared between delimiter and content tokens.
- Preprocessor line breaks must be hidden whitespace in normal code but visible directive-ending tokens in specific parser states.

Encoding those cases as generated grammar tokens would either lose runtime configurability, duplicate scanner-like state in grammar productions, or force layout artifacts such as macro continuation gaps into the syntax tree. `strictfmt` avoids that. The scanner owns lexical facts; the pretty printer owns structured macro whitespace and continuation placement.

## Maintenance Rules

- Add scanner tokens only for lexical or runtime-configured facts that cannot be expressed as static grammar tokens.
- Do not represent structured macro continuation gaps as syntax. Backslash-newline inside structured macro replacements is whitespace.
- Keep raw macro preservation in the raw macro path, not in structured macro parsing.
- Keep scanner token names private with a leading underscore when the format model should not expose them.
- After changing `grammar.js` or scanner external tokens, regenerate the generated parser with `python3 tools/regenerate_tree_sitter_grammar.py`.
- Add focused parser/formatter tests for every scanner behavior that can affect formatting or parse recovery.
