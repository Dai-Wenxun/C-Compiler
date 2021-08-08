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

int genAST(struct ASTnode *n, int label, int parentASTop) {
    int leftreg, rightreg;

    switch (n->op) {
        case A_IF:
            return (genIF(n));
        case A_WHILE:
            return (genWHILE(n));
        case A_GLUE:
            genAST(n->left, NOLABEL, n->op);
            genfreeregs();
            genAST(n->right, NOLABEL, n->op);
            genfreeregs();
            return (NOREG);
        case A_FUNCTION:
            cgfuncpreamble(n->v.id);
            genAST(n->left, NOLABEL, n->op);
            cgfuncpostamble(n->v.id);
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
                    if (Symtable[n->right->v.id].class == C_LOCAL) {
                        return (cgstorlocal(leftreg, n->right->v.id));
                    } else {
                        return (cgstorglob(leftreg, n->right->v.id));
                    }
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
            return (cgloadint(n->v.intvalue, n->type));
        case A_STRLIT:
            return (cgloadglobstr(n->v.id));
        case A_IDENT:
            if (n->rvalue || parentASTop == A_DEREF) {
                if (Symtable[n->v.id].class == C_LOCAL)
                    return (cgloadlocal(n->v.id, n->op));
                else
                    return (cgloadglob(n->v.id, n->op));
            } else
                return (NOREG);
        case A_PREINC:
        case A_PREDEC:
            return (cgloadglob(n->left->v.id, n->op));
        case A_POSTINC:
        case A_POSTDEC:
            return (cgloadglob(n->v.id, n->op));
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
        case A_FUNCCALL:
            return (cgcall(leftreg, n->v.id));
        case A_ADDR:
            return (cgaddress(n->v.id));
        case A_DEREF:
            if (n->rvalue)
                return (cgderef(leftreg, n->left->type));
            else
                return (leftreg);
        case A_WIDEN:
            return (cgwiden(leftreg, n->left->type, n->type));
        case A_SCALE:
            switch (n->v.size) {
                case 2: return (cgshlconst(leftreg, 1));
                case 4: return (cgshlconst(leftreg, 2));
                case 8: return (cgshlconst(leftreg, 3));
                default:
                    rightreg = cgloadint(n->v.size, P_INT);
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

void genglobsym(int id) {
    cgglobsym(id);
}

int genglobstr(char *strvalue) {
    int l = genlabel();
    cgglobstr(l, strvalue);
    return (l);
}

int genprimsize(int type) {
    return (cgprimsize(type));
}

void genresetlocals(void) {
    cgresetlocals();
}

int gengetlocaloffset(int type, int isparam) {
    return (cggetlocaloffset(type, isparam));
}