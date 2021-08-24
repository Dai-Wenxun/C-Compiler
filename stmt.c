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

    trueAST = single_statement();

    if (Token.token == T_ELSE) {
        scan(&Token);
        falseAST = single_statement();
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
    bodyAST = single_statement();
    Looplevel--;

    return (mkastnode(A_WHILE, P_NONE, condAST, NULL, bodyAST, NULL, 0));
}

static struct ASTnode *for_statement(void) {
    struct ASTnode *condAST, *bodyAST;
    struct ASTnode *preopAST, *postopAST;
    struct ASTnode *tree;

    match(T_FOR, "for");
    lparen();

    preopAST = expression_list(T_SEMI);
    semi();

    condAST = binexpr(0);
    if (condAST->op < A_EQ || condAST->op > A_GE)
        condAST = mkastunary(A_TOBOOL, condAST->type, condAST, NULL, 0);
    semi();

    postopAST = expression_list(T_RPAREN);
    rparen();

    Looplevel++;
    bodyAST = single_statement();
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
    if (Looplevel == 0 && Switchlevel == 0)
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

static struct ASTnode *switch_statement(void) {
    struct ASTnode *left, *root, *c, *casetree=NULL, *casetail;
    int inloop = 1, casecount = 0;
    int seendefault = 0;
    int ASTop, casevalue;

    scan(&Token);
    lparen();

    left = binexpr(0);
    rparen();
    lbrace();

    if (!inttype(left->type))
        fatal("Switch expression is not of integer type");

    root = mkastunary(A_SWITCH, P_NONE, left, NULL, 0);

    Switchlevel++;
    while (inloop) {
        switch (Token.token) {
            case T_RBRACE:
//                if (casecount == 0)
//                    fatal("No cases in switch");
                inloop = 0;
                break;
            case T_CASE:
            case T_DEFAULT:
                if (seendefault)
                    fatal("case or default after existing default");

                if (Token.token  == T_DEFAULT) {
                    ASTop = A_DEFAULT;
                    seendefault = 1;
                    scan(&Token);
                } else {
                    ASTop = A_CASE;
                    casecount++;
                    scan(&Token);
                    left = binexpr(0);
                    if (left->op != A_INTLIT)
                        fatal("Expecting integer literal for case value");
                    casevalue = left->intvalue;

                    for (c = casetree; c != NULL; c = c->right)
                        if (casevalue == c->intvalue)
                            fatal("Duplicate case value");
                }

                match(T_COLON, ":");
                left = compound_statement(1);

                if (casetree == NULL) {
                    casetree = casetail = mkastunary(ASTop, P_NONE, left, NULL, casevalue);
                } else {
                    casetail->right = mkastunary(ASTop, P_NONE, left, NULL, casevalue);
                    casetail = casetail->right;
                }
                break;

            default:
                fatald("Unexpected token in switch", Token.token);
        }
    }
    Switchlevel--;

    root->intvalue = casecount;
    root->right = casetree;

    rbrace();

    return (root);
}

static struct ASTnode *single_statement(void) {
    struct ASTnode *stmt;

    switch (Token.token) {
        case T_LBRACE:
            lbrace();
            stmt = compound_statement(0);
            rbrace();
            return (stmt);

        case T_IDENT:
            if (findtypedef(Text) == NULL) {
                stmt = binexpr(0);
                semi();
                return stmt;
            }
        case T_CHAR:
        case T_INT:
        case T_LONG:
        case T_STRUCT:
        case T_UNION:
        case T_ENUM:
        case T_TYPEDEF:
            declaration_list(C_LOCAL, T_SEMI, T_EOF, &stmt);
            semi();
            return (stmt);
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
        case T_SWITCH:
            return (switch_statement());
        default:
            stmt = binexpr(0);
            semi();
            return (stmt);
    }
}

struct ASTnode *compound_statement(int inswitch) {
    struct ASTnode *left = NULL;
    struct ASTnode *tree;

    while (1) {
        tree = single_statement();

        if (tree != NULL) {
            if (left == NULL)
                left = tree;
            else
                left = mkastnode(A_GLUE, P_NONE, left, NULL, tree, NULL, 0);
        }

        if (Token.token == T_RBRACE)
            return (left);
        if (inswitch && (Token.token == T_CASE || Token.token == T_DEFAULT))
            return (left);
    }
}