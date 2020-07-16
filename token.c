#include "token.h"

const char *token_name(const enum token_id id)
{
    static const char *const names[] =
    {
        "TOKEN_SYMBOL",
        "TOKEN_EQ",
        "TOKEN_MOV",
        "TOKEN_INT",
        "TOKEN_HEX",
        "TOKEN_FLOAT",
        "TOKEN_GT",
        "TOKEN_LT",
        "TOKEN_GE",
        "TOKEN_LE",
        "TOKEN_STAR",
        "TOKEN_PLUS",
        "TOKEN_MINUS",
        "TOKEN_SLASH",
        "TOKEN_DOT",
        "TOKEN_LET",
        "TOKEN_IF",
        "TOKEN_WHILE",
        "TOKEN_FOR",
        "TOKEN_ELSE",
        "TOKEN_LP",
        "TOKEN_RP",
        "TOKEN_LC",
        "TOKEN_RC",
        "TOKEN_COMMA"
    };

    if (id < (sizeof names / sizeof *names))
        return names[id];

    return "unknown token";
}
