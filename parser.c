#include "arena.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    Lexer lx;
    Token cur;
    Arena *arena;
    const char *filename;
} Parser;

static _Noreturn void error_at(Parser *p, Token tok, const char *msg) {
    fprintf(stderr, "%s:%d:%d: error: %s\n", p->filename, tok.line, tok.col, msg);
    exit(1);
}

static void advance(Parser *p) {
    p->cur = next_token(&p->lx);
    if (p->cur.kind == TOK_ERROR) {  error_at(p, p->cur, p->cur.err); }
}

static int check(Parser *p, TokenKind kind) { return p->cur.kind == kind; }

static int match(Parser *p, TokenKind kind) {
    if (check(p, kind)) { advance(p); return 1; }
    else return 0;
}

static Token expect(Parser *p, TokenKind kind, const char *msg) {
    if (!check(p, kind)) error_at(p, p->cur, msg);
    Token t = p->cur; advance(p);
    return t;
}

static Type parse_type(Parser *p) {
    switch (p->cur.kind) {
        // match each type-kw token to TYPE_ value, then advance(p)
        case TOK_INT8: { advance(p); return TYPE_I8; }
        case TOK_INT16: { advance(p); return TYPE_I16; }
        case TOK_INT32: { advance(p); return TYPE_I32; }
        case TOK_INT64: { advance(p); return TYPE_I64; }
        case TOK_UINT8: { advance(p); return TYPE_U8; }
        case TOK_UINT16: { advance(p); return TYPE_U16; }
        case TOK_UINT32: { advance(p); return TYPE_U32; }
        case TOK_UINT64: { advance(p); return TYPE_U64; }
        case TOK_UZ: { advance(p); return TYPE_UZ; }
        default: error_at(p, p->cur, "expected a type");
    }
}

static Function *parse_function(Parser *p) {
    Token fn_tok = expect(p, TOK_DEFINE_FUNCTION, "expected a function declaration");
    Type ret = TYPE_NONE;
    if (match(p, TOK_DOT)) ret = parse_type(p);
    Token name = expect(p, TOK_IDENT, "expected function name");
    expect(p, TOK_LPAREN, "expected '(' after function name");
    expect(p, TOK_RPAREN, "expected ')' after function args");
    expect(p, TOK_LBRACE, "expected '{' opening function block");
    expect(p, TOK_RBRACE, "expected '}' closing function block");
    
    Function *f = arena_alloc(p->arena, sizeof(Function));
    f->ret_type = ret; f->line = fn_tok.line; f->col = fn_tok.col;
    f->name = name.start; f->len = name.len;

    return f;
}

Function *parse_program(const char *filename, const char *src, Arena *arena) {
    Parser p;
    p.filename = filename; p.arena = arena; lexer_init(&p.lx, src);
    advance(&p); // p.cur holds the first token
    Function *f = parse_function(&p);
    expect(&p, TOK_EOF, "expected end of file after function");
    return f;
}