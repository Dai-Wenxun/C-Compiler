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

static int newlocaloffset(int type, struct symtable *ctype) {
    localOffset += (typesize(type, ctype) > 4) ? typesize(type, ctype) : 4;

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
    fatal("out of registers");
    return (NOREG);
}

static void free_register(int r) {
    if (freereg[r] != 0) {
        fatald("can't free register", r);
    }
    freereg[r] = 1;
}

void cgpreamble(void) {
    freeall_registers();
    cgtextseg();

    fprintf(Outfile,
           "switch:\n"
           "    pushq   %%rsi\n"
           "    movq    %%rdx, %%rsi\n"   //%%rsi = switch table address
           "    movq    %%rax, %%rbx\n"   //%%rbx = expression value
           "    cld\n"
           "    lodsq\n"               //Load qword at address (R)SI into RAX
           "    movq    %%rax, %%rcx\n"
           "next:\n"
           "    lodsq\n"
           "    movq    %%rax, %%rdx\n"
           "    lodsq\n"
           "    cmpq    %%rdx, %%rbx\n"
           "    jnz     no\n"
           "    popq    %%rsi\n"
           "    jmp *%%rax\n"
           "no:\n"
           "    loop    next\n"
           "    lodsq\n"
           "    popq    %%rsi\n"
           "    jmp *%%rax\n"
           );
}

void cgpostamble(void) {
}

void cgfuncpreamble(struct symtable *sym) {
    char *name = sym->name;
    struct symtable *parm, *locvar;
    int cnt;
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

    for (parm = sym->member, cnt = 1; parm != NULL; parm = parm->next, ++cnt) {
        if (cnt > 6) {
            parm->posn = paramOffset;
            paramOffset += 8;
        } else {
            parm->posn = newlocaloffset(parm->type, parm->ctype);
            cgstorlocal(paramReg--, parm);
        }
    }

    for (locvar = Loclhead; locvar != NULL; locvar = locvar->next) {
        locvar->posn = newlocaloffset(locvar->type, locvar->ctype);
    }

    stackOffset = (localOffset + 15) & ~15;
    if (stackOffset)
        fprintf(Outfile, "\taddq\t$%d, %%rsp\n", -stackOffset);
}

void cgfuncpostamble(struct symtable *sym) {
    cglabel(sym->endlabel);
    if (stackOffset)
        fprintf(Outfile, "\taddq\t$%d, %%rsp\n", stackOffset);
    fputs("\tpopq\t%rbp\n\tret\n", Outfile);
}

int cgstorlocal(int r, struct symtable *sym) {
    if (cgprimsize(sym->type) == 8) {
        fprintf(Outfile, "\tmovq\t%s, %d(%%rbp)\n",
                reglist[r], sym->posn);
    } else {
        switch (sym->type) {
            case P_CHAR:
                fprintf(Outfile, "\tmovb\t%s, %d(%%rbp)\n",
                        breglist[r], sym->posn);
                break;
            case P_INT:
                fprintf(Outfile, "\tmovl\t%s, %d(%%rbp)\n",
                        dreglist[r], sym->posn);
                break;
            default:
                fatald("bad type in cgstorlocal()", sym->type);
        }
    }
    return (r);
}

int cgstorglob(int r, struct symtable *sym) {
    if (cgprimsize(sym->type) == 8) {
        fprintf(Outfile, "\tmovq\t%s, %s(%%rip)\n",
                reglist[r], sym->name);
    } else {
        switch (sym->type) {
            case P_CHAR:
                fprintf(Outfile, "\tmovb\t%s, %s(%%rip)\n",
                        breglist[r], sym->name);
                break;
            case P_INT:
                fprintf(Outfile, "\tmovl\t%s, %s(%%rip)\n",
                        dreglist[r], sym->name);
                break;
            default:
                fatald("bad type in cgstorglob()", sym->type);
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
            fatald("bad type in cgstorderef()", type);
    }
    return (r1);
}

int cgor(int r1, int r2) {
    fprintf(Outfile, "\torq\t\t%s, %s\n", reglist[r1], reglist[r2]);
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
        fatal("bad ASTop in cgcompare_and_jump()");

    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
    fprintf(Outfile, "\t%s\tL%d\n", invcmplist[ASTop - A_EQ], label);
    freeall_registers();
    return (NOREG);
}

static char *cmplist[] =
        {"sete", "setne", "setl", "setg", "setle", "setge"};

int cgcompare_and_set(int ASTop, int r1, int r2) {
    if (ASTop < A_EQ || ASTop > A_GE)
        fatal("bad ASTop in cgcompare_and_set()");

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

int cgloadglobstr(int label) {
    int r = alloc_register();

    fprintf(Outfile, "\tleaq\tL%d(%%rip), %s\n", label, reglist[r]);
    return (r);
}

int cgloadglob(struct symtable* sym, int op) {
    int r = alloc_register();

    if (cgprimsize(sym->type) == 8) {
        if (op == A_PREINC)
            fprintf(Outfile, "\tincq\t%s(%%rip)\n", sym->name);
        if (op == A_PREDEC)
            fprintf(Outfile, "\tdecq\t%s(%%rip)\n", sym->name);

        fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", sym->name, reglist[r]);

        if (op == A_POSTINC)
            fprintf(Outfile, "\tincq\t%s(%%rip)\n", sym->name);
        if (op == A_POSTDEC)
            fprintf(Outfile, "\tdecq\t%s(%%rip)\n", sym->name);
    } else {
        switch (sym->type) {
            case P_CHAR:
                if (op == A_PREINC)
                    fprintf(Outfile, "\tincb\t%s(%%rip)\n", sym->name);
                if (op == A_PREDEC)
                    fprintf(Outfile, "\tdecb\t%s(%%rip)\n", sym->name);

                fprintf(Outfile, "\tmovsbq\t%s(%%rip), %s\n", sym->name, reglist[r]);

                if (op == A_POSTINC)
                    fprintf(Outfile, "\tincb\t%s(%%rip)\n", sym->name);
                if (op == A_POSTDEC)
                    fprintf(Outfile, "\tdecb\t%s(%%rip)\n", sym->name);
                break;

            case P_INT:
                if (op == A_PREINC)
                    fprintf(Outfile, "\tincl\t%s(%%rip)\n", sym->name);
                if (op == A_PREDEC)
                    fprintf(Outfile, "\tdecl\t%s(%%rip)\n", sym->name);

                fprintf(Outfile, "\tmovslq\t%s(%%rip), %s\n", sym->name, reglist[r]);

                if (op == A_POSTINC)
                    fprintf(Outfile, "\tincl\t%s(%%rip)\n", sym->name);
                if (op == A_POSTDEC)
                    fprintf(Outfile, "\tdecl\t%s(%%rip)\n", sym->name);
                break;
            default:
                fatald("bad type in cgloadglob()", sym->type);
        }
    }
    return (r);
}

int cgloadlocal(struct symtable* sym, int op) {
    int r = alloc_register();

    if (cgprimsize(sym->type) == 8) {
        if (op == A_PREINC)
            fprintf(Outfile, "\tincq\t%d(%%rbp)\n", sym->posn);
        if (op == A_PREDEC)
            fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", sym->posn);

        fprintf(Outfile, "\tmovq\t%d(%%rbp), %s\n", sym->posn, reglist[r]);

        if (op == A_POSTINC)
            fprintf(Outfile, "\tincq\t%d(%%rbp)\n", sym->posn);
        if (op == A_POSTDEC)
            fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", sym->posn);
    } else {
        switch (sym->type) {
            case P_CHAR:
                if (op == A_PREINC)
                    fprintf(Outfile, "\tincb\t%d(%%rbp)\n", sym->posn);
                if (op == A_PREDEC)
                    fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", sym->posn);

                fprintf(Outfile, "\tmovsbq\t%d(%%rbp), %s\n", sym->posn, reglist[r]);

                if (op == A_POSTINC)
                    fprintf(Outfile, "\tincb\t%d(%%rbp)\n", sym->posn);
                if (op == A_POSTDEC)
                    fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", sym->posn);
                break;

            case P_INT:
                if (op == A_PREINC)
                    fprintf(Outfile, "\tincl\t%d(%%rbp)\n", sym->posn);
                if (op == A_PREDEC)
                    fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", sym->posn);

                fprintf(Outfile, "\tmovslq\t%d(%%rbp), %s\n", sym->posn, reglist[r]);

                if (op == A_POSTINC)
                    fprintf(Outfile, "\tincl\t%d(%%rbp)\n", sym->posn);
                if (op == A_POSTDEC)
                    fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", sym->posn);
                break;
            default:
                fatald("bad type in cgloadlocal()", sym->type);
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
        fprintf(Outfile, "\tjz\tL%d\n", label);
    else {
        fprintf(Outfile, "\tsetnz\t%s\n", breglist[r]);
        fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]);
    }
    return (r);
}

void cgreturn(int r, struct symtable *sym) {
    switch (sym->type) {
        case P_VOID:
            break;
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
            fatald("bad function type in cgreturn()", sym->type);
    }
    cgjump(sym->endlabel);
}

int cgcall(struct symtable *sym, int numargs) {
    int outr = alloc_register();

    fprintf(Outfile, "\tcall\t%s@PLT\n", sym->name);

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

int cgaddress(struct symtable *sym) {
    int r = alloc_register();
    if (sym->class == C_GLOBAL)
        fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", sym->name, reglist[r]);
    else
        fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", sym->posn, reglist[r]);
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
            fatald("bad type in cgderef()", type);
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

void cgglobsym(struct symtable *sym) {
    int size, type;
    int i, initvalue;

    if (sym == NULL)
        return;

    if (sym->stype == S_FUNCTION)
        return;

    if (sym->stype == S_ARRAY) {
        size = typesize(value_at(sym->type), sym->ctype);
        type = value_at(sym->type);
    }
    else {
        size = sym->size;
        type = sym->type;
    }

    cgdataseg();
    fprintf(Outfile, "\t.globl\t%s\n", sym->name);
    fprintf(Outfile, "%s:\n", sym->name);

    for (i = 0; i < sym->nelems; i++) {
        initvalue = 0;
        if (sym->initlist != NULL)
            initvalue = sym->initlist[i];

        switch (size) {
            case 1:
                fprintf(Outfile, "\t.byte\t%d\n", initvalue);
                break;
            case 4:
                fprintf(Outfile, "\t.long\t%d\n", initvalue);
                break;
            case 8:
                if (sym->initlist != NULL && type == pointer_to(P_CHAR) && initvalue != 0)
                    fprintf(Outfile, "\t.quad\tL%d\n", initvalue);
                else
                    fprintf(Outfile, "\t.quad\t%d\n", initvalue);
                break;
            default:
                for (i = 0; i < size; ++i)
                    fprintf(Outfile, "\t.byte\t0\n");
                break;
        }
    }
}

void cgglobstr(int l, char *strvalue)   {
    char *cptr;
    cgdataseg();
    cglabel(l);
    for (cptr = strvalue; *cptr; cptr++) {
        fprintf(Outfile, "\t.byte\t%d\n", *cptr);
    }
    fprintf(Outfile, "\t.byte\t0\n");
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
            fatald("bad type in cgprimsize()", type);
    }

    return (0);
}

int cglign(int type, int offset, int direction) {
    int alignment;

    switch (type) {
        case P_CHAR:
            return (offset);
        case P_INT:
        case P_LONG:
            break;
        default:
            fatald("bad type in calc_aligned_offset()", type);
    }

    alignment = 4;

    offset = (offset + direction * (alignment-1)) & ~(alignment-1);
    return (offset);
}

void cglabel(int l) {
    fprintf(Outfile, "L%d:\n", l);
}

void cgjump(int l) {
    fprintf(Outfile, "\tjmp\tL%d\n", l);
}

void cgswitch(int reg, int casecount, int toplabel,
              int *caselabel, int *caseval, int defaultlabel) {
    int i, label;

    label = genlabel();
    cglabel(label);

    if (casecount == 0) {
        caseval[0] = 0;
        caselabel[0] = defaultlabel;
        casecount = 1;
    }

    fprintf(Outfile, "\t.quad\t%d\n", casecount);
    for (i = 0; i < casecount; ++i)
        fprintf(Outfile, "\t.quad\t%d, L%d\n", caseval[i], caselabel[i]);
    fprintf(Outfile, "\t.quad\tL%d\n", defaultlabel);

    cglabel(toplabel);
    fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[reg]);
    fprintf(Outfile, "\tleaq\tL%d(%%rip), %%rdx\n", label);
    fprintf(Outfile, "\tjmp\tswitch\n");
}