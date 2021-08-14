#include "defs.h"
#include "data.h"
#include "decl.h"

int genlabel(void) {
    static int id = 1;
    return (id++);
}

static int genIF(struct ASTnode *n) {
    int Lfalse, Lend;

    Lfalse = genlabel();
    if (n->right)
        Lend = genlabel();

    genAST(n->left, Lfalse, n->op);
    genfreeregs();

    genAST(n->mid, NOLABEL, n->op);
    genfreeregs();

    if (n->right)
        cgjump(Lend);

    cglabel(Lfalse);

    if (n->right) {
        genAST(n->right, NOLABEL, n->op);
        genfreeregs();
        cglabel(Lend);
    }

    return (NOREG);
}

static int genWHILE(struct ASTnode *n) {
    int Lstart, Lend;

    Lstart = genlabel();
    Lend = genlabel();

    cglabel(Lstart);

    genAST(n->left, Lend, n->op);
    genfreeregs();

    genAST(n->right, NOLABEL, n->op);
    genfreeregs();

    cgjump(Lstart);
    cglabel(Lend);

    return (NOREG);
}

static int gen_funccall(struct ASTnode *n) {
    struct ASTnode *gluetree = n->left;
    int reg;
    int numargs = 0;

    while (gluetree) {
        reg = genAST(gluetree->right, NOLABEL, gluetree->op);

        cgcopyarg(reg, gluetree->size);

        if (numargs == 0)
            numargs = gluetree->size;
        genfreeregs();
        gluetree = gluetree->left;
    }

    return (cgcall(n->sym, numargs));
}

int genAST(struct ASTnode *n, int label, int parentASTop) {
    int leftreg, rightreg;

    switch (n->op) {
        case A_IF:
            return (genIF(n));
        case A_WHILE:
            return (genWHILE(n));
        case A_FUNCCALL:
            return (gen_funccall(n));
        case A_GLUE:
            genAST(n->left, NOLABEL, n->op);
            genfreeregs();
            genAST(n->right, NOLABEL, n->op);
            genfreeregs();
            return (NOREG);
        case A_FUNCTION:
            cgfuncpreamble(n->sym);
            genAST(n->left, NOLABEL, n->op);
            cgfuncpostamble(n->sym);
            return (NOREG);
    }

    if (n->left)
        leftreg = genAST(n->left, NOLABEL, n->op);
    if (n->right)
        rightreg = genAST(n->right, NOLABEL, n->op);

    switch (n->op) {
        case A_ASSIGN:
            switch (n->right->op) {
                case A_IDENT:
                    if (n->right->sym->class == C_GLOBAL)
                        return (cgstorglob(leftreg, n->right->sym));
                    else
                        return (cgstorlocal(leftreg, n->right->sym));
                case A_DEREF:
                    return (cgstorderef(leftreg, rightreg, n->right->type));
                default:
                    fatald("Can't A_ASSIGN in genAST(), op", n->op);
            }
            break;
        case A_OR:
            return (cgor(leftreg, rightreg));
        case A_XOR:
            return (cgxor(leftreg, rightreg));
        case A_AND:
            return (cgand(leftreg, rightreg));
        case A_EQ:
        case A_NE:
        case A_LT:
        case A_GT:
        case A_LE:
        case A_GE:
            if (parentASTop == A_IF || parentASTop == A_WHILE)
                return (cgcompare_and_jump(n->op, leftreg, rightreg, label));
            else
                return (cgcompare_and_set(n->op, leftreg, rightreg));
        case A_LSHIFT:
            return (cgshl(leftreg, rightreg));
        case A_RSHIFT:
            return (cgshr(leftreg, rightreg));
        case A_ADD:
            return (cgadd(leftreg, rightreg));
        case A_SUBTRACT:
            return (cgsub(leftreg, rightreg));
        case A_MULTIPLY:
            return (cgmul(leftreg, rightreg));
        case A_DIVIDE:
            return (cgdiv(leftreg, rightreg));
        case A_INTLIT:
            return (cgloadint(n->intvalue, n->type));
        case A_STRLIT:
            return (cgloadglobstr(n->intvalue));
        case A_IDENT:
            if (n->rvalue || parentASTop == A_DEREF) {
                if (n->sym->class == C_GLOBAL)
                    return (cgloadglob(n->sym, n->op));
                else
                    return (cgloadlocal(n->sym, n->op));
            } else
                return (NOREG);
        case A_PREINC:
        case A_PREDEC:
        case A_POSTINC:
        case A_POSTDEC:
            if (n->sym->class == C_GLOBAL)
                return (cgloadglob(n->sym, n->op));
            else
                return (cgloadlocal(n->sym, n->op));
        case A_NEGATE:
            return (cgnegate(leftreg));
        case A_INVERT:
            return (cginvert(leftreg));
        case A_LOGNOT:
            return (cglognot(leftreg));
        case A_TOBOOL:
            return (cgboolean(leftreg, parentASTop, label));
        case A_RETURN:
            cgreturn(leftreg, Functionid);
            return (NOREG);
        case A_ADDR:
            return (cgaddress(n->sym));
        case A_DEREF:
            if (n->rvalue || parentASTop == A_DEREF)
                return (cgderef(leftreg, n->type));
            else
                return (leftreg);
        case A_WIDEN:
            return (cgwiden(leftreg, n->left->type, n->type));
        case A_SCALE:
            switch (n->size) {
                case 2: return (cgshlconst(leftreg, 1));
                case 4: return (cgshlconst(leftreg, 2));
                case 8: return (cgshlconst(leftreg, 3));
                default:
                    rightreg = cgloadint(n->size, P_INT);
                    return (cgmul(leftreg, rightreg));
            }
        default:
            fatald("Unknown AST operator", n->op);
    }
}

void genpreamble(void) {
    cgpreamble();
}

void genpostamble(void) {
    cgpostamble();
}

void genfreeregs(void) {
    freeall_registers();
}

void genglobsym(struct symtable *node) {
    cgglobsym(node);
}

int genglobstr(char *strvalue) {
    int l = genlabel();
    cgglobstr(l, strvalue);
    return (l);
}

int genprimsize(int type) {
    return (cgprimsize(type));
}
