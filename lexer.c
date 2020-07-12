#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>

#define LOGV(...) logv(__func__, __LINE__, __VA_ARGS__)
#define LOGVV(...) logvv(__func__, __LINE__, __VA_ARGS__)
#define FATAL_ERROR(...) fatal_error(__func__, __LINE__, __VA_ARGS__)

static int verbose, extra_verbose;
static unsigned int line;

#define TOKEN_LIST \
    X(TOKEN_SYMBOL) \
    X(TOKEN_EQ) \
    X(TOKEN_MOV) \
    X(TOKEN_INT) \
    X(TOKEN_GT) \
    X(TOKEN_LT) \
    X(TOKEN_GE) \
    X(TOKEN_LE) \
    X(TOKEN_STAR) \
    X(TOKEN_PLUS) \
    X(TOKEN_SLASH) \
    X(TOKEN_DOT) \
    X(TOKEN_LET) \
    X(TOKEN_IF) \
    X(TOKEN_WHILE) \
    X(TOKEN_FOR) \
    X(TOKEN_ELSE)

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
        unsigned int line;
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
                {
                    FATAL_ERROR("Could not read %s succesfully. "
                                "Only %d/%d bytes could be read",
                                path, read_bytes, sz);
                }
            }
            else
            {
                FATAL_ERROR("Cannot allocate buffer for file data");
            }

            fclose(f);
        }
    }
    else
    {
        FATAL_ERROR("Input file %s could not be opened", path);
    }

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
                    fprintf(stderr, "could not allocate space for"
                            "token value\n");
                    return 1;
                }
            }
            default:
                new_t->value = "";
        }

        new_t->id = id;
        new_t->line = line;
        LOGVV("id=%s, value=\"%s\", line=%u", token_name(new_t->id),
                new_t->value, new_t->line);
        return 0;

    }
    else
        fprintf(stderr, "could not allocate space for new token\n");

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

        if (*symbol >= '0' && *symbol <= '9')
            if (alloc_token(TOKEN_INT, symbol)) return 1;
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
        {TOKEN_SLASH, "/"},
        {TOKEN_STAR, "*"},
        {TOKEN_DOT, "."},
        {TOKEN_MOV, "="}
    };

    const char *c = buf;
    char symbol[128];
    char *s = symbol;
#define REWIND do {s = symbol; *s = '\0';} while (0)

    while (*c)
    {
start:
        switch (*c)
        {
            case '\n':
                line++;
                /* Fall through. */
            case '\t':
                /* Fall through. */
            case ' ':
                *s = '\0';

                if (process_symbol(symbol))
                    return 1;

                REWIND;
                goto next;

            default:
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
                    if (process_symbol(symbol))
                        return 1;
                    REWIND;
                }

                strcpy(symbol, ops[i].str);
                if (alloc_token(ops[i].id, symbol))
                    return 1;

                REWIND;
                c += len;
                goto start;
            }
        }

        *s++ = *c;
next:
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
                fprintf(stderr, "unrecognized option %s\n", arg);
                return EXIT_FAILURE;
            }
        }
        else
        {
            fprintf(stderr, "input path already specified (%s)", path);
            return EXIT_FAILURE;
        }
    }

    if (path)
    {
        if (exec(path)) return EXIT_FAILURE;
    }
    else
        fprintf(stderr, "no input file specified\n");

    return EXIT_FAILURE;
}

