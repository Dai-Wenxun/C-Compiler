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

    match(T_RETURN, "return");

    if (Functionid->type == P_VOID) {
        if (Token.token == T_SEMI) {
            semi();
            return (mkastunary(A_RETURN, P_NONE, NULL, NULL, 0));
        }
        else
            fatals("return with a value in a void function", Functionid->name);
    }

    lparen();

    tree = binexpr(0);

    tree = modify_type(tree, Functionid->type, 0);
    if (tree == NULL)
        fatal("incompatible type in return statement");

    tree = mkastunary(A_RETURN, P_NONE, tree, NULL, 0);

    rparen();
    semi();
    return (tree);
}

static struct ASTnode *break_statement(void) {
    if (Looplevel == 0 && Switchlevel == 0)
        fatal("no loop or switch to break");
    scan(&Token);
    semi();
    return (mkastleaf(A_BREAK, P_NONE, NULL, 0));
}

static struct ASTnode *continue_statement(void) {
    if (Looplevel == 0)
        fatal("no loop to continue");
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
        fatal("switch expression is not of integer type");

    root = mkastunary(A_SWITCH, P_NONE, left, NULL, 0);

    Switchlevel++;
    while (inloop) {
        switch (Token.token) {
            case T_RBRACE:
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
                        fatal("integer literal expected for case value");
                    casevalue = left->intvalue;

                    for (c = casetree; c != NULL; c = c->right)
                        if (casevalue == c->intvalue)
                            fatal("duplicate case value");
                }

                match(T_COLON, ":");
                Casebreak = CNBREAK;
                left = compound_statement(IN_SWITCH);

                if ((Casebreak == CNBREAK) && (ASTop == A_CASE))
                    fprintf(stderr, "[Warning]: case '%d' with no break\n", casevalue);

                if (casetree == NULL) {
                    casetree = casetail = mkastunary(ASTop, P_NONE, left, NULL, casevalue);
                } else {
                    casetail->right = mkastunary(ASTop, P_NONE, left, NULL, casevalue);
                    casetail = casetail->right;
                }
                break;

            default:
                fatals("unexpected token in switch", Token.tokptr);
        }
    }
    Switchlevel--;

    root->intvalue = casecount;
    root->right = casetree;

    rbrace();
    Switchexist = 1;

    return (root);
}

static struct ASTnode *single_statement(void) {
    struct ASTnode *stmt;

    switch (Token.token) {
        case T_LBRACE:
            lbrace();
            stmt = compound_statement(NO_SWITCH);
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
        if (Token.token == T_RBRACE)
            return (left);
        if ((inswitch == IN_SWITCH) && (Token.token == T_CASE || Token.token == T_DEFAULT))
            return (left);

        tree = single_statement();

        if (tree != NULL) {
            if ((inswitch == IN_SWITCH) && tree->op == A_BREAK)
                Casebreak = CBREAK;

            if (left == NULL)
                left = tree;
            else
                left = mkastnode(A_GLUE, P_NONE, left, NULL, tree, NULL, 0);
        }

    }
}