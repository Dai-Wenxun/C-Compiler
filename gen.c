#include "defs.h"
#include "data.h"
#include "decl.h"

static int labelid = 1;
int genlabel(void) {
    return (labelid++);
}

static int genIF(struct ASTnode *n, int looptoplabel, int loopendlabel) {
    int Lfalse, Lend;

    Lfalse = genlabel();
    if (n->right)
        Lend = genlabel();

    genAST(n->left, Lfalse, NOLABEL, NOLABEL, n->op);
    genfreeregs(-1);

    genAST(n->mid, NOLABEL, looptoplabel, loopendlabel, n->op);
    genfreeregs(-1);

    if (n->right)
        cgjump(Lend);

    cglabel(Lfalse);

    if (n->right) {
        genAST(n->right, NOLABEL, looptoplabel, loopendlabel, n->op);
        genfreeregs(-1);
        cglabel(Lend);
    }

    return (NOREG);
}

static int genWHILE(struct ASTnode *n) {
    int Lstart, Lend;

    Lstart = genlabel();
    Lend = genlabel();

    cglabel(Lstart);

    genAST(n->left, Lend, NOLABEL, NOLABEL, n->op);
    genfreeregs(-1);

    genAST(n->right, NOLABEL, Lstart, Lend, n->op);
    genfreeregs(-1);

    cgjump(Lstart);
    cglabel(Lend);

    return (NOREG);
}

static int genFunccall(struct ASTnode *n) {
    struct ASTnode *gluetree = n->left;
    int reg;
    int numargs = 0;

    while (gluetree) {
        reg = genAST(gluetree->right, NOLABEL, NOLABEL, NOLABEL, gluetree->op);

        cgcopyarg(reg, gluetree->size);

        if (numargs == 0)
            numargs = gluetree->size;
        genfreeregs(-1);
        gluetree = gluetree->left;
    }

    return (cgcall(n->sym, numargs));
}

static int genSWITCH(struct ASTnode *n) {
    int *caseval, *caselabel;
    int Ljumptop, Lend;
    int i, reg, defaultlabel;
    struct ASTnode *c;

    caseval = (int *)malloc((n->intvalue + 1) * sizeof(int));
    caselabel = (int *)malloc((n->intvalue + 1) * sizeof(int));

    Ljumptop = genlabel();
    Lend = genlabel();
    defaultlabel = Lend;

    reg = genAST(n->left, NOLABEL, NOLABEL, NOLABEL, 0);
    cgjump(Ljumptop);
    genfreeregs(-1);

    for (i = 0, c = n->right; c != NULL; i++, c = c->right) {
        caselabel[i] = genlabel();
        caseval[i] = c->intvalue;
        cglabel(caselabel[i]);
        if (c->op == A_DEFAULT)
            defaultlabel = caselabel[i];

        if (c->left != NULL) {
            genAST(c->left, NOLABEL, NOLABEL, Lend, 0);
            genfreeregs(-1);
        }
    }

    cgjump(Lend);
    cgswitch(reg, n->intvalue, Ljumptop, caselabel, caseval, defaultlabel);
    cglabel(Lend);
    return (NOREG);
}

static int genTernary(struct ASTnode *n) {
    int Lfasle, Lend;
    int expreg, reg;

    Lfasle = genlabel();
    Lend = genlabel();


    genAST(n->left, Lfasle, NOLABEL, NOLABEL, n->op);
    genfreeregs(-1);

    reg = alloc_register();

    expreg = genAST(n->mid, NOLABEL, NOLABEL, NOLABEL, 0);
    cgmove(expreg, reg);
    genfreeregs(reg);

    cgjump(Lend);
    cglabel(Lfasle);
    expreg = genAST(n->right, NOLABEL, NOLABEL, NOLABEL, 0);
    cgmove(expreg, reg);
    genfreeregs(reg);
    cglabel(Lend);

    return (reg);
}

int genAST(struct ASTnode *n, int iflabel, int looptoplabel,
            int loopendlabel, int parentASTop) {
    int leftreg, rightreg, asignreg;

    switch (n->op) {
        case A_IF:
            return (genIF(n, looptoplabel, loopendlabel));
        case A_WHILE:
            return (genWHILE(n));
        case A_SWITCH:
            return (genSWITCH(n));
        case A_FUNCCALL:
            return (genFunccall(n));
        case A_TERNARY:
            return (genTernary(n));
        case A_GLUE:
            if (n->left != NULL) {
                genAST(n->left, NOLABEL, looptoplabel, loopendlabel,n->op);
                genfreeregs(-1);
            }
            if (n->right != NULL) {
                genAST(n->right, NOLABEL, looptoplabel, loopendlabel, n->op);
                genfreeregs(-1);
            }
            return (NOREG);
        case A_FUNCTION:
            cgfuncpreamble(n->sym);
            if (n->left != NULL) genAST(n->left, NOLABEL, NOLABEL, NOLABEL, n->op);
            cgfuncpostamble(n->sym);
            return (NOREG);
    }

    if (n->left)
        leftreg = genAST(n->left, NOLABEL, NOLABEL, NOLABEL, n->op);
    if (n->right)
        rightreg = genAST(n->right, NOLABEL, NOLABEL, NOLABEL, n->op);

    switch (n->op) {
        case A_ASSIGN:
        case A_ASPLUS:
        case A_ASMINUS:
        case A_ASSTAR:
        case A_ASSLASH:
            if (n->right->op == A_DEREF)
                 asignreg = cgderef(rightreg, n->right->type);
            switch (n->op) {
                case A_ASPLUS:
                    if (n->right->op == A_DEREF)
                        leftreg = cgadd(asignreg, leftreg);
                    else
                        leftreg = cgadd(rightreg, leftreg);
                    break;
                case A_ASMINUS:
                    if (n->right->op == A_DEREF)
                        leftreg = cgsub(asignreg, leftreg);
                    else
                        leftreg = cgsub(rightreg, leftreg);
                    break;
                case A_ASSTAR:
                    if (n->right->op == A_DEREF)
                        leftreg = cgmul(asignreg, leftreg);
                    else
                        leftreg = cgmul(rightreg, leftreg);
                    break;
                case A_ASSLASH:
                    if (n->right->op == A_DEREF)
                        leftreg = cgdiv(asignreg, leftreg);
                    else
                        leftreg = cgdiv(rightreg, leftreg);
                    break;
            }
            switch (n->right->op) {
                case A_IDENT:
                    if (n->right->sym->class == C_GLOBAL || n->right->sym->class == C_STATIC)
                        return (cgstorglob(leftreg, n->right->sym));
                    else
                        return (cgstorlocal(leftreg, n->right->sym));
                case A_DEREF:
                    return (cgstorderef(leftreg, rightreg, n->right->type));
                default:
                    fatald("can't assign in genAST(), op", n->op);
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
            if (parentASTop == A_IF || parentASTop == A_WHILE ||
                parentASTop == A_TERNARY)
                return (cgcompare_and_jump(n->op, leftreg, rightreg, iflabel));
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
            if (n->rvalue || parentASTop == A_DEREF
                || (parentASTop >= A_ASPLUS && parentASTop <= A_ASSLASH)) {
                if (n->sym->class == C_GLOBAL || n->sym->class == C_STATIC)
                    return (cgloadglob(n->sym, n->op));
                else
                    return (cgloadlocal(n->sym, n->op));
            } else
                return (NOREG);
        case A_PREINC:
        case A_PREDEC:
        case A_POSTINC:
        case A_POSTDEC:
            if (n->sym->class == C_GLOBAL || n->sym->class == C_STATIC)
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
            return (cgboolean(leftreg, parentASTop, iflabel));
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
        case A_CAST:
            return (leftreg);
        case A_BREAK:
            cgjump(loopendlabel);
            return (NOREG);
        case A_CONTINUE:
            cgjump(looptoplabel);
            return (NOREG);
        default:
            fatald("unknown AST operator", n->op);
    }
}

void genpreamble(void) {
    cgpreamble();
}

void genpostamble(void) {
    cgpostamble();
}

void genfreeregs(int keepreg) {
    freeall_registers(keepreg);
}

void genglobsym(struct symtable *sym) {
    cgglobsym(sym);
}

int genglobstr(char *strvalue) {
    int l = genlabel();
    cgglobstr(l, strvalue);
    return (l);
}

int genprimsize(int type) {
    return (cgprimsize(type));
}

int genalign(int type, int offset, int direction) {
    return (cglign(type, offset, direction));
}