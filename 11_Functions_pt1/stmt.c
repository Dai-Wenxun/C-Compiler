#include "defs.h"
#include "data.h"
#include "decl.h"

static struct ASTnode *single_statement(void);

static struct ASTnode *print_statement(void) {
    struct ASTnode *tree;

    // scan in the next token
    match(T_PRINT, "print");

    tree = binexpr(0);
    tree = mkastunary(A_PRINT, tree, 0);

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

struct ASTnode *while_statement(void) {
    struct ASTnode *condAST, *bodyAST;

    match(T_WHILE, "while");
    lparen();
    condAST = binexpr(0);
    if (condAST->op < A_EQ || condAST->op > A_GE)
        fatal("Bad comparison operator");
    rparen();

    bodyAST = compound_statement();

    return mkastnode(A_WHILE, condAST, NULL, bodyAST, 0);
}

struct ASTnode *for_statement(void) {
    struct ASTnode *condAST, *bodyAST;
    struct ASTnode *preopAST, *postopAST;
    struct ASTnode *tree;

    match(T_FOR, "for");
    lparen();

    preopAST = single_statement();
    semi();

    condAST = binexpr(0);
    if (condAST->op < A_EQ || condAST->op > A_GE)
        fatal("Bad comparison operator");
    semi();

    postopAST = single_statement();
    rparen();

    bodyAST = compound_statement();

    tree = mkastnode(A_GLUE, bodyAST, NULL, postopAST, 0);

    tree = mkastnode(A_WHILE, condAST, NULL, tree, 0);

    return mkastnode(A_GLUE, preopAST, NULL, tree, 0);
}

static struct ASTnode *single_statement(void) {
    switch (Token.token) {
        case T_PRINT:
            return print_statement();
        case T_INT:
            var_declaration();
            return NULL;
        case T_IDENT:
            return assignment_statement();
        case T_IF:
            return if_statement();
        case T_WHILE:
            return while_statement();
        case T_FOR:
            return for_statement();
        default:
            fatald("Syntax error, token", Token.token);
    }
}

struct ASTnode *compound_statement(void) {
    struct ASTnode *left = NULL;
    struct ASTnode *tree;

    lbrace();
    while (1) {

        tree = single_statement();

        if (tree != NULL && (tree->op == A_PRINT || tree->op == A_ASSIGN))
            semi();

        if (tree != NULL) {
            if (left == NULL)
                left = tree;
            else
                left = mkastnode(A_GLUE, left, NULL, tree, 0);
        }

        if (Token.token == T_RBRACE) {
            rbrace();
            return left;
        }
    }
}