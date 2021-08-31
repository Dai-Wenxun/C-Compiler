#include "defs.h"
#include "data.h"
#include "decl.h"


static struct ASTnode *fold2(struct ASTnode *n) {
    int val, leftval, rightval;

    leftval = n->left->intvalue;
    rightval = n->right->intvalue;

    switch (n->op) {
        case A_ADD:
            val = leftval + rightval;
            break;
        case A_SUBTRACT:
            val = leftval - rightval;
            break;
        case A_MULTIPLY:
            val = leftval * rightval;
            break;
        case A_DIVIDE:
            if (rightval == 0)
                return (n);
            val = leftval / rightval;
            break;
        default:
            return (n);
    }

    return (mkastleaf(A_INTLIT, n->type, NULL, val));
}

static struct ASTnode *fold1(struct ASTnode *n) {
    int val;

    val = n->left->intvalue;
    switch (n->op) {
        case A_WIDEN:
            break;
        case A_INVERT:
            val = ~val;
            break;
        case A_LOGNOT:
            val = !val;
            break;
        default:
            return (n);
    }

    return (mkastleaf(A_INTLIT, n->type, NULL, val));
}

static struct ASTnode *fold(struct ASTnode *n) {

    if (n == NULL)
        return (NULL);

    n->left = fold(n->left);
    n->right = fold(n->right);

    if (n->left && n->left->op == A_INTLIT) {
        if (n->right && n->right->op == A_INTLIT)
            n = fold2(n);
        else
            n = fold1(n);
    }

    return (n);
}

struct ASTnode *optimise(struct ASTnode *n) {
    n = fold(n);
    return (n);
}