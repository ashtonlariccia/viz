#ifndef AST_H
#define AST_H

#include <stdint.h>
#include "lexer.h"

typedef enum {
    TYPE_NONE,
    TYPE_I8, TYPE_I16, TYPE_I32, TYPE_I64,
    TYPE_U8, TYPE_U16, TYPE_U32, TYPE_U64,
    TYPE_UZ,
} Type;

typedef enum {
    EXPR_INT_LIT,
    EXPR_VAR,
    EXPR_UNARY,
    EXPR_BINARY,
} ExprKind;

typedef struct Expr Expr;
struct Expr {
    ExprKind kind;
    Type type;
    int line, col;
    union {
        struct { Expr *lhs, *rhs; TokenKind op; } binary;
        struct { uint64_t value; } int_lit;
        struct { const char *name; int len; } var;
        struct { TokenKind op; Expr *operand; } unary;
    };
};

typedef enum {
    STMT_VAR_DECL, STMT_ASSIGN, STMT_RETURN,
} StmtKind;

typedef struct Stmt Stmt;
struct Stmt {
    StmtKind kind;
    Stmt *next;
    int line, col;
    union {
        struct {
            Type type;
             const char *name;
             int len; int immutable;
             Expr *init;
        } var_decl;
        struct { const char *name; int len; TokenKind op; Expr *value; } assign;
        struct { Expr *value; } ret;
    };
};

typedef struct {
    const char *name;
    int len; Type ret_type;
    Stmt *body;
    int line, col;
} Function;

#endif