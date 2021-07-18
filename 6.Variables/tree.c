#include "defs.h"
#include "data.h"
#include "decl.h"


struct ASTnode *mkastnode(int op, struct ASTnode *left,
        struct ASTnode *right, int intvalue) {
    struct ASTnode *n;

    n = (struct ASTnode *)malloc(sizeof(struct ASTnode));
    if (n == NULL)
        fatal("Unable to malloc in mkastnode()");

    n->op = op;
    n->left = left;
    n->right = right;
    n->v.intvalue = intvalue;

    return n;
}

struct ASTnode *mkastleaf(int op, int intvalue) {
    return mkastnode(op, NULL, NULL, intvalue);
}

struct ASTnode *mkastunary(int op, struct ASTnode *left, int intvalue) {

}