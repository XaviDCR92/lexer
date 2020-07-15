#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>

#define LOGV(...) logv(__func__, __LINE__, __VA_ARGS__)
#define LOGVV(...) logvv(__func__, __LINE__, __VA_ARGS__)
#define FATAL_ERROR(...) fatal_error(__func__, __LINE__, __VA_ARGS__)

static int verbose, extra_verbose;
static unsigned int line, start_column, column, pos, start_pos;

#define TOKEN_LIST \
    X(TOKEN_SYMBOL) \
    X(TOKEN_EQ) \
    X(TOKEN_MOV) \
    X(TOKEN_INT) \
    X(TOKEN_HEX) \
    X(TOKEN_FLOAT) \
    X(TOKEN_GT) \
    X(TOKEN_LT) \
    X(TOKEN_GE) \
    X(TOKEN_LE) \
    X(TOKEN_STAR) \
    X(TOKEN_PLUS) \
    X(TOKEN_MINUS) \
    X(TOKEN_SLASH) \
    X(TOKEN_DOT) \
    X(TOKEN_LET) \
    X(TOKEN_IF) \
    X(TOKEN_WHILE) \
    X(TOKEN_FOR) \
    X(TOKEN_ELSE) \
    X(TOKEN_LP) \
    X(TOKEN_RP) \
    X(TOKEN_LC) \
    X(TOKEN_RC) \
    X(TOKEN_COMMA)

enum token_id
{
#define X(x) x,
    TOKEN_LIST
#undef X
};

static const char *token_name(const enum token_id id)
{
    static const char *const names[] =
    {
#define X(x) [x] = #x,
        TOKEN_LIST
#undef X
    };

    if (id < (sizeof names / sizeof *names))
        return names[id];

    return "unknown token";
}

struct
{
    struct token
    {
        enum token_id id;
        char *value;
        unsigned int line, column, pos;
    } *list;
    size_t n;
} tokens;

static void logv(const char *const func, const int line,
        const char *const format, ...)
{
    if ((verbose || extra_verbose) && func && format)
    {
        va_list ap;
        printf("[v] %s:%d: ", func, line);
        va_start(ap, format);
        vprintf(format, ap);
        printf("\n");
        va_end(ap);
    }
}

static void logvv(const char *const func, const int line,
        const char *const format, ...)
{
    if (extra_verbose && func && format)
    {
        va_list ap;
        printf("[vv] %s:%d: ", func, line);
        va_start(ap, format);
        vprintf(format, ap);
        printf("\n");
        va_end(ap);
    }
}

static void cleanup(void)
{
    if (tokens.list)
    {
        free(tokens.list);
    }
}

static void fatal_error(const char *const func, const int line,
        const char *const format, ...)
{
    if (func && format)
    {
        va_list ap;
        fprintf(stderr, "[error]");

        if (verbose)
            fprintf(stderr, " %s:%d: ", func, line);
        else
            fprintf(stderr, ": ");

        va_start(ap, format);
        vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
        va_end(ap);
        cleanup();
        exit(EXIT_FAILURE);
    }
}

static char *read_file(const char *const path)
{
    FILE *const f = fopen(path, "rb");

    if (f)
    {
        fseek(f, 0, SEEK_END);

        {
            const size_t sz = ftell(f);
            char *const buf = malloc((sz + 1) * sizeof *buf);

            rewind(f);

            if (buf)
            {
                const size_t read_bytes = fread(buf,
                        sizeof *buf, sz, f);

                fclose(f);

                if (read_bytes == sz)
                {
                    /* File contents were read succesfully. */
                    LOGV("File %s was opened successfully", path);

                    buf[sz] = '\0';
                    line = 1;

                    return buf;
                }
                else
                    FATAL_ERROR("Could not read %s succesfully. "
                                "Only %d/%d bytes could be read",
                                path, read_bytes, sz);
            }
            else
                FATAL_ERROR("Cannot allocate buffer for file data");

            fclose(f);
        }
    }
    else
        FATAL_ERROR("Input file %s could not be opened", path);

    return NULL;
}

static int alloc_token(const enum token_id id,
        const char *const value)
{
    if (!tokens.list)
        tokens.list = malloc(sizeof *tokens.list);
    else
        tokens.list = realloc(tokens.list, (tokens.n + 1)
                * sizeof *tokens.list);

    if (tokens.list)
    {
        struct token *const new_t = &tokens.list[tokens.n++];

        switch (id)
        {
            case TOKEN_HEX:
                /* Fall through. */
            case TOKEN_FLOAT:
                /* Fall through. */
            case TOKEN_SYMBOL:
                /* Fall through. */
            case TOKEN_INT:
            {
                char *const buf = malloc((strlen(value) + 1) * sizeof *buf);

                if (buf)
                {
                    strcpy(buf, value);
                    new_t->value = buf;
                    break;
                }
                else
                {
                    FATAL_ERROR("could not allocate space for token value");
                    return 1;
                }
            }

            default:
                new_t->value = "";
                break;
        }

        new_t->id = id;
        new_t->line = line;
        new_t->column = start_column;
        new_t->pos = start_pos;
        start_column = 0; start_pos = 0;
        LOGVV("id=%s, value=\"%s\", line=%u, col=%u, pos=%u",
              token_name(new_t->id), new_t->value, new_t->line,
              new_t->column, new_t->pos);
        return 0;

    }
    else
        FATAL_ERROR("could not allocate space for new token");

    return 1;
}

static int is_integer(const char *const symbol)
{
    for (const char *s = symbol; *s; s++)
    {
        if (*s < '0' || *s > '9')
            return 0;
    }

    return 1;
}

static int is_hex(const char *const symbol)
{
    const size_t len = strlen(symbol);
    const size_t minlen = strlen("0x");

    if (len >= minlen && symbol[0] == '0' &&
        (symbol[1] == 'x' || symbol[1] == 'X'))
    {
        if (len > minlen)
        {
            for (const char *s = &symbol[2]; *s; s++)
            {
                if ((*s >= 'a' && *s <= 'f')
                 || (*s >= 'A' && *s <= 'F')
                 || (*s >= '0' && *s <= '9'))
                    continue;
                else
                    return 0;
            }
        }
        else
            FATAL_ERROR("invalid hex number at line %u, column %u",
                        line, start_column);
    }
    else
        return 0;

    return 1;
}

static int is_float(const char *const symbol)
{
    int count = 0;

    for (const char *s = symbol; *s; s++)
    {
        if (count > 1)
            return 0;
        else if (*s < '0' || *s > '9')
        {
            if (*s == '.')
                count++;
            else
                return 0;
        }
    }

    return 1;
}

static int process_symbol(const char *const symbol)
{
    if (strlen(symbol))
    {
        static const struct
        {
            enum token_id id;
            const char *value;
        }res[] =
        {
            {TOKEN_LET, "let"}, {TOKEN_IF, "if"}, {TOKEN_WHILE, "while"},
            {TOKEN_FOR, "for"}, {TOKEN_ELSE, "else"}
        };

        for (size_t i = 0; i < sizeof res / sizeof *res; i++)
            if (!strcmp(symbol, res[i].value))
            {
                if (alloc_token(res[i].id, symbol))
                    return 1;

                return 0;
            }

        if ((*symbol >= 'a' && *symbol <= 'z')
                || (*symbol >= 'A' && *symbol <= 'Z'))
            if (alloc_token(TOKEN_SYMBOL, symbol)) return 1;

        if (is_integer(symbol))
        {
            if (alloc_token(TOKEN_INT, symbol)) return 1;
        }
        else if (is_hex(symbol))
        {
            if (alloc_token(TOKEN_HEX, symbol)) return 1;
        }
        else if (is_float(symbol))
            if (alloc_token(TOKEN_FLOAT, symbol)) return 1;
    }

    return 0;
}

static int tokenize(const char *const buf)
{
    static const struct
    {
        enum token_id id;
        const char *str;
    } ops[] =
    {
        {TOKEN_GE, ">="},
        {TOKEN_LE, "<="},
        {TOKEN_EQ, "=="},
        {TOKEN_LT, "<"},
        {TOKEN_GT, ">"},
        {TOKEN_PLUS, "+"},
        {TOKEN_MINUS, "-"},
        {TOKEN_SLASH, "/"},
        {TOKEN_STAR, "*"},
        {TOKEN_DOT, "."},
        {TOKEN_MOV, "="},
        {TOKEN_LP, "("},
        {TOKEN_RP, ")"},
        {TOKEN_LC, "{"},
        {TOKEN_RC, "}"},
        {TOKEN_COMMA, ","}
    };

    const char *c = buf;
    char symbol[128];
    char *s = symbol;
    int comment = 0;
    column = 1;
#define REWIND do {s = symbol;} while (0)

    while (*c)
    {
start:
        pos++;

        switch (*c)
        {
            case '#':
                comment = 1;
                goto next;

            case '\n':
                line++;
                /* Fall through. */
            case '\r':
                /* Support for CR-only. */
                if (*c == '\r' && (*(c + 1) != '\n'))
                    line++;

                comment = 0;
                column = 1;
                /* Fall through. */
            case '\t':
                /* Fall through. */
            case ' ':
                *s = '\0';

                if (process_symbol(symbol))
                    return 1;

                start_column = 0;
                REWIND;
                goto next;

            default:
                if (comment)
                    goto next;

                break;
        }

        for (size_t i = 0; i < sizeof ops / sizeof *ops; i++)
        {
            const size_t len = strlen(ops[i].str);

            if (!strncmp(c, ops[i].str, len))
            {
                if (s != symbol)
                {
                    *s = '\0';

                    if (ops[i].id == TOKEN_DOT && is_integer(symbol))
                    {
                        /* Exception: dot is used both as operator and decimal
                         * separator according to the context. */
                        goto store;
                    }
                    else if (process_symbol(symbol))
                        return 1;

                    REWIND;
                }

                strcpy(symbol, ops[i].str);
                start_column = column;
                start_pos = pos;
                if (alloc_token(ops[i].id, symbol))
                    return 1;

                REWIND;
                c += len;
                column += len;
                goto start;
            }
        }

        if (!isalnum(*c))
            FATAL_ERROR("invalid character '%c' (0x%hhu), line %u, column %u,"
                        " pos %u", *c, *c, line, column, pos);
store:
        *s++ = *c;
        if (!start_column)
            start_column = column;

        if (!start_pos)
            start_pos = pos;
next:
        if (*c != '\n' && *c != '\r')
            column++;

        c++;
    }

    return 0;
}

static int exec(const char *const path)
{
    char *const buf = read_file(path);

    if (buf) tokenize(buf); else return 1;

    free(buf);
    return 0;
}

int main(const int argc, const char *argv[])
{
    const char *path = NULL;

    for (int i = 1; i < argc; i++)
    {
        const char *const arg = argv[i];

        if (*arg != '-' && !path)
            path = arg;
        else if (*arg == '-')
        {
            if (!strcmp(arg, "-vv"))
                extra_verbose = 1;
            else if (!strcmp(arg, "-v"))
                verbose = 1;
            else
            {
                FATAL_ERROR("unrecognized option %s", arg);
                return EXIT_FAILURE;
            }
        }
        else
        {
            FATAL_ERROR("input path already specified (%s)", path);
            return EXIT_FAILURE;
        }
    }

    if (path)
    {
        if (exec(path)) return EXIT_FAILURE;
    }
    else
        FATAL_ERROR("no input file specified");

    return EXIT_SUCCESS;
}
