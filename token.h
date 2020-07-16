#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

enum token_id
{
    TOKEN_SYMBOL,
    TOKEN_EQ,
    TOKEN_MOV,
    TOKEN_INT,
    TOKEN_HEX,
    TOKEN_FLOAT,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_GE,
    TOKEN_LE,
    TOKEN_STAR,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_SLASH,
    TOKEN_DOT,
    TOKEN_LET,
    TOKEN_IF,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_ELSE,
    TOKEN_LP,
    TOKEN_RP,
    TOKEN_LC,
    TOKEN_RC,
    TOKEN_COMMA
};

struct token_list
{
    struct token
    {
        enum token_id id;
        char *value;
        const char *file;
        unsigned int line, column, pos;
    } *list;
    size_t n;
};

const char *token_name(enum token_id id);

#endif
