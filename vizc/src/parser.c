#include "arena.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static Expr *parse_primary(Parser *p) {
    Token start = p->cur;
    Expr *e = arena_alloc(p-> arena, sizeof(Expr));
    e->line = start.line; e->col = start.col;

    if (check(p, TOK_INT_LIT)) {
        advance(p);
        e->kind = EXPR_INT_LIT;
    } else if (check(p, TOK_SIG_DOLLAR)) {
        advance(p);
        Token name = expect(p, TOK_IDENT, "expected a variable identifier after $");
        e->kind = EXPR_VAR;
        e->var.name = name.start;
        e->var.len = name.len;
    } else {
        error_at(p, p->cur, "expected an expression");
    }
    return e;
}
static Expr *parse_expr(Parser *p) { return parse_primary(p); } // temp

static Stmt *parse_var_decl(Parser *p) {
    Token t = expect(p, TOK_DEFINE_VARIABLE, "expected a variable declaration");
    Stmt *s = arena_alloc(p->arena, sizeof(Stmt));
    s->kind = STMT_VAR_DECL; s->line = t.line; s->col = t.col;
    if (match(p, TOK_LPAREN)) {
        expect(p, TOK_IM, "expected 'im' after '('");
        expect(p, TOK_RPAREN, "expected ')' after 'im'");
        s->var_decl.immutable = 1;
    }
    expect(p, TOK_DOT, "expected a type describer");
    s->var_decl.type = parse_type(p);
    Token name = expect(p, TOK_IDENT, "expected a variable name");
    s->var_decl.name = name.start; s->var_decl.len = name.len;
    if (match(p, TOK_ASSIGN)) s->var_decl.init = parse_expr(p);
    expect(p, TOK_SEMICOLON, "expected a ';' after variable declaration");
    return s;
}
static Stmt *parse_assign(Parser *p) {
    Token t = expect(p, TOK_SIG_DOLLAR, "expected a '$'");
    Token name = expect(p, TOK_IDENT, "expected a variable name after '$'");
    Stmt *s = arena_alloc(p->arena, sizeof(Stmt));
    s->kind = STMT_ASSIGN; s->line = t.line; s->col = t.col;
    s->assign.name = name.start; s->assign.len = name.len;
    switch (p->cur.kind) {
        case TOK_ASSIGN: { s->assign.op = p->cur.kind; advance(p); break; }
        case TOK_ADDASSIGN: { s->assign.op = p->cur.kind; advance(p); break; }
        case TOK_SUBASSIGN: { s->assign.op = p->cur.kind; advance(p); break; }
        case TOK_MULTASSIGN: { s->assign.op = p->cur.kind; advance(p); break; }
        case TOK_DIVASSIGN: { s->assign.op = p->cur.kind; advance(p); break; }
        case TOK_MODULOASSIGN: { s->assign.op = p->cur.kind; advance(p); break; }
        case TOK_ANDASSIGN: { s->assign.op = p->cur.kind; advance(p); break; }
        case TOK_ORASSIGN: { s->assign.op = p->cur.kind; advance(p); break; }
        case TOK_XORASSIGN: { s->assign.op = p->cur.kind; advance(p); break; }
        case TOK_LSHIFTASSIGN: { s->assign.op = p->cur.kind; advance(p); break; }
        case TOK_RSHIFTASSIGN: { s->assign.op = p->cur.kind; advance(p); break; }
        default: error_at(p, p->cur, "expected an assignment operator");
    }
    s->assign.value = parse_expr(p);
    expect(p, TOK_SEMICOLON, "expected a ';' after assignment clause");
    return s;
}
static Stmt *parse_return(Parser *p) {
    Token t = expect(p, TOK_RETURN, "expected a return statement");
    Stmt *s = arena_alloc(p->arena, sizeof(Stmt));
    s->kind = STMT_RETURN; s->line = t.line; s->col = t.col;
    if (!check(p, TOK_SEMICOLON)) s->ret.value = parse_expr(p);
    expect(p, TOK_SEMICOLON, "expected a ';' after 'return'");
    return s;
}

static Stmt *parse_stmt(Parser *p) {
    switch (p->cur.kind) {
        case TOK_DEFINE_VARIABLE: return parse_var_decl(p);
        case TOK_SIG_DOLLAR: return parse_assign(p);
        case TOK_RETURN: return parse_return(p);
        default: error_at(p, p->cur, "expected a statement");
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
    Stmt *head = NULL, *tail = NULL;
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        Stmt *s = parse_stmt(p);
        if (head == NULL) head = s;
        else tail->next = s;
        tail = s;
    }
    expect(p, TOK_RBRACE, "expected '}' closing function block");
    
    Function *f = arena_alloc(p->arena, sizeof(Function));
    f->body = head;
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

