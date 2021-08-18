#include "defs.h"
#include "data.h"
#include "decl.h"

static struct ASTnode *single_statement(void);

static struct ASTnode *if_statement(void) {
    struct ASTnode *condAST, *trueAST, *falseAST = NULL;

    match(T_IF, "if");
    lparen();
    condAST = binexpr(0);

    if (condAST->op < A_EQ || condAST->op > A_GE)
        condAST = mkastunary(A_TOBOOL, condAST->type, condAST, NULL, 0);
    rparen();

    trueAST = compound_statement();

    if (Token.token == T_ELSE) {
        scan(&Token);
        falseAST = compound_statement();
    }

    return (mkastnode(A_IF, P_NONE, condAST, trueAST, falseAST, NULL, 0));
}

static struct ASTnode *while_statement(void) {
    struct ASTnode *condAST, *bodyAST;

    match(T_WHILE, "while");
    lparen();

    condAST = binexpr(0);
    if (condAST->op < A_EQ || condAST->op > A_GE)
        condAST = mkastunary(A_TOBOOL, condAST->type, condAST, NULL, 0);
    rparen();

    Looplevel++;
    bodyAST = compound_statement();
    Looplevel--;

    return (mkastnode(A_WHILE, P_NONE, condAST, NULL, bodyAST, NULL, 0));
}

static struct ASTnode *for_statement(void) {
    struct ASTnode *condAST, *bodyAST;
    struct ASTnode *preopAST, *postopAST;
    struct ASTnode *tree;

    match(T_FOR, "for");
    lparen();

    preopAST = single_statement();
    semi();

    condAST = binexpr(0);
    if (condAST->op < A_EQ || condAST->op > A_GE)
        condAST = mkastunary(A_TOBOOL, condAST->type, condAST, NULL, 0);
    semi();

    postopAST = single_statement();
    rparen();

    Looplevel++;
    bodyAST = compound_statement();
    Looplevel--;

    tree = mkastnode(A_GLUE, P_NONE, bodyAST, NULL, postopAST, NULL, 0);

    tree = mkastnode(A_WHILE, P_NONE, condAST, NULL, tree, NULL, 0);

    return (mkastnode(A_GLUE, P_NONE, preopAST, NULL, tree, NULL, 0));
}

static struct ASTnode *return_statement(void) {
    struct ASTnode *tree;

    if (Functionid->type == P_VOID)
        fatal("Can't return from a void function");

    match(T_RETURN, "return");
    lparen();

    tree = binexpr(0);

    tree = modify_type(tree, Functionid->type, 0);
    if (tree == NULL)
        fatal("Incompatible type in return statement");

    tree = mkastunary(A_RETURN, P_NONE, tree, NULL, 0);

    rparen();
    semi();
    return (tree);
}

static struct ASTnode *break_statement(void) {
    if (Looplevel == 0)
        fatal("no loop to break out from");
    scan(&Token);
    semi();
    return (mkastleaf(A_BREAK, P_NONE, NULL, 0));
}

static struct ASTnode *continue_statement(void) {
    if (Looplevel == 0)
        fatal("no loop to continue to");
    scan(&Token);
    semi();
    return (mkastleaf(A_CONTINUE, P_NONE, NULL, 0));
}

static struct ASTnode *single_statement(void) {
    int type, class = C_LOCAL;
    struct symtable *ctype;

    switch (Token.token) {
        case T_IDENT:
            if (findtypedef(Text) == NULL)
                return (binexpr(0));
        case T_CHAR:
        case T_INT:
        case T_LONG:
        case T_STRUCT:
        case T_UNION:
        case T_ENUM:
        case T_TYPEDEF:
            type = parse_type(&ctype, &class);
            ident();
            var_declaration(type, ctype, class);
            semi();
            return (NULL);
        case T_IF:
            return (if_statement());
        case T_WHILE:
            return (while_statement());
        case T_FOR:
            return (for_statement());
        case T_RETURN:
            return (return_statement());
        case T_BREAK:
            return (break_statement());
        case T_CONTINUE:
            return (continue_statement());
        default:
            return (binexpr(0));
    }
}

struct ASTnode *compound_statement(void) {
    struct ASTnode *left = NULL;
    struct ASTnode *tree;

    lbrace();
    while (1) {

        tree = single_statement();

        if (tree != NULL && (tree->op == A_ASSIGN || tree->op == A_FUNCCALL))
            semi();

        if (tree != NULL) {
            if (left == NULL)
                left = tree;
            else
                left = mkastnode(A_GLUE, P_NONE, left, NULL, tree, NULL, 0);
        }

        if (Token.token == T_RBRACE) {
            rbrace();
            return (left);
        }
    }
}