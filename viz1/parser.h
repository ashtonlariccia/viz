#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "arena.h"

Function *parse_program(const char *filename, const char *src, Arena *arena);

#endif