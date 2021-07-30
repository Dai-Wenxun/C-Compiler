#include "defs.h"
#include "data.h"
#include "decl.h"

static int label(void) {
    static int id = 1;
    return id++;
}

static int genIF(struct ASTnode *n) {
    int Lfalse, Lend;

    Lfalse = label();
    if (n->right)
        Lend = label();

    genAST(n->left, Lfalse, n->op);
    genfreeregs();

    genAST(n->mid, NOREG, n->op);
    genfreeregs();

    if (n->right)
        cgjump(Lend);

    cglabel(Lfalse);

    if (n->right) {
        genAST(n->right, NOREG, n->op);
        genfreeregs();
        cglabel(Lend);
    }

    return NOREG;
}


int genAST(struct ASTnode *n, int reg, int parentASTop) {
    int leftreg, rightreg;

    switch (n->op) {
        case A_IF:
            return genIF(n);
        case A_GLUE:
            genAST(n->left, NOREG, n->op);
            genfreeregs();
            genAST(n->right, NOREG, n->op);
            genfreeregs();
            return NOREG;
    }

    if (n->left)
        leftreg = genAST(n->left, NOREG, n->op);
    if (n->right)
        rightreg = genAST(n->right, leftreg, n->op);

    switch (n->op) {
        case A_EQ:
        case A_NE:
        case A_LT:
        case A_GT:
        case A_LE:
        case A_GE:
            if (parentASTop == A_IF)
                return cgcompare_and_jump(n->op, leftreg, rightreg, reg);
            else
                return cgcompare_and_set(n->op, leftreg, rightreg);

        case A_ADD:
            return cgadd(leftreg, rightreg);
        case A_SUBTRACT:
            return cgsub(leftreg, rightreg);
        case A_MULTIPLY:
            return cgmul(leftreg, rightreg);
        case A_DIVIDE:
            return cgdiv(leftreg, rightreg);
        case A_INTLIT:
            return cgloadint(n->v.intvalue);
        case A_IDENT:
            return cgloadglob(Gsym[n->v.id].name);
        case A_LVIDENT:
            return cgstorglob(reg, Gsym[n->v.id].name);
        case A_ASSIGN:
            return rightreg;
        case A_PRINT:
            genprintint(leftreg);
            genfreeregs();
            return NOREG;
        default:
            fprintf(stderr, "Unknown AST operator %d\n", n->op);
            exit(1);
    }
}

void genpreamble() {
    cgpreamble();
}

void genpostamble() {
    cgpostamble();
}

void genfreeregs() {
    freeall_registers();
}

void genprintint(int reg) {
    cgprintint(reg);
}

void genglobsym(char *s) {
    cgglobsym(s);
}

