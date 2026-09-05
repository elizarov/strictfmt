#include "tree_sitter/alloc.h"
#include "tree_sitter/parser.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wctype.h>

enum TokenType {
    RAW_STRING_DELIMITER,
    RAW_STRING_CONTENT,
    RAW_MACRO_DEFINITION_IDENTIFIER,
    RAW_MACRO_REPLACEMENT,
    MACRO_TOKEN_PASTE_IDENTIFIER_PREFIX,
    MACRO_TOKEN_PASTE_NUMBER_PREFIX,
    BARE_MACRO_IDENTIFIER,
    DECLARATION_PREFIX_MACRO_IDENTIFIER,
    CALL_SYNTAX_MACRO_IDENTIFIER,
    STATEMENT_ARGUMENT_MACRO_IDENTIFIER,
    TYPE_SPECIFIER_MACRO_IDENTIFIER,
    PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER,
    SEMICOLONLESS_CALL_MACRO_IDENTIFIER,
    PREPROC_DIRECTIVE_END,
    LINE_BREAK_WHITESPACE,
};

enum MacroCategory {
    MACRO_CATEGORY_RAW_DEFINITION = 0,
    MACRO_CATEGORY_BARE_IDENTIFIER = 1,
    MACRO_CATEGORY_CALL_SYNTAX = 2,
    MACRO_CATEGORY_STATEMENT_ARGUMENT = 3,
    MACRO_CATEGORY_DECLARATION_PREFIX = 4,
    MACRO_CATEGORY_TYPE_SPECIFIER = 5,
    MACRO_CATEGORY_PREPROCESSOR_ARGUMENT = 6,
    MACRO_CATEGORY_SEMICOLONLESS_CALL = 7,
};

/// The spec limits raw-string delimiters to 16 chars.
#define MAX_DELIMITER_LENGTH 16
#define MAX_MACRO_NAME_LENGTH 256

typedef struct {
    uint8_t delimiter_length;
    wchar_t delimiter[MAX_DELIMITER_LENGTH];
} Scanner;

extern bool strictfmt_tree_sitter_cpp_macro_category_matches(
    unsigned category,
    const char *text,
    unsigned length
);

static inline void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

static inline void advance_skip(TSLexer *lexer) { lexer->advance(lexer, true); }

static inline void reset(Scanner *scanner) {
    scanner->delimiter_length = 0;
    memset(scanner->delimiter, 0, sizeof scanner->delimiter);
}

static bool is_identifier_start(int32_t ch) { return ch == '_' || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'); }

static bool is_identifier_continue(int32_t ch) {
    return is_identifier_start(ch) || (ch >= '0' && ch <= '9');
}

static void skip_external_whitespace(TSLexer *lexer) {
    for (;;) {
        while (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\f') {
            advance_skip(lexer);
        }
        if (lexer->lookahead != '\\') {
            return;
        }
        advance_skip(lexer);
        if (lexer->lookahead == '\r') {
            advance_skip(lexer);
            if (lexer->lookahead == '\n') {
                advance_skip(lexer);
            }
            continue;
        }
        if (lexer->lookahead == '\n') {
            advance_skip(lexer);
            continue;
        }
        if (lexer->lookahead != ' ' && lexer->lookahead != '\t' && lexer->lookahead != '\f') {
            return;
        }
    }
}

static bool scan_identifier(TSLexer *lexer, char *name, unsigned *length) {
    if (!is_identifier_start(lexer->lookahead)) {
        return false;
    }

    unsigned used = 0;
    while (is_identifier_continue(lexer->lookahead)) {
        if (used + 1 >= MAX_MACRO_NAME_LENGTH) {
            return false;
        }
        name[used++] = (char)lexer->lookahead;
        advance(lexer);
    }
    name[used] = '\0';
    *length = used;
    return true;
}

static bool scan_preprocessing_number(TSLexer *lexer) {
    if (lexer->lookahead == '.') {
        advance(lexer);
        if (lexer->lookahead < '0' || lexer->lookahead > '9') {
            return false;
        }
    } else if (lexer->lookahead < '0' || lexer->lookahead > '9') {
        return false;
    }

    for (;;) {
        const int32_t ch = lexer->lookahead;
        if (is_identifier_continue(ch) || ch == '.' || ch == '\'') {
            advance(lexer);
            if ((ch == 'e' || ch == 'E' || ch == 'p' || ch == 'P') &&
                (lexer->lookahead == '+' || lexer->lookahead == '-')) {
                advance(lexer);
            }
            continue;
        }
        return true;
    }
}

static bool scan_macro_identifier(TSLexer *lexer, enum MacroCategory category) {
    char name[MAX_MACRO_NAME_LENGTH];
    unsigned length = 0;
    if (!scan_identifier(lexer, name, &length)) {
        return false;
    }
    lexer->mark_end(lexer);
    return strictfmt_tree_sitter_cpp_macro_category_matches(category, name, length);
}

static bool has_following_token_paste(TSLexer *lexer) {
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\f') {
        advance(lexer);
    }
    if (lexer->lookahead != '#') {
        return false;
    }
    advance(lexer);
    return lexer->lookahead == '#';
}

static bool has_token_paste_number_prefix(TSLexer *lexer) {
    return scan_preprocessing_number(lexer) && has_following_token_paste(lexer);
}

static bool has_following_argument_list(TSLexer *lexer) {
    for (;;) {
        while (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\f' ||
               lexer->lookahead == '\v' || lexer->lookahead == '\r' || lexer->lookahead == '\n') {
            advance_skip(lexer);
        }
        if (lexer->lookahead == '(') {
            return true;
        }
        if (lexer->lookahead == '\\') {
            advance_skip(lexer);
            if (lexer->lookahead == '\r') {
                advance_skip(lexer);
                if (lexer->lookahead == '\n') {
                    advance_skip(lexer);
                }
                continue;
            }
            if (lexer->lookahead == '\n') {
                advance_skip(lexer);
                continue;
            }
            return false;
        }
        if (lexer->lookahead != '/') {
            return false;
        }
        advance_skip(lexer);
        if (lexer->lookahead == '/') {
            while (!lexer->eof(lexer) && lexer->lookahead != '\r' && lexer->lookahead != '\n') {
                advance_skip(lexer);
            }
            continue;
        }
        if (lexer->lookahead != '*') {
            return false;
        }
        advance_skip(lexer);
        bool closed = false;
        for (bool star = false; !lexer->eof(lexer);) {
            if (star && lexer->lookahead == '/') {
                advance_skip(lexer);
                closed = true;
                break;
            }
            star = lexer->lookahead == '*';
            advance_skip(lexer);
        }
        if (!closed) {
            return false;
        }
    }
}

static bool has_valid_macro_identifier(
    TSLexer *lexer,
    const bool *valid_symbols,
    bool include_non_statement_categories
) {
    char name[MAX_MACRO_NAME_LENGTH];
    unsigned length = 0;
    if (!scan_identifier(lexer, name, &length)) {
        return false;
    }

    if (valid_symbols[MACRO_TOKEN_PASTE_IDENTIFIER_PREFIX] && has_following_token_paste(lexer)) {
        return true;
    }

    if (valid_symbols[TYPE_SPECIFIER_MACRO_IDENTIFIER] &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_TYPE_SPECIFIER, name, length)) {
        return true;
    }

    if (valid_symbols[PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER] &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_PREPROCESSOR_ARGUMENT, name, length)) {
        return true;
    }

    if (valid_symbols[BARE_MACRO_IDENTIFIER] &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_BARE_IDENTIFIER, name, length)) {
        return true;
    }

    if (valid_symbols[DECLARATION_PREFIX_MACRO_IDENTIFIER] &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_DECLARATION_PREFIX, name, length)) {
        return true;
    }

    if (valid_symbols[CALL_SYNTAX_MACRO_IDENTIFIER] &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_CALL_SYNTAX, name, length) &&
        has_following_argument_list(lexer)) {
        return true;
    }

    if (include_non_statement_categories &&
        ((valid_symbols[CALL_SYNTAX_MACRO_IDENTIFIER] &&
          strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_CALL_SYNTAX, name, length)) ||
         (valid_symbols[SEMICOLONLESS_CALL_MACRO_IDENTIFIER] &&
          strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_SEMICOLONLESS_CALL, name, length)))) {
        return true;
    }

    return valid_symbols[STATEMENT_ARGUMENT_MACRO_IDENTIFIER] &&
           strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_STATEMENT_ARGUMENT, name, length) &&
           has_following_argument_list(lexer);
}

static bool classify_macro_identifier_token(
    TSLexer *lexer,
    const char *name,
    unsigned length,
    bool allow_call,
    bool allow_statement_argument,
    bool allow_type_specifier,
    bool allow_declaration_prefix,
    bool allow_bare,
    bool allow_preprocessor_argument,
    bool allow_semicolonless_call
) {
    const bool call_match =
        allow_call &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_CALL_SYNTAX, name, length);
    const bool statement_argument_match =
        allow_statement_argument &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_STATEMENT_ARGUMENT, name, length);
    const bool type_specifier_match =
        allow_type_specifier &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_TYPE_SPECIFIER, name, length);
    const bool declaration_prefix_match =
        allow_declaration_prefix &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_DECLARATION_PREFIX, name, length);
    const bool bare_match =
        allow_bare && strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_BARE_IDENTIFIER, name, length);
    const bool preprocessor_argument_match =
        allow_preprocessor_argument &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_PREPROCESSOR_ARGUMENT, name, length);
    const bool semicolonless_call_match =
        allow_semicolonless_call &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_SEMICOLONLESS_CALL, name, length);

    if (declaration_prefix_match) {
        lexer->result_symbol = DECLARATION_PREFIX_MACRO_IDENTIFIER;
        return true;
    }

    if (preprocessor_argument_match) {
        lexer->result_symbol = PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER;
        return true;
    }

    if (semicolonless_call_match) {
        lexer->result_symbol = SEMICOLONLESS_CALL_MACRO_IDENTIFIER;
        return true;
    }

    if (call_match) {
        lexer->result_symbol = CALL_SYNTAX_MACRO_IDENTIFIER;
        return true;
    }

    if (statement_argument_match) {
        lexer->result_symbol = STATEMENT_ARGUMENT_MACRO_IDENTIFIER;
        return true;
    }

    if (type_specifier_match) {
        lexer->result_symbol = TYPE_SPECIFIER_MACRO_IDENTIFIER;
        return true;
    }

    if (bare_match) {
        lexer->result_symbol = BARE_MACRO_IDENTIFIER;
        return true;
    }

    return false;
}

static void skip_spaces_tabs(TSLexer *lexer) {
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
        advance(lexer);
    }
}

static bool scan_macro_identifier_token(
    TSLexer *lexer,
    bool allow_token_paste_prefix,
    bool allow_call,
    bool allow_statement_argument,
    bool allow_type_specifier,
    bool allow_declaration_prefix,
    bool allow_bare,
    bool allow_preprocessor_argument,
    bool allow_semicolonless_call
) {
    char name[MAX_MACRO_NAME_LENGTH];
    unsigned length = 0;
    if (!scan_identifier(lexer, name, &length)) {
        return false;
    }
    lexer->mark_end(lexer);

    if (allow_token_paste_prefix && has_following_token_paste(lexer)) {
        lexer->result_symbol = MACRO_TOKEN_PASTE_IDENTIFIER_PREFIX;
        return true;
    }

    return classify_macro_identifier_token(
        lexer,
        name,
        length,
        allow_call,
        allow_statement_argument,
        allow_type_specifier,
        allow_declaration_prefix,
        allow_bare,
        allow_preprocessor_argument,
        allow_semicolonless_call
    );
}

static bool scan_newline(TSLexer *lexer) {
    if (lexer->lookahead == '\r') {
        advance(lexer);
        if (lexer->lookahead == '\n') {
            advance(lexer);
        }
        return true;
    }
    if (lexer->lookahead == '\n') {
        advance(lexer);
        return true;
    }
    return false;
}

static bool scan_horizontal_whitespace(TSLexer *lexer) {
    bool consumed = false;
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\f') {
        advance(lexer);
        consumed = true;
    }
    return consumed;
}

static bool scan_raw_macro_replacement(TSLexer *lexer) {
    if (lexer->lookahead == '\r' || lexer->lookahead == '\n' || lexer->eof(lexer)) {
        return false;
    }
    if (lexer->lookahead != ' ' && lexer->lookahead != '\t' && lexer->lookahead != '\f' &&
        lexer->lookahead != '\\') {
        return false;
    }

    bool consumed = false;
    int32_t previous = 0;
    lexer->mark_end(lexer);
    while (!lexer->eof(lexer)) {
        if (lexer->lookahead == '\r' || lexer->lookahead == '\n') {
            if (previous != '\\') {
                return consumed;
            }
            scan_newline(lexer);
            lexer->mark_end(lexer);
            consumed = true;
            previous = 0;
            continue;
        }
        previous = lexer->lookahead;
        advance(lexer);
        lexer->mark_end(lexer);
        consumed = true;
    }
    return consumed;
}

static bool scan_raw_string_delimiter(Scanner *scanner, TSLexer *lexer) {
    if (scanner->delimiter_length > 0) {
        for (int i = 0; i < scanner->delimiter_length; ++i) {
            if (lexer->lookahead != scanner->delimiter[i]) {
                return false;
            }
            advance(lexer);
        }
        reset(scanner);
        return true;
    }

    for (;;) {
        if (scanner->delimiter_length >= MAX_DELIMITER_LENGTH || lexer->eof(lexer) || lexer->lookahead == '\\' ||
            iswspace(lexer->lookahead)) {
            return false;
        }
        if (lexer->lookahead == '(') {
            return scanner->delimiter_length > 0;
        }
        scanner->delimiter[scanner->delimiter_length++] = lexer->lookahead;
        advance(lexer);
    }
}

static bool scan_raw_string_content(Scanner *scanner, TSLexer *lexer) {
    for (int delimiter_index = -1;;) {
        if (lexer->eof(lexer)) {
            lexer->mark_end(lexer);
            return true;
        }

        if (delimiter_index >= 0) {
            if (delimiter_index == scanner->delimiter_length) {
                if (lexer->lookahead == '"') {
                    return true;
                }
                delimiter_index = -1;
            } else if (lexer->lookahead == scanner->delimiter[delimiter_index]) {
                delimiter_index += 1;
            } else {
                delimiter_index = -1;
            }
        }

        if (delimiter_index == -1 && lexer->lookahead == ')') {
            lexer->mark_end(lexer);
            delimiter_index = 0;
        }

        advance(lexer);
    }
}

void *tree_sitter_cpp_external_scanner_create() {
    Scanner *scanner = (Scanner *)ts_calloc(1, sizeof(Scanner));
    memset(scanner, 0, sizeof(Scanner));
    return scanner;
}

bool tree_sitter_cpp_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
    Scanner *scanner = (Scanner *)payload;

    const bool raw_string_ambiguous = valid_symbols[RAW_STRING_DELIMITER] && valid_symbols[RAW_STRING_CONTENT];

    if (!raw_string_ambiguous && valid_symbols[RAW_STRING_DELIMITER]) {
        lexer->result_symbol = RAW_STRING_DELIMITER;
        return scan_raw_string_delimiter(scanner, lexer);
    }

    if (!raw_string_ambiguous && valid_symbols[RAW_STRING_CONTENT]) {
        lexer->result_symbol = RAW_STRING_CONTENT;
        return scan_raw_string_content(scanner, lexer);
    }

    if (valid_symbols[RAW_MACRO_REPLACEMENT]) {
        if (scan_raw_macro_replacement(lexer)) {
            lexer->result_symbol = RAW_MACRO_REPLACEMENT;
            return true;
        }
    }

    if (valid_symbols[PREPROC_DIRECTIVE_END] &&
        (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\f' ||
         lexer->lookahead == '\r' || lexer->lookahead == '\n')) {
        const bool horizontal = scan_horizontal_whitespace(lexer);
        if (horizontal) {
            lexer->mark_end(lexer);
        }
        if (scan_newline(lexer)) {
            lexer->result_symbol = PREPROC_DIRECTIVE_END;
            return true;
        }
        if (horizontal && valid_symbols[LINE_BREAK_WHITESPACE] &&
            !valid_symbols[RAW_MACRO_DEFINITION_IDENTIFIER] &&
            (has_valid_macro_identifier(lexer, valid_symbols, false) ||
             (valid_symbols[MACRO_TOKEN_PASTE_NUMBER_PREFIX] && has_token_paste_number_prefix(lexer)))) {
            lexer->result_symbol = LINE_BREAK_WHITESPACE;
            return true;
        }
        return false;
    }

    if (valid_symbols[LINE_BREAK_WHITESPACE] &&
        (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\f' ||
         lexer->lookahead == '\r' || lexer->lookahead == '\n' || lexer->lookahead == '\\')) {
        // Only these categories distinguish leading from non-leading horizontal whitespace.
        const bool at_line_start =
            (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\f') &&
            !valid_symbols[RAW_MACRO_DEFINITION_IDENTIFIER] &&
            (valid_symbols[CALL_SYNTAX_MACRO_IDENTIFIER] || valid_symbols[SEMICOLONLESS_CALL_MACRO_IDENTIFIER]) &&
            lexer->get_column(lexer) == 0;
        const bool horizontal = scan_horizontal_whitespace(lexer);
        if (horizontal) {
            lexer->mark_end(lexer);
        }

        bool line_break = scan_newline(lexer);
        if (!line_break && lexer->lookahead == '\\') {
            advance(lexer);
            line_break = scan_newline(lexer);
        }
        if (line_break) {
            scan_horizontal_whitespace(lexer);
            lexer->mark_end(lexer);
            lexer->result_symbol = LINE_BREAK_WHITESPACE;
            return true;
        }

        if (horizontal && !valid_symbols[RAW_MACRO_DEFINITION_IDENTIFIER] &&
            (has_valid_macro_identifier(lexer, valid_symbols, at_line_start) ||
             (valid_symbols[MACRO_TOKEN_PASTE_NUMBER_PREFIX] && has_token_paste_number_prefix(lexer)))) {
            lexer->result_symbol = LINE_BREAK_WHITESPACE;
            return true;
        }

        return false;
    }

    if (valid_symbols[RAW_MACRO_DEFINITION_IDENTIFIER] || valid_symbols[MACRO_TOKEN_PASTE_IDENTIFIER_PREFIX] ||
        valid_symbols[MACRO_TOKEN_PASTE_NUMBER_PREFIX] ||
        valid_symbols[BARE_MACRO_IDENTIFIER] ||
        valid_symbols[DECLARATION_PREFIX_MACRO_IDENTIFIER] || valid_symbols[CALL_SYNTAX_MACRO_IDENTIFIER] ||
        valid_symbols[STATEMENT_ARGUMENT_MACRO_IDENTIFIER] || valid_symbols[TYPE_SPECIFIER_MACRO_IDENTIFIER] ||
        valid_symbols[PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER] ||
        valid_symbols[SEMICOLONLESS_CALL_MACRO_IDENTIFIER]) {
        skip_external_whitespace(lexer);
    }

    if (valid_symbols[RAW_MACRO_DEFINITION_IDENTIFIER] && is_identifier_start(lexer->lookahead)) {
        lexer->result_symbol = RAW_MACRO_DEFINITION_IDENTIFIER;
        return scan_macro_identifier(lexer, MACRO_CATEGORY_RAW_DEFINITION);
    }

    if (valid_symbols[MACRO_TOKEN_PASTE_NUMBER_PREFIX] &&
        (lexer->lookahead == '.' || (lexer->lookahead >= '0' && lexer->lookahead <= '9'))) {
        if (!scan_preprocessing_number(lexer)) {
            return false;
        }
        lexer->mark_end(lexer);
        if (has_following_token_paste(lexer)) {
            lexer->result_symbol = MACRO_TOKEN_PASTE_NUMBER_PREFIX;
            return true;
        }
        return false;
    }

    if ((valid_symbols[MACRO_TOKEN_PASTE_IDENTIFIER_PREFIX] || valid_symbols[CALL_SYNTAX_MACRO_IDENTIFIER] ||
         valid_symbols[DECLARATION_PREFIX_MACRO_IDENTIFIER] ||
         valid_symbols[STATEMENT_ARGUMENT_MACRO_IDENTIFIER] || valid_symbols[TYPE_SPECIFIER_MACRO_IDENTIFIER] ||
         valid_symbols[BARE_MACRO_IDENTIFIER] || valid_symbols[PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER] ||
         valid_symbols[SEMICOLONLESS_CALL_MACRO_IDENTIFIER]) &&
        is_identifier_start(lexer->lookahead)) {
        return scan_macro_identifier_token(
            lexer,
            valid_symbols[MACRO_TOKEN_PASTE_IDENTIFIER_PREFIX],
            valid_symbols[CALL_SYNTAX_MACRO_IDENTIFIER],
            valid_symbols[STATEMENT_ARGUMENT_MACRO_IDENTIFIER],
            valid_symbols[TYPE_SPECIFIER_MACRO_IDENTIFIER],
            valid_symbols[DECLARATION_PREFIX_MACRO_IDENTIFIER],
            valid_symbols[BARE_MACRO_IDENTIFIER],
            valid_symbols[PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER],
            valid_symbols[SEMICOLONLESS_CALL_MACRO_IDENTIFIER]
        );
    }

    return false;
}

unsigned tree_sitter_cpp_external_scanner_serialize(void *payload, char *buffer) {
    static_assert(MAX_DELIMITER_LENGTH * sizeof(wchar_t) < TREE_SITTER_SERIALIZATION_BUFFER_SIZE,
                  "Serialized delimiter is too long!");

    Scanner *scanner = (Scanner *)payload;
    size_t size = scanner->delimiter_length * sizeof(wchar_t);
    memcpy(buffer, scanner->delimiter, size);
    return (unsigned)size;
}

void tree_sitter_cpp_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
    assert(length % sizeof(wchar_t) == 0 && "Can't decode serialized delimiter!");

    Scanner *scanner = (Scanner *)payload;
    scanner->delimiter_length = length / sizeof(wchar_t);
    if (length > 0) {
        memcpy(&scanner->delimiter[0], buffer, length);
    }
}

void tree_sitter_cpp_external_scanner_destroy(void *payload) {
    Scanner *scanner = (Scanner *)payload;
    ts_free(scanner);
}
