#include "defs.h"
#include "data.h"
#include "decl.h"

static char *ASTOP[] = {"+", "-", "*", "/"};

int interpretAST(struct ASTnode *n) {
//    switch (n->op) {
//        case A_ADD:
//            return n->left->intvalue + interpretAST(n->right);
//        case A_SUBTRACT:
//            return n->left->intvalue - interpretAST(n->right);
//        case A_MULTIPLY:
//            return n->left->intvalue * interpretAST(n->right);
//        case A_DIVIDE:
//            return n->left->intvalue / interpretAST(n->right);
//        case A_INTLIT:
//            return n->intvalue;
//    }

    int leftval, rightval;
    if (n->left)
        leftval = interpretAST(n->left);
    if (n->right)
        rightval = interpretAST(n->right);

    if (n->op == A_INTLIT)
        printf("int %d\n", n->intvalue);
    else
        printf("%d %s %d\n", leftval, ASTOP[n->op], rightval);

    switch (n->op) {
        case A_ADD:
            return leftval + rightval;
        case A_SUBTRACT:
            return leftval - rightval;
        case A_MULTIPLY:
            return leftval * rightval;
        case A_DIVIDE:
            return leftval / rightval;
        case A_INTLIT:
            return n->intvalue;
        default:
            fprintf(stderr, "Unknown AST operator %d\n", n->op);
            exit(1);
    }
}