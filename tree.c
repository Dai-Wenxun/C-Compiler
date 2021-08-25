#include "defs.h"
#include "data.h"
#include "decl.h"


struct ASTnode *mkastnode(int op, int type,
                struct ASTnode *left, struct ASTnode *mid, struct ASTnode *right,
                struct symtable *sym, int intvalue) {
    struct ASTnode *node;

    node = (struct ASTnode *)malloc(sizeof(struct ASTnode));
    if (node == NULL)
        fatal("unable to malloc a ASTnode in mkastnode()");

    node->op = op;
    node->type = type;
    node->rvalue = 0;
    node->left = left;
    node->mid = mid;
    node->right = right;
    node->sym = sym;
    node->intvalue = intvalue;

    return (node);
}

struct ASTnode *mkastleaf(int op, int type,
                struct symtable *sym, int intvalue) {
    return (mkastnode(op, type, NULL, NULL, NULL, sym, intvalue));
}

struct ASTnode *mkastunary(int op, int type, struct ASTnode *left,
                struct symtable *sym, int intvalue) {
    return (mkastnode(op, type, left, NULL, NULL, sym, intvalue));
}