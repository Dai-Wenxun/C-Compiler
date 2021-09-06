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

static int hexchar(void) {
    int c, h, n = 0, f = 0;

    while (isxdigit(c = next())) {
        h = chrpos("0123456789abcdef", tolower(c));

        n = n * 16 + h;

        f = 1;
    }
    putback(c);

    if (!f)
        fatal("missing digits after '\\x'");
    if (n > 255)
        fatal("value out of range after '\\x'");
    return n;
}

static int scanch(void) {
    int i, c, c2;

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
            case '\\':
                return '\\';
            case '\"':
                return '\"';
            case '\'':
                return '\'';
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                for (i = c2 = 0; isdigit(c) && c < '8'; c = next()) {
                    if (++i > 3)
                        break;
                    c2 = c2 * 8 + (c - '0');
                }
                putback(c);
                return (c2);
            case 'x':
                return hexchar();
            default:
                fatalc("unknown escape sequence", c);
        }
    }
    return (c);
}

static int scanint(int c) {
    int k, val = 0, radix = 10;

    if (c == '0') {
        c = next();
        if (c == 'x') {
            radix = 16;
            c =  next();
        } else {
            radix = 8;
        }
    }

    while ((k = chrpos("0123456789abcdef", tolower(c))) >= 0) {
        if (k >= radix)
            fatalc("invalid digit in integer literal", c);
        val = radix * val + k;
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
            if (!strcmp(s, "sizeof"))
                return (T_SIZEOF);
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
        "EOF", "=", "+=", "-=", "*=", "/=",
        "||", "&&", "|", "^", "&",
        "==", "!=", "<", ">", "<=", ">=", "<<", ">>",
        "+", "-", "*", "/", "++", "--", "~", "!",
        "void", "char", "int", "long",
        "struct", "union", "enum",
        "if", "else", "while", "for", "return",
        "typedef", "extern", "break", "continue",
        "switch", "case", "default", "sizeof",
        "intlit", "strlit", "identifier",
        "{", "}", "(", ")", "[", "]",
        ";", ",", ".", "->", ":"
};

static int minus_modify = 0;

void dealing_minus(struct token *t) {
    if (minus_modify == 0) {
        t->token = T_MINUS;
        t->intvalue = -t->intvalue;
        t->tokptr = Tstring[t->token];
        minus_modify = 1;
    } else {
        t->token = T_INTLIT;
        t->tokptr = Tstring[t->token];
        minus_modify = 0;
    }
}

int scan(struct token *t) {
    int c, tokentype;

    if (minus_modify) {
        dealing_minus(t);
        return (1);
    }

    if (Peektoken.token != 0) {
        t->token = Peektoken.token;
        t->intvalue = Peektoken.intvalue;
        t->tokptr = Peektoken.tokptr;
        Peektoken.token = 0;
        return (1);
    }
    c = skip();

    switch (c) {
        case EOF:
            t->token = T_EOF;
            return (0);
        case '+':
            if ((c=next()) == '+') {
                t->token = T_INC;
            } else if (c == '=') {
                t->token = T_ASPLUS;
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
            } else if (isdigit(c)) {
                t->intvalue = -scanint(c);
                t->token = T_INTLIT;
                break;
            } else if (c == '=') {
                t->token = T_ASMINUS;
            } else {
                putback(c);
                t->token = T_MINUS;
            }
            break;
        case '*':
            if ((c = next()) == '=') {
                t->token = T_ASSTAR;
            } else {
                putback(c);
                t->token = T_STAR;
            }
            break;
        case '/':
            if ((c = next()) == '=') {
                t->token = T_ASSLASH;
            } else {
                putback(c);
                t->token = T_SLASH;
            }
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
