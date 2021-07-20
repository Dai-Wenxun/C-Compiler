#include "defs.h"
#include "data.h"
#include "decl.h"

static int freereg[4];
static char *reglist[4] = {"%r8", "%r9", "%r10", "%r11"};
static char *breglist[4] = {"%r8b", "%r9b", "%r10b", "%r11b"};

void freeall_registers(void) {
    freereg[0] = freereg[1] = freereg[2] = freereg[3] = 1;
}

static int alloc_register(void) {
    for (int i = 0; i < 4; i++) {
        if (freereg[i]) {
            freereg[i] = 0;
            return i;
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
    fputs(
        "\t.text\n"
        ".LC0:\n"
        "\t.string\t\"%d\\n\"\n"
        "printint:\n"
        "\tpushq\t%rbp\n"
        "\tmovq\t%rsp, %rbp\n"
        "\tsubq\t$16, %rsp\n"
        "\tmovl\t%edi, -4(%rbp)\n"
        "\tmovl\t-4(%rbp), %eax\n"
        "\tmovl\t%eax, %esi\n"
        "\tleaq\t.LC0(%rip), %rdi\n"
        "\tmovl\t$0, %eax\n"
        "\tcall\tprintf@PLT\n"
        "\tnop\n"
        "\tleave\n"
        "\tret\n"
        "\n"
        "\t.globl\tmain\n"
        "\t.type\tmain, @function\n"
        "main:\n"
        "\tpushq\t%rbp\n"
        "\tmovq\t%rsp, %rbp\n",
        Outfile);

}

void cgpostamble() {
    fputs(
        "\tmovl\t$0, %eax\n"
        "\tpopq\t%rbp\n"
        "\tret\n",
        Outfile);
}

int cgloadint(int value) {
    int r = alloc_register();

    fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
    return r;
}

int cgloadglob(char *identifier) {
    int r = alloc_register();

    fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", identifier, reglist[r]);
    return r;
}

int cgadd(int r1, int r2) {
    fprintf(Outfile, "\taddq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);
    return r2;
}

int cgsub(int r1, int r2) {
    fprintf(Outfile, "\tsubq\t%s, %s\n", reglist[r2], reglist[r1]);
    free_register(r2);
    return r1;
}

int cgmul(int r1, int r2) {
    fprintf(Outfile, "\timulq\t%s, %s\n", reglist[r1], reglist[r2]);
    free_register(r1);
    return r2;
}

int cgdiv(int r1, int r2) {
    fprintf(Outfile, "\tmovq\t%s,%%rax\n", reglist[r1]);
    fprintf(Outfile, "\tcqo\n");
    fprintf(Outfile, "\tidivq\t%s\n", reglist[r2]);
    fprintf(Outfile, "\tmovq\t%%rax,%s\n", reglist[r1]);
    free_register(r2);
    return r1;
}

void cgprintint(int r) {
    fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
    fprintf(Outfile, "\tcall\tprintint\n");
    free_register(r);
}

int cgstorglob(int r, char *identifier) {
    fprintf(Outfile, "\tmovq\t%s, %s(%%rip)\n", reglist[r], identifier);
    return r;
}

void cgglobsym(char *sym) {
    fprintf(Outfile, "\t.comm\t%s, 8, 8\n", sym);
}

static int cgcompare(int r1, int r2, char *how) {
    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
    fprintf(Outfile, "\t%s\t%s\n", how, breglist[r2]);
    fprintf(Outfile, "\tandq\t$255, %s\n", reglist[r2]);
    free_register(r1);
    return r2;
}

int cgequal(int r1, int r2) { return cgcompare(r1, r2, "sete"); }
int cgnotequal(int r1, int r2) { return cgcompare(r1, r2, "setne"); }
int cglessthan(int r1, int r2) { return cgcompare(r1, r2, "setl"); }
int cggreaterthan(int r1, int r2) {return cgcompare(r1, r2, "setg"); }
int cglessequal(int r1, int r2) { return cgcompare(r1, r2, "setle"); }
int cggreaterequal(int r1, int r2) {return cgcompare(r1, r2, "setge"); }