#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lexer.h"

static void print_usage() { fprintf(stderr, "USAGE: vizc <args>\n\t\t-> --tokens <filepath>\n"); }

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "--tokens") != 0) { print_usage(); return 1; }
    
    FILE *fp = fopen(argv[2], "rb");
    if (fp == NULL) { perror(argv[2]); return 1; }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    
    char *buf = malloc(size + 1);
    if (buf == NULL) { perror("malloc"); return 1; }
    
    size_t bytes_read = fread(buf, 1, size, fp);
    buf[bytes_read] = '\0';
    
    fclose(fp);

    Lexer lx;
    lexer_init(&lx, buf);
    for (;;) {
        Token t = next_token(&lx);
        if (t.kind == TOK_ERROR) {
            fprintf(stderr, "%s:%d:%d: error: %s\n", argv[2], t.line, t.col, t.err);
            return 1;
        }
        printf("%d:%d\t%-16s'%.*s'\n", t.line, t.col, token_name(t.kind), t.len, t.start);
        if (t.kind == TOK_EOF) break;
    }
    free(buf);
    return 0;
}