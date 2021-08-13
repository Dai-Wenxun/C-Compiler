#include "defs.h"
#include "data.h"
#include "decl.h"

enum {no_seg, text_seg, data_seg} currSeg = no_seg;

void cgtextseg(void) {
    if (currSeg != text_seg) {
        fputs("\t.text\n", Outfile);
        currSeg = text_seg;
    }
}

void cgdataseg(void) {
    if (currSeg != data_seg) {
        fputs("\t.data\n", Outfile);
        currSeg = data_seg;
    }
}

static int localOffset;
static int stackOffset;

static int newlocaloffset(int type) {
    localOffset += (cgprimsize(type) > 4) ? cgprimsize(type) : 4;

    return (-localOffset);
}

#define NUMFREEREGS 4
#define FIRSTPARAMREG 9
static int freereg[NUMFREEREGS];
static char *reglist[] =
    { "%r10", "%r11", "%r12", "%r13", "%r9", "%r8", "%rcx", "%rdx", "%rsi", "%rdi" };
static char *dreglist[] =
    { "%r10d", "%r11d", "%r12d", "%r13d", "%r9d", "%r8d", "%ecx", "%edx", "%esi", "%edi" };
static char *breglist[] =
    { "%r10b", "%r11b", "%r12b", "%r13b", "%r9b", "%r8b", "%cl", "%dl", "%sil", "%dil" };

void freeall_registers(void) {
    freereg[0] = freereg[1] = freereg[2] = freereg[3] = 1;
}

static int alloc_register(void) {
    for (int i = 0; i < 4; i++) {
        if (freereg[i]) {
            freereg[i] = 0;
            return (i);
        }
    }
    fatal("Out of registers");
    return (NOREG);
}

static void free_register(int r) {
    if (freereg[r] != 0) {
        fatald("Error trying to free register", r);
    }
    freereg[r] = 1;
}

void cgpreamble(void) {
    freeall_registers();
}

void cgpostamble(void) {
}

void cgfuncpreamble(int id) {
    char *name = Symtable[id].name;
    int i;
    int paramOffset = 16;
    int paramReg = FIRSTPARAMREG;


    cgtextseg();
    localOffset = 0;

    fprintf(Outfile,
            "\t.globl\t%s\n"
            "\t.type\t%s, @function\n"
            "%s:\n"
            "\tpushq\t%%rbp\n"
            "\tmovq\t%%rsp, %%rbp\n",
            name, name, name);

    for (i = NSYMBOLS - 1; i > Locls; --i) {
        if (Symtable[i].class != C_PARAM)
            break;
        if (i < NSYMBOLS - 6)
            break;
        Symtable[i].posn = newlocaloffset(Symtable[i].type);
        cgstorlocal(paramReg--, i);
    }

    for (; i > Locls; --i) {
        if (Symtable[i].class == C_PARAM) {
            Symtable[i].posn = paramOffset;
            paramOffset += 8;
        } else {
            Symtable[i].posn = newlocaloffset(Symtable[i].type);
        }
    }

    stackOffset = (localOffset + 15) & ~15;
    if (stackOffset)
        fprintf(Outfile, "\taddq\t$%d, %%rsp\n", -stackOffset);
}

void cgfuncpostamble(int id) {
    cglabel(Symtable[id].endlabel);
    if (stackOffset)
        fprintf(Outfile, "\taddq\t$%d, %%rsp\n", stackOffset);
    fputs("\tpopq\t%rbp\n\tret\n", Outfile);
}

int cgstorlocal(int r, int id) {
    if (cgprimsize(Symtable[id].type) == 8) {
        fprintf(Outfile, "\tmovq\t%s, %d(%%rbp)\n",
                reglist[r], Symtable[id].posn);
    } else {
        switch (Symtable[id].type) {
            case P_CHAR:
                fprintf(Outfile, "\tmovb\t%s, %d(%%rbp)\n",
                        breglist[r], Symtable[id].posn);
                break;
            case P_INT:
                fprintf(Outfile, "\tmovl\t%s, %d(%%rbp)\n",
                        dreglist[r], Symtable[id].posn);
                break;
            default:
                fatald("Bad type in cgstorlocal()", Symtable[id].type);
        }
    }
    return (r);
}

int cgstorglob(int r, int id) {
    if (cgprimsize(Symtable[id].type) == 8) {
        fprintf(Outfile, "\tmovq\t%s, %s(%%rip)\n",
                reglist[r], Symtable[id].name);
    } else {
        switch (Symtable[id].type) {
            case P_CHAR:
                fprintf(Outfile, "\tmovb\t%s, %s(%%rip)\n",
                        breglist[r], Symtable[id].name);
                break;
            case P_INT:
                fprintf(Outfile, "\tmovl\t%s, %s(%%rip)\n",
                        dreglist[r], Symtable[id].name);
                break;
            default:
                fatald("Bad type in cgstorglob()", Symtable[id].type);
        }
    }
    return (r);
}

int cgstorderef(int r1, int r2, int type) {

    int size = genprimsize(type);

    switch (size) {
        case 1:
            fprintf(Outfile, "\tmovb\t%s, (%s)\n", breglist[r1], reglist[r2]);
            break;
        case 4:
            fprintf(Outfile, "\tmovl\t%s, (%s)\n", dreglist[r1], reglist[r2]);
            break;
        case 8:
            fprintf(Outfile, "\tmovq\t%s, (%s)\n", reglist[r1], reglist[r2]);
            break;
        default:
            fatald("Bad type in cgstorderef()", type);
    }
    return (r1);
}

int cgor(int r1, int r2) {
    fprintf(Outfile, "\torq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);
    return (r2);
}

int cgxor(int r1, int r2) {
    fprintf(Outfile, "\txorq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);
    return (r2);
}

int cgand(int r1, int r2) {
    fprintf(Outfile, "\tandq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);
    return (r2);
}

static char *invcmplist[] =
        {"jne", "je", "jge", "jle", "jg", "jl"};

int cgcompare_and_jump(int ASTop, int r1, int r2, int label) {
    if (ASTop < A_EQ || ASTop > A_GE)
        fatal("Bad ASTop in cgcompare_and_jump()");

    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
    fprintf(Outfile, "\t%s\tL%d\n", invcmplist[ASTop - A_EQ], label);
    freeall_registers();
    return (NOREG);
}

static char *cmplist[] =
        {"sete", "setne", "setl", "setg", "setle", "setge"};

int cgcompare_and_set(int ASTop, int r1, int r2) {
    if (ASTop < A_EQ || ASTop > A_GE)
        fatal("Bad ASTop in cgcompare_and_set()");

    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
    fprintf(Outfile, "\t%s\t%s\n", cmplist[ASTop - A_EQ], breglist[r2]);
    fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r2], reglist[r2]);
    free_register(r1);
    return (r2);
}

int cgshl(int r1, int r2) {
    fprintf(Outfile, "\tmovb\t%s, %%cl\n", breglist[r2]);
    fprintf(Outfile, "\tshlq\t%%cl, %s\n", reglist[r1]);
    free_register(r2);
    return (r1);
}

int cgshr(int r1, int r2) {
    fprintf(Outfile, "\tmovb\t%s, %%cl\n", breglist[r2]);
    fprintf(Outfile, "\tshrq\t%%cl, %s\n", reglist[r1]);
    free_register(r2);
    return (r1);
}

int cgadd(int r1, int r2) {
    fprintf(Outfile, "\taddq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);
    return (r2);
}

int cgsub(int r1, int r2) {
    fprintf(Outfile, "\tsubq\t%s, %s\n", reglist[r2], reglist[r1]);
    free_register(r2);
    return (r1);
}

int cgmul(int r1, int r2) {
    fprintf(Outfile, "\timulq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);
    return (r2);
}

int cgdiv(int r1, int r2) {
    fprintf(Outfile, "\tmovq\t%s,%%rax\n", reglist[r1]);
    fprintf(Outfile, "\tcqo\n");
    fprintf(Outfile, "\tidivq\t%s\n", reglist[r2]);
    fprintf(Outfile, "\tmovq\t%%rax,%s\n", reglist[r1]);
    free_register(r2);
    return (r1);
}

int cgloadint(int value, int type) {
    int r = alloc_register();

    fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
    return (r);
}

int cgloadglobstr(int id) {
    int r = alloc_register();

    fprintf(Outfile, "\tleaq\tL%d(%%rip), %s\n", id, reglist[r]);
    return (r);
}

int cgloadlocal(int id, int op) {
    int r = alloc_register();

    if (cgprimsize(Symtable[id].type) == 8) {
        if (op == A_PREINC)
            fprintf(Outfile, "\tincq\t%d(%%rbp)\n", Symtable[id].posn);
        if (op == A_PREDEC)
            fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", Symtable[id].posn);

        fprintf(Outfile, "\tmovq\t%d(%%rbp), %s\n", Symtable[id].posn, reglist[r]);

        if (op == A_POSTINC)
            fprintf(Outfile, "\tincq\t%d(%%rbp)\n", Symtable[id].posn);
        if (op == A_POSTDEC)
            fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", Symtable[id].posn);
    } else {
        switch (Symtable[id].type) {
            case P_CHAR:
                if (op == A_PREINC)
                    fprintf(Outfile, "\tincb\t%d(%%rbp)\n", Symtable[id].posn);
                if (op == A_PREDEC)
                    fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", Symtable[id].posn);

                fprintf(Outfile, "\tmovsbq\t%d(%%rbp), %s\n", Symtable[id].posn, reglist[r]);

                if (op == A_POSTINC)
                    fprintf(Outfile, "\tincb\t%d(%%rbp)\n", Symtable[id].posn);
                if (op == A_POSTDEC)
                    fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", Symtable[id].posn);
                break;

            case P_INT:
                if (op == A_PREINC)
                    fprintf(Outfile, "\tincl\t%d(%%rbp)\n", Symtable[id].posn);
                if (op == A_PREDEC)
                    fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", Symtable[id].posn);

                fprintf(Outfile, "\tmovslq\t%d(%%rbp), %s\n", Symtable[id].posn, reglist[r]);

                if (op == A_POSTINC)
                    fprintf(Outfile, "\tincl\t%d(%%rbp)\n", Symtable[id].posn);
                if (op == A_POSTDEC)
                    fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", Symtable[id].posn);
                break;
            default:
                fatald("Bad type in cgloadlocal()", Symtable[id].type);
        }
    }
    return (r);
}

int cgloadglob(int id, int op) {
    int r = alloc_register();

    if (cgprimsize(Symtable[id].type) == 8) {
        if (op == A_PREINC)
            fprintf(Outfile, "\tincq\t%s(%%rip)\n", Symtable[id].name);
        if (op == A_PREDEC)
            fprintf(Outfile, "\tdecq\t%s(%%rip)\n", Symtable[id].name);

        fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);

        if (op == A_POSTINC)
            fprintf(Outfile, "\tincq\t%s(%%rip)\n", Symtable[id].name);
        if (op == A_POSTDEC)
            fprintf(Outfile, "\tdecq\t%s(%%rip)\n", Symtable[id].name);
    } else {
        switch (Symtable[id].type) {
            case P_CHAR:
                if (op == A_PREINC)
                    fprintf(Outfile, "\tincb\t%s(%%rip)\n", Symtable[id].name);
                if (op == A_PREDEC)
                    fprintf(Outfile, "\tdecb\t%s(%%rip)\n", Symtable[id].name);

                fprintf(Outfile, "\tmovsbq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);

                if (op == A_POSTINC)
                    fprintf(Outfile, "\tincb\t%s(%%rip)\n", Symtable[id].name);
                if (op == A_POSTDEC)
                    fprintf(Outfile, "\tdecb\t%s(%%rip)\n", Symtable[id].name);
                break;

            case P_INT:
                if (op == A_PREINC)
                    fprintf(Outfile, "\tincl\t%s(%%rip)\n", Symtable[id].name);
                if (op == A_PREDEC)
                    fprintf(Outfile, "\tdecl\t%s(%%rip)\n", Symtable[id].name);

                fprintf(Outfile, "\tmovslq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);

                if (op == A_POSTINC)
                    fprintf(Outfile, "\tincl\t%s(%%rip)\n", Symtable[id].name);
                if (op == A_POSTDEC)
                    fprintf(Outfile, "\tdecl\t%s(%%rip)\n", Symtable[id].name);
                break;
            default:
                fatald("Bad type in cgloadglob()", Symtable[id].type);
        }
    }
    return (r);
}

int cgnegate(int r) {
    fprintf(Outfile, "\tnegq\t%s\n", reglist[r]);
    return (r);
}

int cginvert(int r) {
    fprintf(Outfile, "\tnotq\t%s\n", reglist[r]);
    return (r);
}

int cglognot(int r) {
    fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);
    fprintf(Outfile, "\tsete\t%s\n", breglist[r]);
    fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]);
    return (r);
}

int cgboolean(int r, int op, int label) {
    fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);
    if (op == A_IF || op == A_WHILE)
        fprintf(Outfile, "\tje\tL%d\n", label);
    else {
        fprintf(Outfile, "\tsetnz\t%s\n", breglist[r]);
        fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]);
    }
    return (r);
}

void cgreturn(int r, int id) {
    switch (Symtable[id].type) {
        case P_CHAR:
            fprintf(Outfile, "\tmovzbl\t%s, %%eax\n", breglist[r]);
            break;
        case P_INT:
            fprintf(Outfile, "\tmovl\t%s, %%eax\n", dreglist[r]);
            break;
        case P_LONG:
            fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[r]);
            break;
        default:
            fatald("Bad function type in cgreturn()", Symtable[id].type);
    }
    cgjump(Symtable[id].endlabel);
}

int cgcall(int id, int numargs) {
    int outr = alloc_register();

    fprintf(Outfile, "\tcall\t%s@PLT\n", Symtable[id].name);

    if (numargs > 6)
        fprintf(Outfile, "\taddq\t$%d, %%rsp\n", 8*(numargs-6));

    fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[outr]);
    return (outr);
}

void cgcopyarg(int r, int argposn) {
    if (argposn > 6) {
        fprintf(Outfile, "\tpushq\t%s\n", reglist[r]);
    } else {
        fprintf(Outfile, "\tmovq\t%s, %s\n", reglist[r],
                reglist[FIRSTPARAMREG - argposn + 1]);
    }
}

int cgaddress(int id) {
    int r = alloc_register();
    if (Symtable[id].class == C_GLOBAL)
        fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);
    else
        fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", Symtable[id].posn, reglist[r]);
    return (r);
}

int cgderef(int r, int type) {
    int size = cgprimsize(type);

    switch (size) {
        case 1:
            fprintf(Outfile, "\tmovzbq\t(%s), %s\n", reglist[r], reglist[r]);
            break;
        case 4:
            fprintf(Outfile, "\tmovl\t(%s), %s\n", reglist[r], dreglist[r]);
            break;
        case 8:
            fprintf(Outfile, "\tmovq\t(%s), %s\n", reglist[r], reglist[r]);
            break;
        default:
            fatald("Bad type in cgderef()", type);
    }
    return (r);
}

int cgwiden(int r, int oldtype, int newtype) {
    return (r);
}

int cgshlconst(int r, int val) {
    fprintf(Outfile, "\tsalq\t$%d, %s\n", val, reglist[r]);
    return (r);
}

int cgprimsize(int type) {
    if (ptrtype(type))
        return (8);
    switch (type) {
        case P_CHAR:
            return (1);
        case P_INT:
            return (4);
        case P_LONG:
            return (8);
        default:
            fatald("Bad type in cgprimsize()", type);
    }

    return (0);
}

void cgglobsym(int id) {
    int typesize;

    if (Symtable[id].stype == S_FUNCTION)
        return;
    else if (Symtable[id].stype == S_ARRAY)
        typesize = cgprimsize(value_at(Symtable[id].type));
    else
        typesize = cgprimsize(Symtable[id].type);

    cgdataseg();
    fprintf(Outfile, "\t.globl\t%s\n", Symtable[id].name);
    fprintf(Outfile, "%s:", Symtable[id].name);

    for (int i = 0; i < Symtable[id].size; i++) {
        switch (typesize) {
            case 1:
                fprintf(Outfile, "\t.byte\t0\n");
                break;
            case 4:
                fprintf(Outfile, "\t.long\t0\n");
                break;
            case 8:
                fprintf(Outfile, "\t.quad\t0\n");
                break;
            default:
                fatald("Unknown typesize in cgglobsym()", typesize);
        }
    }
}

void cgglobstr(int l, char *strvalue)   {
    char *cptr;
    cglabel(l);
    for (cptr = strvalue; *cptr; cptr++) {
        fprintf(Outfile, "\t.byte\t%d\n", *cptr);
    }
    fprintf(Outfile, "\t.byte\t0\n");
}

void cglabel(int l) {
    fprintf(Outfile, "L%d:\n", l);
}

void cgjump(int l) {
    fprintf(Outfile, "\tjmp\tL%d\n", l);
}