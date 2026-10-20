#ifndef LEXER_H
#define LEXER_H

typedef enum {
    // keywords
    TOK_DEFINE_FUNCTION,
    TOK_DEFINE_VARIABLE,
    TOK_RETURN, TOK_IM,
    TOK_IF, TOK_ELSE,
    TOK_DEC, TOK_UNTIL,
    TOK_BREAK, TOK_CONTINUE,
    TOK_CT, TOK_LOAD, TOK_ASM,
    TOK_UF, TOK_NULL, TOK_SZ,
    // types
    TOK_INT8, TOK_INT16, TOK_INT32, TOK_INT64,
    TOK_UINT8, TOK_UINT16, TOK_UINT32, TOK_UINT64,
    TOK_UZ, TOK_FLOAT32, TOK_FLOAT64, TOK_CH,
    // sigils
    TOK_SIG_DOLLAR,
    // punctuation
    TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACE, TOK_RBRACE,
    TOK_EOF, TOK_SEMICOLON,
    TOK_COLON, TOK_COMMA,
    TOK_LBRACK, TOK_RBRACK,
    // literals
    TOK_FLOAT_LIT, TOK_CHAR_LIT,
    TOK_STRING_LIT, TOK_INT_LIT,
    TOK_ERROR,
    // operators
    TOK_PLUS, TOK_MINUS, TOK_ASTERISK, TOK_FSLASH, TOK_MODULO,
    TOK_ADDASSIGN, TOK_SUBASSIGN, TOK_MULTASSIGN, TOK_DIVASSIGN,
    TOK_MODULOASSIGN, TOK_ANDASSIGN, TOK_ORASSIGN, TOK_XORASSIGN,
    TOK_LSHIFTASSIGN, TOK_RSHIFTASSIGN, TOK_AMPERSAND, TOK_DOT, TOK_ASSIGN,
    TOK_COMP_EQ, TOK_COMP_NOTEQ, TOK_COMP_LTHAN, TOK_COMP_GTHAN,
    TOK_COMP_LTHAN_EQ, TOK_COMP_GTHAN_EQ, TOK_LOG_AND, TOK_LOG_OR, TOK_LOG_NOT,
    TOK_BW_OR, TOK_BW_XOR, TOK_BW_NOT, TOK_BW_LSHIFT, TOK_BW_RSHIFT,

    TOK_IDENT, TOK_COUNT,
} TokenKind;

typedef struct {
    TokenKind kind;
    const char* start;
    int len;

    const char *err;
    int line;
    int col;
} Token;

typedef struct {
    const char *source_string;
    int current_position;

    int line;
    int col;
    int err_line;
    int err_col;
} Lexer;

void lexer_init(Lexer *lx, const char* source);
Token next_token(Lexer *lx);
const char *token_name(TokenKind k);

#endif