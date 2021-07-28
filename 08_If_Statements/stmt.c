#include "defs.h"
#include "data.h"
#include "decl.h"

static struct ASTnode *print_statement(void) {
    struct ASTnode *tree;
    int reg;

    // scan in the next token
    match(T_PRINT, "print");

    tree = binexpr(0);
    tree = mkastunary(A_PRINT, tree, 0);

    semi();
    return tree;
}

static struct ASTnode *assignment_statement(void) {
    struct ASTnode *left, *right, *tree;
    int id;

    ident();
    if ((id = findglob(Text)) == -1)
        fatals("Undeclared variable", Text);

    right = mkastleaf(A_LVIDENT, id);

    match(T_ASSIGN, "=");

    left = binexpr(0);

    tree = mkastnode(A_ASSIGN, left, NULL, right, 0);

    semi();

    return tree;
}

struct ASTnode *if_statement(void) {
    struct ASTnode *condAST, *trueAST, *falseAST = NULL;

    match(T_IF, "if");
    lparen();
    condAST = binexpr(0);

    if (condAST->op < A_EQ || condAST->op > A_GE)
        fatal("Bad comparison operator");
    rparen();

    trueAST = compound_statement();

    if (Token.token == T_ELSE) {
        scan(&Token);
        falseAST = compound_statement();
    }

    return mkastnode(A_IF, condAST, trueAST, falseAST, 0);
}
struct ASTnode *compound_statement(void) {
    struct ASTnode *left = NULL;
    struct ASTnode *tree;

    lbrace();
    while (1) {
        switch (Token.token) {
            case T_PRINT:
                tree = print_statement();
                break;
            case T_INT:
                var_declaration();
                tree = NULL;
                break;
            case T_IDENT:
                tree = assignment_statement();
                break;
            case T_IF:
                tree = if_statement();
                break;
            case T_RBRACE:
                rbrace();
                return left;
            default:
                fatald("Syntax error, token", Token.token);
        }

        if (tree) {
            if (left == NULL)
                left = tree;
            else
                left = mkastnode(A_GLUE, left, NULL, tree, 0);
        }
    }
}