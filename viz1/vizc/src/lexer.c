#include "lexer.h"
#include <string.h> // memcmp
#include <ctype.h> // isdigit, isalpha, isalnum, isxdigit

static const struct { const char *word; TokenKind kind; } keywords[] = {
    // keywords
    {"fn", TOK_DEFINE_FUNCTION},
    {"vr", TOK_DEFINE_VARIABLE},
    {"return", TOK_RETURN},
    {"im", TOK_IM},
    {"if", TOK_IF},
    {"else", TOK_ELSE},
    {"dec", TOK_DEC},
    {"until", TOK_UNTIL},
    {"break", TOK_BREAK},
    {"continue", TOK_CONTINUE},
    {"ct", TOK_CT},
    {"load", TOK_LOAD},
    {"asm", TOK_ASM},
    {"uf", TOK_UF},
    {"null", TOK_NULL},
    {"sz", TOK_SZ},

    // types
    {"i8", TOK_INT8},
    {"i16", TOK_INT16},
    {"i32", TOK_INT32},
    {"i64", TOK_INT64},
    {"u8", TOK_UINT8},
    {"u16", TOK_UINT16},
    {"u32", TOK_UINT32},
    {"u64", TOK_UINT64},
    {"uz", TOK_UZ},
    {"f32", TOK_FLOAT32},
    {"f64", TOK_FLOAT64},
    {"ch", TOK_CH},
};

const char *token_name(TokenKind k);
    
static char peek(Lexer *lx) {
    return lx -> source_string[lx -> current_position];
}

static int at_end(Lexer *lx) { return peek(lx) == '\0'; }

static char peek_next(Lexer *lx) {
    return peek(lx) ? lx -> source_string[lx -> current_position + 1]: '\0';
}

static char advance(Lexer *lx) {
    char c = peek(lx); lx -> current_position++;
    if (c == '\n') { lx -> line++; lx -> col = 1; }
    else { lx -> col++; }  
    return c;
}

static int match(Lexer *lx, char want) {
    if (peek(lx) == want) { advance(lx); return 1; }
    else { return 0; }
}

void lexer_init(Lexer *lx, const char *source) {
    lx -> source_string = source;
    lx -> current_position = 0;
    lx -> line = 1; lx -> col = 1;
    lx -> err_line = 0; lx -> err_col = 0;
}

static Token make_token(Lexer *lx, TokenKind kind, int start, int line, int col) {
    Token t; t.kind = kind;
    t.start = lx -> source_string + start;
    t.len = lx -> current_position - start;
    t.line = line; t.col = col; t.err = NULL;
    return t;
}

static Token error_token(Lexer *lx, const char *msg, int start, int line, int col) {
    Token t = make_token(lx, TOK_ERROR, start, line, col);
    t.err = msg; return t;
}

static int skip_whitespace(Lexer *lx) {
    for (;;) {
        char c = peek(lx);
        switch (c) {
            case ' ': case '\t': case '\r': case '\n':
                advance(lx);
                break;
            case '/':
                if (peek_next(lx) == '/') {
                    advance(lx); advance(lx);
                    while (peek(lx) != '\n' && !at_end(lx)) { advance(lx); } 
                    break;
                } else if (peek_next(lx) == '*') {
                    lx->err_line = lx->line; lx->err_col = lx->col;
                    advance(lx); advance(lx);
                    while (!(peek(lx) == '*' && peek_next(lx) == '/')) {
                        if (at_end(lx)) { return 0; }
                        advance(lx);
                    }
                    advance(lx); advance(lx); break;
                }
                return 1;
            default: return 1;
        }
    }
}

static TokenKind scan_identifier(Lexer *lx, int start) {
    char c = peek(lx);
    while (isalnum((unsigned char)c) || c == '_') { advance(lx); c = peek(lx); }
    const char *s = lx->source_string + start;
    size_t length = lx->current_position - start;
    size_t kwc = sizeof(keywords) / sizeof(keywords[0]);
    for (size_t i = 0; i < kwc; i++) {
        if (strlen(keywords[i].word) == length &&
           strncmp(s, keywords[i].word, length) == 0) {
            return keywords[i].kind;
           }
    }
    return TOK_IDENT;
}

static Token scan_number(Lexer *lx, int start, int line, int col) {
    char c = lx->source_string[start];
    if (c == '0' && peek(lx) == 'x') {
        advance(lx); int i = lx->current_position;
        while (isxdigit((unsigned char)peek(lx)) || peek(lx) == '_') {
            advance(lx);
        }
        if (lx->current_position == i) return error_token(lx, "expected digits after 0x", start, line, col);
        return make_token(lx, TOK_INT_LIT, start, line, col);
    }
    else if (c == '0' && peek(lx) == 'b') {
        advance(lx); int i = lx->current_position;
        while (peek(lx) == '0' || peek(lx) == '1' || peek(lx) == '_') {
            advance(lx);
        }
        if (lx->current_position == i) return error_token(lx, "expected digits after 0b", start, line, col);
        return make_token(lx, TOK_INT_LIT, start, line, col);
    }
    else {
        while (isdigit((unsigned char)peek(lx)) || peek(lx) == '_') {
            advance(lx);
        }
        if (peek(lx) == '.' && isdigit((unsigned char)peek_next(lx))) {
            advance(lx);
            while (isdigit((unsigned char)peek(lx)) || peek(lx) == '_') {
                advance(lx);
            }
            return make_token(lx, TOK_FLOAT_LIT, start, line, col);
        }
        return make_token(lx, TOK_INT_LIT, start, line, col);
    }
}
    
Token next_token(Lexer *lx) {
    int r = skip_whitespace(lx);
    if (r == 0) {
        return error_token(lx, "unclosed block comment", lx->current_position, lx->err_line, lx->err_col);
    }
    int start = lx -> current_position;
    int line = lx -> line; int col = lx -> col;
    if (at_end(lx)) { return make_token(lx, TOK_EOF, start, line, col); }
    char c = advance(lx);
    if (isalpha((unsigned char)c) || c == '_') {
        return make_token(lx, scan_identifier(lx, start), start, line, col);
    } else if (isdigit((unsigned char)c)) { return scan_number(lx, start, line, col); }
    switch (c) {
        case '(': return make_token(lx, TOK_LPAREN, start, line, col);
        case ')': return make_token(lx, TOK_RPAREN, start, line, col);
        case '[': return make_token(lx, TOK_LBRACK, start, line, col);
        case ']': return make_token(lx, TOK_RBRACK, start, line, col);
        case '{': return make_token(lx, TOK_LBRACE, start, line, col);
        case '}': return make_token(lx, TOK_RBRACE, start, line, col);
        case ';': return make_token(lx, TOK_SEMICOLON, start, line, col);
        case ',': return make_token(lx, TOK_COMMA, start, line, col);
        case ':': return make_token(lx, TOK_COLON, start, line, col);
        case '.': return make_token(lx, TOK_DOT, start, line, col);
        case '$': return make_token(lx, TOK_SIG_DOLLAR, start, line, col);
        case '~': return make_token(lx, TOK_BW_NOT, start, line, col);

        case '+': return make_token(lx, match(lx, '=') ? TOK_ADDASSIGN : TOK_PLUS, start, line, col);
        case '-': return make_token(lx, match(lx, '=') ? TOK_SUBASSIGN : TOK_MINUS, start, line, col);
        case '*': return make_token(lx, match(lx, '=') ? TOK_MULTASSIGN : TOK_ASTERISK, start, line, col);
        case '/': return make_token(lx, match(lx, '=') ? TOK_DIVASSIGN : TOK_FSLASH, start, line, col);
        case '%': return make_token(lx, match(lx, '=') ? TOK_MODULOASSIGN : TOK_MODULO, start, line, col);
        case '^': return make_token(lx, match(lx, '=') ? TOK_XORASSIGN : TOK_BW_XOR, start, line, col);
        case '=': return make_token(lx, match(lx, '=') ? TOK_COMP_EQ : TOK_ASSIGN, start, line, col);
        case '!': return make_token(lx, match(lx, '=') ? TOK_COMP_NOTEQ : TOK_LOG_NOT, start, line, col);
        
        case '&':
            if (match(lx, '&')) {return make_token(lx, TOK_LOG_AND, start, line, col);}
            else if (match(lx, '=')) {return make_token(lx, TOK_ANDASSIGN, start, line, col);}
            else {return make_token(lx, TOK_AMPERSAND, start, line, col);}
        case '|':
            if (match(lx, '|')) {return make_token(lx, TOK_LOG_OR, start, line, col);}
            else if (match(lx, '=')) {return make_token(lx, TOK_ORASSIGN, start, line, col);}
            else {return make_token(lx, TOK_BW_OR, start, line, col);}
        case '<':
            if (match(lx, '<'))
                return make_token(lx, match(lx, '=') ? TOK_LSHIFTASSIGN : TOK_BW_LSHIFT, start, line, col);
            return make_token(lx, match(lx, '=') ? TOK_COMP_LTHAN_EQ : TOK_COMP_LTHAN, start, line, col);
        case '>':
            if (match(lx, '>'))
                return make_token(lx, match(lx, '=') ? TOK_RSHIFTASSIGN : TOK_BW_RSHIFT, start, line, col);
            return make_token(lx, match(lx, '=') ? TOK_COMP_GTHAN_EQ : TOK_COMP_GTHAN, start, line, col);
        default: return error_token(lx, "unexpected token", start, line, col);
    }
}

const char *token_name(TokenKind k) {
    static const char *names[TOK_COUNT] = {
        // keywords
        [TOK_DEFINE_FUNCTION] = "DEFINE_FUNCTION",
        [TOK_DEFINE_VARIABLE] = "DEFINE_VARIABLE",
        [TOK_RETURN] = "RETURN",
        [TOK_IM] = "IM",
        [TOK_IF] = "IF",
        [TOK_ELSE] = "ELSE",
        [TOK_DEC] = "DEC",
        [TOK_UNTIL] = "UNTIL",
        [TOK_BREAK] = "BREAK",
        [TOK_CONTINUE] = "CONTINUE",
        [TOK_CT] = "CT",
        [TOK_LOAD] = "LOAD",
        [TOK_ASM] = "ASM",
        [TOK_UF] = "UF",
        [TOK_NULL] = "NULL",
        [TOK_SZ] = "SZ",

        // types
        [TOK_INT8] = "INT8",
        [TOK_INT16] = "INT16",
        [TOK_INT32] = "INT32",
        [TOK_INT64] = "INT64",
        [TOK_UINT8] = "UINT8",
        [TOK_UINT16] = "UINT16",
        [TOK_UINT32] = "UINT32",
        [TOK_UINT64] = "UINT64",
        [TOK_UZ] = "UZ",
        [TOK_FLOAT32] = "FLOAT32",
        [TOK_FLOAT64] = "FLOAT64",
        [TOK_CH] = "CH",

        // sigils
        [TOK_SIG_DOLLAR] = "SIG_DOLLAR",

        // punctuation
        [TOK_LPAREN] = "LPAREN",
        [TOK_RPAREN] = "RPAREN",
        [TOK_LBRACE] = "LBRACE",
        [TOK_RBRACE] = "RBRACE",
        [TOK_EOF] = "EOF",
        [TOK_SEMICOLON] = "SEMICOLON",
        [TOK_COLON] = "COLON",
        [TOK_COMMA] = "COMMA",
        [TOK_LBRACK] = "LBRACK",
        [TOK_RBRACK] = "RBRACK",

        // literals
        [TOK_FLOAT_LIT] = "FLOAT_LIT",
        [TOK_CHAR_LIT] = "CHAR_LIT",
        [TOK_STRING_LIT] = "STRING_LIT",
        [TOK_INT_LIT] = "INT_LIT",
        [TOK_ERROR] = "ERROR",

        // operators
        [TOK_PLUS] = "PLUS",
        [TOK_MINUS] = "MINUS",
        [TOK_ASTERISK] = "ASTERISK",
        [TOK_FSLASH] = "FSLASH",
        [TOK_MODULO] = "MODULO",
        [TOK_ADDASSIGN] = "ADDASSIGN",
        [TOK_SUBASSIGN] = "SUBASSIGN",
        [TOK_MULTASSIGN] = "MULTASSIGN",
        [TOK_DIVASSIGN] = "DIVASSIGN",
        [TOK_MODULOASSIGN] = "MODULOASSIGN",
        [TOK_ANDASSIGN] = "ANDASSIGN",
        [TOK_ORASSIGN] = "ORASSIGN",
        [TOK_XORASSIGN] = "XORASSIGN",
        [TOK_LSHIFTASSIGN] = "LSHIFTASSIGN",
        [TOK_RSHIFTASSIGN] = "RSHIFTASSIGN",
        [TOK_AMPERSAND] = "AMPERSAND",
        [TOK_DOT] = "DOT",
        [TOK_ASSIGN] = "ASSIGN",
        [TOK_COMP_EQ] = "COMP_EQ",
        [TOK_COMP_NOTEQ] = "COMP_NOTEQ",
        [TOK_COMP_LTHAN] = "COMP_LTHAN",
        [TOK_COMP_GTHAN] = "COMP_GTHAN",
        [TOK_COMP_LTHAN_EQ] = "COMP_LTHAN_EQ",
        [TOK_COMP_GTHAN_EQ] = "COMP_GTHAN_EQ",
        [TOK_LOG_AND] = "LOG_AND",
        [TOK_LOG_OR] = "LOG_OR",
        [TOK_LOG_NOT] = "LOG_NOT",
        [TOK_BW_OR] = "BW_OR",
        [TOK_BW_XOR] = "BW_XOR",
        [TOK_BW_NOT] = "BW_NOT",
        [TOK_BW_LSHIFT] = "BW_LSHIFT",
        [TOK_BW_RSHIFT] = "BW_RSHIFT",

        [TOK_IDENT] = "IDENT",
    };

    if (k >= 0 && k < TOK_COUNT && names[k]) {
        return names[k];
    }
    return "???";
}