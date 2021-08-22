#include "defs.h"
#include "data.h"
#include "decl.h"

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

void match(int t, char *what) {
    if (Token.token == t) {
        scan(&Token);
    } else {
        fatals("Expected", what);
    }
}

void ident(void) {
    match(T_IDENT, "identifier");
}

void semi(void) {
    match(T_SEMI, ";");
}

void lbrace(void) {
    match(T_LBRACE, "{");
}

void rbrace(void) {
    match(T_RBRACE, "}");
}

void lparen(void) {
    match(T_LPAREN, "(");
}

void rparen(void) {
    match(T_RPAREN, ")");
}

void comma(void) {
    match(T_COMMA, "comma");
}

void warn(char *s) {
    fprintf(stderr, "[Warning]: %s on line %d\n", s, Line);
}

void fatal(char *s) {
    fprintf(stderr, "[Error]: %s on line %d\n", s, Line);
    fclose(Outfile);
    unlink(Outfilename);
    exit(1);
}

void fatals(char *s1, char *s2) {
    fprintf(stderr, "[Error]: %s: %s on line %d\n", s1, s2, Line);
    fclose(Outfile);
    unlink(Outfilename);
    exit(1);
}

void fatald(char *s, int d) {
    fprintf(stderr, "[Error]: %s: %s on line %d\n", s, Tstring[d], Line);
    fclose(Outfile);
    unlink(Outfilename);
    exit(1);
}

void fatalc(char *s, int c) {
    fprintf(stderr, "[Error]: %s: %c on line %d\n", s, c, Line);
    fclose(Outfile);
    unlink(Outfilename);
    exit(1);
}