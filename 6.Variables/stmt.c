#include "defs.h"
#include "data.h"
#include "decl.h"

void print_statement(void) {
    struct ASTnode *tree;
    int reg;

    // scan in the next token
    match(T_PRINT, "print");

    tree = binexpr(0);

    reg = genAST(tree, -1);
    genprintint(reg);
    genfreeregs();

    semi();
    if (Token.token == T_EOF)
        return;
}

void assignment_statement(void) {
    struct ASTnode *left, *right, *tree;
    int id;

    ident();
    if ((id = findglob(Text)) == -1)
        fatals("Undeclared variable", Text);

    right = mkastleaf(A_LVIDENT, id);

    match(T_EQUALS, "=");

    left = binexpr(0);
}

void statements(void) {
    while (1) {
        switch (Token.token) {
            case T_PRINT:
                print_statement();
                break;
            case T_INT:
                var_declaration();
                break;
            case T_IDENT:
                assignment_statement();
                break;
            case T_EOF:
                return;
            default:
                fatald("Syntax error, token", Token.token);
        }
    }
}