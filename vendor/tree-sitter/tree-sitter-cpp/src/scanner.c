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
    BARE_MACRO_IDENTIFIER,
    CALL_SYNTAX_MACRO_IDENTIFIER,
    CONDITIONAL_MACRO_FUNCTION_HEADER,
    PREPROC_DIRECTIVE_END,
    LINE_BREAK_WHITESPACE,
};

enum MacroCategory {
    MACRO_CATEGORY_RAW_DEFINITION = 0,
    MACRO_CATEGORY_BARE_IDENTIFIER = 1,
    MACRO_CATEGORY_CALL_SYNTAX = 2,
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
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\r' || lexer->lookahead == '\n' ||
           lexer->lookahead == '\f') {
        advance_skip(lexer);
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

static bool scan_macro_identifier(TSLexer *lexer, enum MacroCategory category) {
    char name[MAX_MACRO_NAME_LENGTH];
    unsigned length = 0;
    if (!scan_identifier(lexer, name, &length)) {
        return false;
    }
    lexer->mark_end(lexer);
    return strictfmt_tree_sitter_cpp_macro_category_matches(category, name, length);
}

static void skip_spaces_tabs(TSLexer *lexer) {
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
        advance(lexer);
    }
}

static bool scan_char(TSLexer *lexer, int32_t expected) {
    if (lexer->lookahead != expected) {
        return false;
    }
    advance(lexer);
    return true;
}

static bool scan_literal(TSLexer *lexer, const char *literal) {
    for (const char *ch = literal; *ch != '\0'; ++ch) {
        if (lexer->lookahead != *ch) {
            return false;
        }
        advance(lexer);
    }
    return true;
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

static bool scan_to_line_end(TSLexer *lexer, bool allow_continuations) {
    int32_t previous = 0;
    lexer->mark_end(lexer);
    while (!lexer->eof(lexer)) {
        if (lexer->lookahead == '\r' || lexer->lookahead == '\n') {
            if (allow_continuations && previous == '\\') {
                scan_newline(lexer);
                lexer->mark_end(lexer);
                previous = 0;
                continue;
            }
            return true;
        }
        previous = lexer->lookahead;
        advance(lexer);
        lexer->mark_end(lexer);
    }
    return true;
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

static bool scan_comment_line(TSLexer *lexer) {
    skip_spaces_tabs(lexer);
    if (lexer->lookahead != '/') {
        return false;
    }
    advance(lexer);
    if (lexer->lookahead != '/') {
        return false;
    }
    return scan_to_line_end(lexer, false) && scan_newline(lexer);
}

static bool scan_optional_comment_lines(TSLexer *lexer) {
    for (;;) {
        if (!scan_comment_line(lexer)) {
            return true;
        }
    }
}

static bool scan_conditional_macro_branch(TSLexer *lexer) {
    scan_optional_comment_lines(lexer);
    skip_spaces_tabs(lexer);
    if (!scan_macro_identifier(lexer, MACRO_CATEGORY_CALL_SYNTAX)) {
        return false;
    }
    while (!lexer->eof(lexer)) {
        if (lexer->lookahead == '{') {
            advance(lexer);
            return scan_to_line_end(lexer, false) && scan_newline(lexer);
        }
        if (lexer->lookahead == '\r' || lexer->lookahead == '\n') {
            return false;
        }
        advance(lexer);
    }
    return false;
}

static bool scan_preproc_directive_line(TSLexer *lexer, const char *directive) {
    if (!scan_char(lexer, '#')) {
        return false;
    }
    skip_spaces_tabs(lexer);
    if (!scan_literal(lexer, directive)) {
        return false;
    }
    return scan_to_line_end(lexer, false) && scan_newline(lexer);
}

static bool scan_conditional_macro_function_header(TSLexer *lexer) {
    if (!scan_preproc_directive_line(lexer, "if")) {
        return false;
    }
    if (!scan_conditional_macro_branch(lexer)) {
        return false;
    }
    if (!scan_preproc_directive_line(lexer, "else")) {
        return false;
    }
    if (!scan_conditional_macro_branch(lexer)) {
        return false;
    }
    if (!scan_char(lexer, '#')) {
        return false;
    }
    skip_spaces_tabs(lexer);
    return scan_literal(lexer, "endif") && scan_to_line_end(lexer, false);
}

static bool scan_macro_identifier_token(TSLexer *lexer, bool allow_call, bool allow_bare) {
    char name[MAX_MACRO_NAME_LENGTH];
    unsigned length = 0;
    if (!scan_identifier(lexer, name, &length)) {
        return false;
    }
    lexer->mark_end(lexer);

    const bool call_match =
        allow_call && strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_CALL_SYNTAX, name, length);
    const bool bare_match =
        allow_bare && strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_BARE_IDENTIFIER, name, length);
    if (call_match) {
        lexer->result_symbol = CALL_SYNTAX_MACRO_IDENTIFIER;
        return true;
    }

    if (bare_match) {
        lexer->result_symbol = BARE_MACRO_IDENTIFIER;
        return true;
    }

    return false;
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

    if (valid_symbols[PREPROC_DIRECTIVE_END] && (lexer->lookahead == '\r' || lexer->lookahead == '\n')) {
        lexer->result_symbol = PREPROC_DIRECTIVE_END;
        return scan_newline(lexer);
    }

    if (valid_symbols[LINE_BREAK_WHITESPACE] && (lexer->lookahead == '\r' || lexer->lookahead == '\n')) {
        lexer->result_symbol = LINE_BREAK_WHITESPACE;
        return scan_newline(lexer);
    }

    if (valid_symbols[CONDITIONAL_MACRO_FUNCTION_HEADER]) {
        skip_external_whitespace(lexer);
    } else if (valid_symbols[RAW_MACRO_DEFINITION_IDENTIFIER] || valid_symbols[BARE_MACRO_IDENTIFIER] ||
               valid_symbols[CALL_SYNTAX_MACRO_IDENTIFIER]) {
        skip_external_whitespace(lexer);
    }

    if (valid_symbols[CONDITIONAL_MACRO_FUNCTION_HEADER] && lexer->lookahead == '#') {
        lexer->result_symbol = CONDITIONAL_MACRO_FUNCTION_HEADER;
        return scan_conditional_macro_function_header(lexer);
    }

    if (valid_symbols[RAW_MACRO_DEFINITION_IDENTIFIER] && is_identifier_start(lexer->lookahead)) {
        lexer->result_symbol = RAW_MACRO_DEFINITION_IDENTIFIER;
        return scan_macro_identifier(lexer, MACRO_CATEGORY_RAW_DEFINITION);
    }

    if ((valid_symbols[CALL_SYNTAX_MACRO_IDENTIFIER] || valid_symbols[BARE_MACRO_IDENTIFIER]) &&
        is_identifier_start(lexer->lookahead)) {
        return scan_macro_identifier_token(
            lexer,
            valid_symbols[CALL_SYNTAX_MACRO_IDENTIFIER],
            valid_symbols[BARE_MACRO_IDENTIFIER]
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
