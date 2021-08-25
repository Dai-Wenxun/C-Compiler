#include "defs.h"
#include "data.h"
#include "decl.h"

static int chrpos(char *s, int c) {
    char *p;

    p = strchr(s, c);
    return (p ? p-s : -1);
}

static int next(void) {
    int c;

    if (Putback) {
        c = Putback;
        Putback = 0;
        return (c);
    }

    c = fgetc(Infile);

    if (c == '\n')
        Line++;

    return (c);
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

    return (c);
}

static int scanch(void) {
    int c;

    c = next();
    if (c == '\\') {
        switch (c = next()) {
            case 'a':
                return '\a';
            case 'b':
                return '\b';
            case 'f':
                return '\f';
            case 'n':
                return '\n';
            case 'r':
                return '\r';
            case 't':
                return '\t';
            case 'v':
                return '\v';
            case '0':
                return '\0';
            case '\\':
                return '\\';
            case '\"':
                return '\"';
            case '\'':
                return '\'';
            default:
                fatalc("unknown escape sequence", c);
        }
    }
    return (c);
}

static int scanint(int c) {
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        val = 10 * val + k;
        c = next();
    }

    putback(c);
    return (val);
}

static void scanstr(void) {
    int i, c;

    for (i = 0; i < TEXTLEN + 1; i++) {
        if ((c = scanch()) == '\"') {
            Text[i] = '\0';
            return;
        }
        Text[i] = (char)c;
    }
    fatal("string literal too long");
}

static void scanident(int c) {
    int i = 0;

    while (isalpha(c) || isdigit(c) || '_' == c) {
        if (TEXTLEN == i) {
            fatal("identifier too long");
        } else if (i < TEXTLEN) {
            Text[i++] = (char)c;
        }
        c = next();
    }

    putback(c);
    Text[i] = '\0';
}

static int keyword(char *s) {
    switch (*s) {
        case 'b':
            if (!strcmp(s, "break"))
                return (T_BREAK);
            break;
        case 'c':
            if (!strcmp(s, "char"))
                return (T_CHAR);
            if (!strcmp(s, "continue"))
                return (T_CONTINUE);
            if (!strcmp(s, "case"))
                return (T_CASE);
            break;
        case 'd':
            if (!strcmp(s, "default"))
                return (T_DEFAULT);
            break;
        case 'e':
            if (!strcmp(s, "else"))
                return (T_ELSE);
            if (!strcmp(s, "enum"))
                return (T_ENUM);
            if (!strcmp(s, "extern"))
                return (T_EXTERN);
            break;
        case 'f':
            if (!strcmp(s, "for"))
                return (T_FOR);
            break;
        case 'i':
            if (!strcmp(s, "if"))
                return (T_IF);
            if (!strcmp(s, "int"))
                return (T_INT);
            break;
        case 'l':
            if (!strcmp(s, "long"))
                return (T_LONG);
            break;
        case 'r':
            if (!strcmp(s, "return"))
                return (T_RETURN);
            break;
        case 's':
            if (!strcmp(s, "struct"))
                return (T_STRUCT);
            if (!strcmp(s, "switch"))
                return (T_SWITCH);
            break;
        case 't':
            if (!strcmp(s, "typedef"))
                return (T_TYPEDEF);
            break;
        case 'u':
            if (!strcmp(s, "union"))
                return (T_UNION);
            break;
        case 'v':
            if (!strcmp(s, "void"))
                return (T_VOID);
            break;
        case 'w':
            if (!strcmp(s, "while"))
                return (T_WHILE);
            break;
    }
    return (0);
}

char *Tstring[] = {
        "EOF", "=", "||", "&&", "|", "^", "&",
        "==", "!=", "<", ">", "<=", ">=", "<<", ">>",
        "+", "-", "*", "/", "++", "--", "~", "!",
        "void", "char", "int", "long",
        "struct", "union", "enum",
        "if", "else", "while", "for", "return",
        "typedef", "extern", "break", "continue",
        "switch", "case", "default",
        "intlit", "strlit", "identifier",
        "{", "}", "(", ")", "[", "]",
        ";", ",", ".", "->", ":"
};

int scan(struct token *t) {
    int c, tokentype;

    c = skip();

    switch (c) {
        case EOF:
            t->token = T_EOF;
            return (0);
        case '+':
            if ((c=next()) == '+') {
                t->token = T_INC;
            } else {
                putback(c);
                t->token = T_PLUS;
            }
            break;
        case '-':
            if ((c=next()) == '-') {
                t->token = T_DEC;
            } else if (c == '>') {
                t->token = T_ARROW;
            } else {
                putback(c);
                t->token = T_MINUS;
            }
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
        case '[':
            t->token = T_LBRACKET;
            break;
        case ']':
            t->token = T_RBRACKET;
            break;
        case '~':
            t->token = T_INVERT;
            break;
        case '^':
            t->token = T_XOR;
            break;
        case ',':
            t->token = T_COMMA;
            break;
        case '.':
            t->token = T_DOT;
            break;
        case ':':
            t->token = T_COLON;
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
                putback(c);
                t->token = T_LOGNOT;
            }
            break;
        case '<':
            if ((c=next()) == '=') {
                t->token = T_LE;
            } else if (c == '<') {
                t->token = T_LSHIFT;
            } else {
                putback(c);
                t->token = T_LT;
            }
            break;
        case '>':
            if ((c=next()) == '=') {
                t->token = T_GE;
            } else if (c == '>') {
                t->token = T_RSHIFT;
            } else {
                putback(c);
                t->token = T_GT;
            }
            break;
        case '&':
            if ((c=next()) == '&') {
                t->token = T_LOGAND;
            } else {
                putback(c);
                t->token = T_AMPER;
            }
            break;
        case '|':
            if ((c=next() == '|')) {
                t->token = T_LOGOR;
            } else {
                putback(c);
                t->token = T_OR;
            }
            break;
        case '\'':
            t->intvalue = scanch();
            t->token = T_INTLIT;
            if (next() != '\'')
                fatal("expected '\'' at end of char literal");
            break;
        case '\"':
            scanstr();
            t->token = T_STRLIT;
            break;
        default:
            if (isdigit(c)) {
                t->intvalue = scanint(c);
                t->token = T_INTLIT;
                break;
            } else if (isalpha(c) || '_' == c) {
                scanident(c);

                tokentype = keyword(Text);
                if (tokentype) {
                    t->token = tokentype;
                    break;
                }

                t->token = T_IDENT;
                break;
            }

            fatalc("unrecognised character", c);
    }

    t->tokptr = Tstring[t->token];
    return (1);
}
