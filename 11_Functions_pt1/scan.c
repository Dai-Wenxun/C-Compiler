#include "defs.h"
#include "data.h"
#include "decl.h"

static int chrpos(char *s, int c) {
    char *p;

    p = strchr(s, c);
    return p ? p-s : -1;
}

static int next(void) {
    int c;

    if (Putback) {
        c = Putback;
        Putback = 0;
        return c;
    }

    c = fgetc(Infile);
    if (c == '\n')
        Line++;

    return c;
}

static void putback(int c) {
    Putback = c;
}

static int skip(void) {
    int c;

    c = next();

    while (' ' == c || '\t' == c || '\n' == c || '\f' == c || '\r' == c) {
        c = next();
    }

    return c;
}

static int scanint(int c) {
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        val = 10 * val + k;
        c = next();
    }

    putback(c);
    return val;
}

static int scanident(int c, char *buf, int lim) {
    int i = 0;

    while (isalpha(c) || isdigit(c) || '_' == c) {
        if (lim - 1 == i) {
            fatal("identifier too long");
        } else if (i < lim - 1) {
            buf[i++] = (char)c;
        }
        c = next();
    }

    putback(c);
    buf[i] = '\0';
    return i;
}

static int keyword(char *s) {
    switch (*s) {
        case 'e':
            if (!strcmp(s, "else"))
                return T_ELSE;
            break;
        case 'f':
            if (!strcmp(s, "for"))
                return T_FOR;
            break;
        case 'i':
            if (!strcmp(s, "if"))
                return T_IF;
            if (!strcmp(s, "int"))
                return T_INT;
            break;
        case 'p':
            if (!strcmp(s, "print"))
                return T_PRINT;
            break;
        case 'v':
            if (!strcmp(s, "void"))
                return T_VOID;
            break;
        case 'w':
            if (!strcmp(s, "while"))
                return T_WHILE;
            break;
    }
    return 0;
}

int scan(struct token *t) {
    int c, tokentype;

    c = skip();

    switch (c) {
        case EOF:
            t->token = T_EOF;
            return 0;
        case '+':
            t->token = T_PLUS;
            break;
        case '-':
            t->token = T_MINUS;
            break;
        case '*':
            t->token = T_STAR;
            break;
        case '/':
            t->token = T_SLASH;
            break;
        case ';':
            t->token = T_SEMI;
            break;
        case '{':
            t->token = T_LBRACE;
            break;
        case '}':
            t->token = T_RBRACE;
            break;
        case '(':
            t->token = T_LPAREN;
            break;
        case ')':
            t->token = T_RPAREN;
            break;
        case '=':
            if ((c=next()) == '=') {
                t->token = T_EQ;
            } else {
                putback(c);
                t->token = T_ASSIGN;
            }
            break;
        case '!':
            if ((c=next()) == '=') {
                t->token = T_NE;
            } else {
                fatalc("Unrecognised character", c);
            }
            break;
        case '<':
            if ((c=next()) == '=') {
                t->token = T_LE;
            } else {
                putback(c);
                t->token = T_LT;
            }
            break;
        case '>':
            if ((c=next()) == '=') {
                t->token = T_GE;
            } else {
                putback(c);
                t->token = T_GT;
            }
            break;
        default:
            if (isdigit(c)) {
                t->intvalue = scanint(c);
                t->token = T_INTLIT;
                break;
            } else if (isalpha(c) || '_' == c) {
                scanident(c, Text, TEXTLEN);

                tokentype = keyword(Text);
                if (tokentype) {
                    t->token = tokentype;
                    break;
                }
                // Not a keyword, so must be an identifier.
                t->token = T_IDENT;
                break;
            }

            fatalc("Unrecognised character", c);
    }
    return 1;
}
