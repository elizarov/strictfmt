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
    OBJECT_MACRO_REPLACEMENT_START,
    FUNCTION_MACRO_REPLACEMENT_START,
    RAW_MACRO_TOKEN,
    MACRO_TOKEN_PASTE_IDENTIFIER_PREFIX,
    MACRO_TOKEN_PASTE_NUMBER_PREFIX,
    BARE_MACRO_IDENTIFIER,
    DECLARATION_PREFIX_MACRO_IDENTIFIER,
    METHOD_DECLARATION_MACRO_IDENTIFIER,
    STATEMENT_ARGUMENT_MACRO_IDENTIFIER,
    TYPE_SPECIFIER_MACRO_IDENTIFIER,
    PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER,
    SEMICOLONLESS_CALL_MACRO_IDENTIFIER,
    STATEMENT_PREFIX_MACRO_IDENTIFIER,
    SEMICOLONLESS_PREPROCESSOR_CALL_MACRO_IDENTIFIER,
    PREPROC_DIRECTIVE_END,
    LINE_BREAK_WHITESPACE,
    MACRO_DEFINITION_START,
    NONCONDITIONAL_DIRECTIVE_START,
    TEMPLATE_ARGUMENT_CLOSE,
    SPLIT_RIGHT_ANGLE,
};

enum MacroCategory {
    MACRO_CATEGORY_BARE_IDENTIFIER = 0,
    MACRO_CATEGORY_METHOD_DECLARATION = 1,
    MACRO_CATEGORY_STATEMENT_ARGUMENT = 2,
    MACRO_CATEGORY_DECLARATION_PREFIX = 3,
    MACRO_CATEGORY_TYPE_SPECIFIER = 4,
    MACRO_CATEGORY_PREPROCESSOR_ARGUMENT = 5,
    MACRO_CATEGORY_SEMICOLONLESS_CALL = 6,
    MACRO_CATEGORY_STATEMENT_PREFIX = 7,
};

/// The spec limits raw-string delimiters to 16 chars.
#define MAX_DELIMITER_LENGTH 16
#define MAX_MACRO_NAME_LENGTH 256

typedef struct {
    bool in_directive;
    bool in_macro_header;
    bool split_right_angle;
    uint8_t delimiter_length;
    wchar_t delimiter[MAX_DELIMITER_LENGTH];
} Scanner;

#ifdef STRICTFMT_RUNTIME_MACRO_CATEGORIES
extern bool strictfmt_tree_sitter_cpp_macro_category_matches(
    unsigned category,
    const char *text,
    unsigned length
);
#else
static bool strictfmt_tree_sitter_cpp_macro_category_matches(unsigned category, const char *text, unsigned length) {
    return false;
}
#endif

static inline void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

static inline void advance_skip(TSLexer *lexer) { lexer->advance(lexer, true); }

static inline void reset(Scanner *scanner) {
    scanner->delimiter_length = 0;
    memset(scanner->delimiter, 0, sizeof scanner->delimiter);
}

static bool has_angle_token(TSLexer *lexer, const bool *valid_symbols) {
    return valid_symbols[TEMPLATE_ARGUMENT_CLOSE] && lexer->lookahead == '>';
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
            advance(lexer);
        }
        if (lexer->lookahead == '(') {
            return true;
        }
        if (lexer->lookahead == '\\') {
            advance(lexer);
            if (lexer->lookahead == '\r') {
                advance(lexer);
                if (lexer->lookahead == '\n') {
                    advance(lexer);
                }
                continue;
            }
            if (lexer->lookahead == '\n') {
                advance(lexer);
                continue;
            }
            return false;
        }
        if (lexer->lookahead != '/') {
            return false;
        }
        advance(lexer);
        if (lexer->lookahead == '/') {
            while (!lexer->eof(lexer) && lexer->lookahead != '\r' && lexer->lookahead != '\n') {
                advance(lexer);
            }
            continue;
        }
        if (lexer->lookahead != '*') {
            return false;
        }
        advance(lexer);
        bool closed = false;
        for (bool star = false; !lexer->eof(lexer);) {
            if (star && lexer->lookahead == '/') {
                advance(lexer);
                closed = true;
                break;
            }
            star = lexer->lookahead == '*';
            advance(lexer);
        }
        if (!closed) {
            return false;
        }
    }
}

static bool classify_macro_identifier_token(
    TSLexer *lexer,
    const char *name,
    unsigned length,
    bool allow_method_declaration,
    bool allow_statement_argument,
    bool allow_type_specifier,
    bool allow_declaration_prefix,
    bool allow_bare,
    bool allow_preprocessor_argument,
    bool allow_semicolonless_call,
    bool allow_statement_prefix,
    bool allow_semicolonless_preprocessor_call
) {
    const bool method_declaration_match =
        allow_method_declaration &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_METHOD_DECLARATION, name, length);
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
        (allow_preprocessor_argument || allow_semicolonless_preprocessor_call) &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_PREPROCESSOR_ARGUMENT, name, length);
    const bool semicolonless_call_match =
        (allow_semicolonless_call || allow_semicolonless_preprocessor_call) &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_SEMICOLONLESS_CALL, name, length);

    if (allow_statement_prefix &&
        strictfmt_tree_sitter_cpp_macro_category_matches(MACRO_CATEGORY_STATEMENT_PREFIX, name, length)) {
        lexer->result_symbol = STATEMENT_PREFIX_MACRO_IDENTIFIER;
        return true;
    }

    if (declaration_prefix_match) {
        lexer->result_symbol = DECLARATION_PREFIX_MACRO_IDENTIFIER;
        return true;
    }

    const bool has_arguments =
        (preprocessor_argument_match || semicolonless_call_match || method_declaration_match ||
         statement_argument_match || type_specifier_match) && has_following_argument_list(lexer);

    if (allow_semicolonless_preprocessor_call && preprocessor_argument_match &&
        semicolonless_call_match && has_arguments) {
        lexer->result_symbol = SEMICOLONLESS_PREPROCESSOR_CALL_MACRO_IDENTIFIER;
        return true;
    }

    if (allow_preprocessor_argument && preprocessor_argument_match && has_arguments) {
        lexer->result_symbol = PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER;
        return true;
    }

    if (allow_semicolonless_call && semicolonless_call_match && has_arguments) {
        lexer->result_symbol = SEMICOLONLESS_CALL_MACRO_IDENTIFIER;
        return true;
    }

    if (method_declaration_match && has_arguments) {
        lexer->result_symbol = METHOD_DECLARATION_MACRO_IDENTIFIER;
        return true;
    }

    if (statement_argument_match && has_arguments) {
        lexer->result_symbol = STATEMENT_ARGUMENT_MACRO_IDENTIFIER;
        return true;
    }

    if (type_specifier_match && has_arguments) {
        lexer->result_symbol = TYPE_SPECIFIER_MACRO_IDENTIFIER;
        return true;
    }

    if (bare_match) {
        lexer->result_symbol = BARE_MACRO_IDENTIFIER;
        return true;
    }

    return false;
}

static bool has_valid_macro_identifier(TSLexer *lexer, const bool *valid_symbols) {
    char name[MAX_MACRO_NAME_LENGTH];
    unsigned length = 0;
    if (!scan_identifier(lexer, name, &length)) {
        return false;
    }

    if (valid_symbols[MACRO_TOKEN_PASTE_IDENTIFIER_PREFIX] && has_following_token_paste(lexer)) {
        return true;
    }

    return classify_macro_identifier_token(
        lexer,
        name,
        length,
        valid_symbols[METHOD_DECLARATION_MACRO_IDENTIFIER],
        valid_symbols[STATEMENT_ARGUMENT_MACRO_IDENTIFIER],
        valid_symbols[TYPE_SPECIFIER_MACRO_IDENTIFIER],
        valid_symbols[DECLARATION_PREFIX_MACRO_IDENTIFIER],
        valid_symbols[BARE_MACRO_IDENTIFIER],
        valid_symbols[PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER],
        valid_symbols[SEMICOLONLESS_CALL_MACRO_IDENTIFIER],
        valid_symbols[STATEMENT_PREFIX_MACRO_IDENTIFIER],
        valid_symbols[SEMICOLONLESS_PREPROCESSOR_CALL_MACRO_IDENTIFIER]
    );
}

static void skip_spaces_tabs(TSLexer *lexer) {
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
        advance(lexer);
    }
}

static bool scan_macro_identifier_token(
    TSLexer *lexer,
    bool allow_token_paste_prefix,
    bool allow_method_declaration,
    bool allow_statement_argument,
    bool allow_type_specifier,
    bool allow_declaration_prefix,
    bool allow_bare,
    bool allow_preprocessor_argument,
    bool allow_semicolonless_call,
    bool allow_statement_prefix,
    bool allow_semicolonless_preprocessor_call
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
        allow_method_declaration,
        allow_statement_argument,
        allow_type_specifier,
        allow_declaration_prefix,
        allow_bare,
        allow_preprocessor_argument,
        allow_semicolonless_call,
        allow_statement_prefix,
        allow_semicolonless_preprocessor_call
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

static bool scan_quoted_macro_token(TSLexer *lexer, int32_t quote) {
    advance(lexer);
    while (!lexer->eof(lexer) && lexer->lookahead != '\r' && lexer->lookahead != '\n') {
        if (lexer->lookahead == quote) {
            advance(lexer);
            return true;
        }
        if (lexer->lookahead == '\\') {
            advance(lexer);
            if (scan_newline(lexer)) {
                continue;
            }
            if (lexer->eof(lexer)) {
                return false;
            }
        }
        advance(lexer);
    }
    return false;
}

static bool scan_raw_macro_string(TSLexer *lexer) {
    int32_t delimiter[MAX_DELIMITER_LENGTH];
    unsigned length = 0;
    advance(lexer);
    while (lexer->lookahead != '(') {
        if (length == MAX_DELIMITER_LENGTH || lexer->eof(lexer) ||
            iswspace(lexer->lookahead) || lexer->lookahead == '\\' || lexer->lookahead == ')') {
            return false;
        }
        delimiter[length++] = lexer->lookahead;
        advance(lexer);
    }
    advance(lexer);
    while (!lexer->eof(lexer)) {
        if (lexer->lookahead != ')') {
            advance(lexer);
            continue;
        }
        advance(lexer);
        unsigned matched = 0;
        while (matched < length && lexer->lookahead == delimiter[matched]) {
            ++matched;
            advance(lexer);
        }
        if (matched == length && lexer->lookahead == '"') {
            advance(lexer);
            return true;
        }
    }
    return false;
}

static bool scan_raw_macro_token(TSLexer *lexer) {
    if (lexer->lookahead == '\r' || lexer->lookahead == '\n' || lexer->eof(lexer)) {
        return false;
    }
    bool consumed = false;
    lexer->mark_end(lexer);
    while (!lexer->eof(lexer) && lexer->lookahead != '\r' && lexer->lookahead != '\n') {
        const int32_t ch = lexer->lookahead;
        if (ch == '\\') {
            advance(lexer);
            scan_newline(lexer);
        } else if (ch == '"' || ch == '\'') {
            if (!scan_quoted_macro_token(lexer, ch)) {
                return false;
            }
        } else if (ch >= '0' && ch <= '9') {
            scan_preprocessing_number(lexer);
        } else if (is_identifier_start(ch) || ch >= 0x80) {
            char prefix[4] = {0};
            unsigned length = 0;
            do {
                if (length < sizeof(prefix) - 1) {
                    prefix[length] = (char)lexer->lookahead;
                }
                ++length;
                advance(lexer);
            } while (is_identifier_continue(lexer->lookahead) || lexer->lookahead >= 0x80);
            if (lexer->lookahead == '"' && length < sizeof(prefix) &&
                (strcmp(prefix, "R") == 0 || strcmp(prefix, "u8R") == 0 ||
                 strcmp(prefix, "uR") == 0 || strcmp(prefix, "UR") == 0 || strcmp(prefix, "LR") == 0)) {
                if (!scan_raw_macro_string(lexer)) {
                    return false;
                }
            }
        } else if (ch == '/') {
            advance(lexer);
            if (lexer->lookahead == '*') {
                advance(lexer);
                bool closed = false;
                while (!lexer->eof(lexer)) {
                    if (lexer->lookahead == '*') {
                        advance(lexer);
                        if (lexer->lookahead == '/') {
                            advance(lexer);
                            closed = true;
                            break;
                        }
                    } else {
                        advance(lexer);
                    }
                }
                if (!closed) {
                    return false;
                }
            } else if (lexer->lookahead == '/') {
                while (!lexer->eof(lexer) && lexer->lookahead != '\r' && lexer->lookahead != '\n') {
                    if (lexer->lookahead == '\\') {
                        advance(lexer);
                        scan_newline(lexer);
                    } else {
                        advance(lexer);
                    }
                }
            }
        } else {
            advance(lexer);
        }
        lexer->mark_end(lexer);
        consumed = true;
        if (ch != ' ' && ch != '\t' && ch != '\f' && ch != '\v' && ch != '\\') {
            return true;
        }
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

static bool scan_preprocessor_start(Scanner *scanner, TSLexer *lexer, const bool *valid_symbols) {
    advance(lexer);
    scan_horizontal_whitespace(lexer);
    char keyword[16];
    unsigned length = 0;
    while (is_identifier_continue(lexer->lookahead)) {
        if (length + 1 >= sizeof(keyword)) {
            return false;
        }
        keyword[length++] = (char)lexer->lookahead;
        advance(lexer);
    }
    keyword[length] = '\0';
    if (valid_symbols[MACRO_DEFINITION_START] && strcmp(keyword, "define") == 0 &&
        (lexer->lookahead == ' ' || lexer->lookahead == '\t')) {
        scan_horizontal_whitespace(lexer);
        lexer->mark_end(lexer);
        scanner->in_directive = true;
        scanner->in_macro_header = true;
        lexer->result_symbol = MACRO_DEFINITION_START;
        return true;
    }
    if (valid_symbols[NONCONDITIONAL_DIRECTIVE_START] &&
        (strcmp(keyword, "undef") == 0 || strcmp(keyword, "pragma") == 0 ||
         strcmp(keyword, "line") == 0 || strcmp(keyword, "error") == 0 ||
         strcmp(keyword, "warning") == 0 || strcmp(keyword, "using") == 0 ||
         (length == 0 && (lexer->lookahead == '\n' || lexer->lookahead == '\r' || lexer->eof(lexer))))) {
        lexer->mark_end(lexer);
        scanner->in_directive = true;
        lexer->result_symbol = NONCONDITIONAL_DIRECTIVE_START;
        return true;
    }
    return false;
}

static bool has_runtime_token_boundary(Scanner *scanner, TSLexer *lexer, const bool *valid_symbols) {
    return (!scanner->in_directive &&
            (valid_symbols[MACRO_DEFINITION_START] || valid_symbols[NONCONDITIONAL_DIRECTIVE_START]) &&
            lexer->lookahead == '#') ||
           has_angle_token(lexer, valid_symbols) ||
           has_valid_macro_identifier(lexer, valid_symbols) ||
           (valid_symbols[MACRO_TOKEN_PASTE_NUMBER_PREFIX] && has_token_paste_number_prefix(lexer));
}

bool tree_sitter_cpp_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
    Scanner *scanner = (Scanner *)payload;

    if (lexer->lookahead != '>') {
        scanner->split_right_angle = false;
    }

    if (scanner->split_right_angle && lexer->lookahead == '>' &&
        (valid_symbols[TEMPLATE_ARGUMENT_CLOSE] || valid_symbols[SPLIT_RIGHT_ANGLE])) {
        advance(lexer);
        lexer->mark_end(lexer);
        scanner->split_right_angle = false;
        lexer->result_symbol = valid_symbols[TEMPLATE_ARGUMENT_CLOSE] ? TEMPLATE_ARGUMENT_CLOSE : SPLIT_RIGHT_ANGLE;
        return true;
    }

    const bool raw_string_ambiguous = valid_symbols[RAW_STRING_DELIMITER] && valid_symbols[RAW_STRING_CONTENT];

    if (!raw_string_ambiguous && valid_symbols[RAW_STRING_DELIMITER]) {
        lexer->result_symbol = RAW_STRING_DELIMITER;
        return scan_raw_string_delimiter(scanner, lexer);
    }

    if (!raw_string_ambiguous && valid_symbols[RAW_STRING_CONTENT]) {
        lexer->result_symbol = RAW_STRING_CONTENT;
        return scan_raw_string_content(scanner, lexer);
    }

    if ((valid_symbols[MACRO_DEFINITION_START] || valid_symbols[NONCONDITIONAL_DIRECTIVE_START]) &&
        !scanner->in_directive && lexer->lookahead == '#') {
        return scan_preprocessor_start(scanner, lexer, valid_symbols);
    }

    // Object-like definitions cannot have an immediately adjacent parameter list.
    // The zero-width boundary forks header reductions before raw/structured lexing.
    if (scanner->in_macro_header && !raw_string_ambiguous &&
        (valid_symbols[FUNCTION_MACRO_REPLACEMENT_START] ||
         (valid_symbols[OBJECT_MACRO_REPLACEMENT_START] && lexer->lookahead != '('))) {
        lexer->mark_end(lexer);
        scanner->in_macro_header = false;
        lexer->result_symbol = valid_symbols[FUNCTION_MACRO_REPLACEMENT_START]
                                   ? FUNCTION_MACRO_REPLACEMENT_START
                                   : OBJECT_MACRO_REPLACEMENT_START;
        return true;
    }

    if (scanner->in_directive && !scanner->in_macro_header && !raw_string_ambiguous &&
        valid_symbols[RAW_MACRO_TOKEN] && lexer->lookahead != '\r' && lexer->lookahead != '\n' &&
        !lexer->eof(lexer)) {
        lexer->result_symbol = RAW_MACRO_TOKEN;
        return scan_raw_macro_token(lexer);
    }

    if (valid_symbols[PREPROC_DIRECTIVE_END] &&
        (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\f' ||
         lexer->lookahead == '\r' || lexer->lookahead == '\n' || lexer->eof(lexer))) {
        const bool horizontal = scan_horizontal_whitespace(lexer);
        if (horizontal) {
            lexer->mark_end(lexer);
        }
        if (scan_newline(lexer) || lexer->eof(lexer)) {
            lexer->mark_end(lexer);
            scanner->in_directive = false;
            scanner->in_macro_header = false;
            lexer->result_symbol = PREPROC_DIRECTIVE_END;
            return true;
        }
        // Built-in extras can skip trailing splices without calling us again at the directive end.
        // Continuing content needs a whitespace boundary only before runtime-classified tokens.
        if (lexer->lookahead == '\\') {
            do {
                advance(lexer);
                if (!scan_newline(lexer)) {
                    return false;
                }
                scan_horizontal_whitespace(lexer);
            } while (lexer->lookahead == '\\');
            if (!lexer->eof(lexer) && !scan_newline(lexer)) {
                lexer->mark_end(lexer);
                if (valid_symbols[LINE_BREAK_WHITESPACE] &&
                    has_runtime_token_boundary(scanner, lexer, valid_symbols)) {
                    lexer->result_symbol = LINE_BREAK_WHITESPACE;
                    return true;
                }
                return false;
            }
            lexer->mark_end(lexer);
            scanner->in_directive = false;
            scanner->in_macro_header = false;
            lexer->result_symbol = PREPROC_DIRECTIVE_END;
            return true;
        }
        if (horizontal && valid_symbols[LINE_BREAK_WHITESPACE] &&
            has_runtime_token_boundary(scanner, lexer, valid_symbols)) {
            lexer->result_symbol = LINE_BREAK_WHITESPACE;
            return true;
        }
        return false;
    }

    if (valid_symbols[LINE_BREAK_WHITESPACE] &&
        (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\f' ||
         lexer->lookahead == '\r' || lexer->lookahead == '\n' || lexer->lookahead == '\\')) {
        const bool horizontal = scan_horizontal_whitespace(lexer);
        if (horizontal) {
            lexer->mark_end(lexer);
        }

        bool line_break = scan_newline(lexer);
        if (line_break && scanner->in_directive) {
            return false;
        }
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

        if (horizontal &&
            has_runtime_token_boundary(scanner, lexer, valid_symbols)) {
            lexer->result_symbol = LINE_BREAK_WHITESPACE;
            return true;
        }

        return false;
    }

    if (valid_symbols[TEMPLATE_ARGUMENT_CLOSE] && lexer->lookahead == '>') {
        advance(lexer);
        lexer->mark_end(lexer);
        if (lexer->lookahead == '=') {
            return false;
        }
        const bool split_pair = lexer->lookahead == '>';
        if (split_pair) {
            advance(lexer);
            if (lexer->lookahead == '=') {
                return false;
            }
        }
        scanner->split_right_angle = split_pair;
        lexer->result_symbol = TEMPLATE_ARGUMENT_CLOSE;
        return true;
    }

    if (valid_symbols[MACRO_TOKEN_PASTE_IDENTIFIER_PREFIX] ||
        valid_symbols[MACRO_TOKEN_PASTE_NUMBER_PREFIX] ||
        valid_symbols[BARE_MACRO_IDENTIFIER] ||
        valid_symbols[DECLARATION_PREFIX_MACRO_IDENTIFIER] || valid_symbols[METHOD_DECLARATION_MACRO_IDENTIFIER] ||
        valid_symbols[STATEMENT_ARGUMENT_MACRO_IDENTIFIER] || valid_symbols[TYPE_SPECIFIER_MACRO_IDENTIFIER] ||
        valid_symbols[PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER] ||
        valid_symbols[SEMICOLONLESS_CALL_MACRO_IDENTIFIER] || valid_symbols[STATEMENT_PREFIX_MACRO_IDENTIFIER] ||
        valid_symbols[SEMICOLONLESS_PREPROCESSOR_CALL_MACRO_IDENTIFIER]) {
        skip_external_whitespace(lexer);
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

    if ((valid_symbols[MACRO_TOKEN_PASTE_IDENTIFIER_PREFIX] || valid_symbols[METHOD_DECLARATION_MACRO_IDENTIFIER] ||
         valid_symbols[DECLARATION_PREFIX_MACRO_IDENTIFIER] ||
         valid_symbols[STATEMENT_ARGUMENT_MACRO_IDENTIFIER] || valid_symbols[TYPE_SPECIFIER_MACRO_IDENTIFIER] ||
         valid_symbols[BARE_MACRO_IDENTIFIER] || valid_symbols[PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER] ||
         valid_symbols[SEMICOLONLESS_CALL_MACRO_IDENTIFIER] || valid_symbols[STATEMENT_PREFIX_MACRO_IDENTIFIER] ||
         valid_symbols[SEMICOLONLESS_PREPROCESSOR_CALL_MACRO_IDENTIFIER]) &&
        is_identifier_start(lexer->lookahead)) {
        return scan_macro_identifier_token(
            lexer,
            valid_symbols[MACRO_TOKEN_PASTE_IDENTIFIER_PREFIX],
            valid_symbols[METHOD_DECLARATION_MACRO_IDENTIFIER],
            valid_symbols[STATEMENT_ARGUMENT_MACRO_IDENTIFIER],
            valid_symbols[TYPE_SPECIFIER_MACRO_IDENTIFIER],
            valid_symbols[DECLARATION_PREFIX_MACRO_IDENTIFIER],
            valid_symbols[BARE_MACRO_IDENTIFIER],
            valid_symbols[PREPROCESSOR_ARGUMENT_MACRO_IDENTIFIER],
            valid_symbols[SEMICOLONLESS_CALL_MACRO_IDENTIFIER],
            valid_symbols[STATEMENT_PREFIX_MACRO_IDENTIFIER],
            valid_symbols[SEMICOLONLESS_PREPROCESSOR_CALL_MACRO_IDENTIFIER]
        );
    }

    return false;
}

unsigned tree_sitter_cpp_external_scanner_serialize(void *payload, char *buffer) {
    Scanner *scanner = (Scanner *)payload;
    unsigned delimiter_bytes = scanner->delimiter_length * sizeof(wchar_t);
    buffer[0] = scanner->in_directive | (scanner->split_right_angle << 1) | (scanner->in_macro_header << 2);
    memcpy(buffer + 1, scanner->delimiter, delimiter_bytes);
    return 1 + delimiter_bytes;
}

void tree_sitter_cpp_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
    Scanner *scanner = (Scanner *)payload;
    reset(scanner);
    scanner->in_directive = length > 0 && (buffer[0] & 1);
    scanner->split_right_angle = length > 0 && (buffer[0] & 2);
    scanner->in_macro_header = length > 0 && (buffer[0] & 4);
    if (length > 0) {
        --length;
        assert(length % sizeof(wchar_t) == 0 && length <= sizeof(scanner->delimiter) &&
               "Can't decode serialized delimiter!");
        scanner->delimiter_length = length / sizeof(wchar_t);
        memcpy(scanner->delimiter, buffer + 1, length);
    }
}

void tree_sitter_cpp_external_scanner_destroy(void *payload) {
    Scanner *scanner = (Scanner *)payload;
    ts_free(scanner);
}
