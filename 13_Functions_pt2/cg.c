#include "defs.h"
#include "data.h"
#include "decl.h"

static int freereg[4];
static char *reglist[4] = {"%r8", "%r9", "%r10", "%r11"};
static char *dreglist[4] = {"%r8d", "%r9d", "%r10d", "%r11d"};
static char *breglist[4] = {"%r8b", "%r9b", "%r10b", "%r11b"};

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
}

static void free_register(int reg) {
    if (freereg[reg] != 0) {
        fatald("Error trying to free register", reg);
    }
    freereg[reg] = 1;
}

void cgpreamble() {
    freeall_registers();
    fputs("\t.text\n", Outfile);
}

void cgfuncpreamble(int id) {
    char *name = Gsym[id].name;
    fprintf(Outfile,
            "\t.text\n"
            "\t.globl\t%s\n"
            "\t.type\t%s, @function\n"
            "%s:\n"
            "\tpushq\t%%rbp\n"
            "\tmovq\t%%rsp, %%rbp\n",
            name, name, name);
}

void cgfuncpostamble(int id) {
    cglabel(Gsym[id].endlabel);
    fputs(
        "\tpopq\t%rbp\n"
        "\tret\n",
        Outfile);
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

int cgloadglob(int id) {
    int r = alloc_register();

    switch (Gsym[id].type) {
        case P_CHAR:
            fprintf(Outfile, "\tmovzbq\t%s(%%rip), %s\n", Gsym[id].name, reglist[r]);
            break;
        case P_INT:
            fprintf(Outfile, "\tmovl\t%s(%%rip), %s\n", Gsym[id].name, dreglist[r]);
            break;
        case P_LONG:
            fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", Gsym[id].name, reglist[r]);
            break;
    }
    return (r);
}

int cgstorglob(int r, int id) {
    switch (Gsym[id].type) {
        case P_CHAR:
            fprintf(Outfile, "\tmovb\t%s, %s(%%rip)\n",
                    breglist[r], Gsym[id].name);
            break;
        case P_INT:
            fprintf(Outfile, "\tmovl\t%s, %s(%%rip)\n",
                    dreglist[r], Gsym[id].name);
            break;
        case P_LONG:
            fprintf(Outfile, "\tmovq\t%s, %s(%%rip)\n",
                    reglist[r], Gsym[id].name);
            break;
        default:
            fatald("Bad type in cgstorglob()", Gsym[id].type);
    }
    return (r);
}

void cgprintint(int r) {
    fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
    fprintf(Outfile, "\tcall\tprintint\n");
    free_register(r);
}

int cgwiden(int r, int oldtype, int newtype) {
    return (r);
}

void cgreturn(int r, int id) {
    switch (Gsym[id].type) {
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
            fatald("Bad function type in cgreturn()", Gsym[id].type);
    }
    cgjump(Gsym[id].endlabel);
}

int cgcall(int r, int id) {
    int outr = alloc_register();
    fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
    fprintf(Outfile, "\tcall\t%s\n", Gsym[id].name);
    fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[outr]);
    free_register(r);
    return (outr);
}

static int psize[] = {0, 0 , 1, 4, 8};
int cgprimsize(int type) {
    if (type < P_NONE || type > P_LONG)
        fatal("Bad type in cgprimsize()");
    return (psize[type]);
}

void cgglobsym(int id) {
    int typesize;

    typesize = cgprimsize(Gsym[id].type);
    fprintf(Outfile, "\t.comm\t%s,%d,%d\n",
            Gsym[id].name, typesize, typesize);
}

void cglabel(int l) {
    fprintf(Outfile, "L%d:\n", l);
}

void cgjump(int l) {
    fprintf(Outfile, "\tjmp\tL%d\n", l);
}





