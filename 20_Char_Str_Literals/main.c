#include "defs.h"
#define extern_
#include "data.h"
#undef extern_
#include "decl.h"

static void init() {
    Line = 1;
    Putback = '\n';
    Globs = 0;
    O_dumpAST = 0;
}

static void usage(char *prog) {
    fprintf(stderr, "Usage: %s [-T] infile\n", prog);
    exit(1);
}

int main(int argc, char* argv[]) {
    int i;

    init();

    for (i = 1; i < argc; i++) {
        if (*argv[i] != '-') break;
        for (int j = 1; argv[i][j]; j++) {
            switch (argv[i][j]) {
                case 'T': O_dumpAST = 1; break;
                default: usage(argv[0]);
            }
        }
    }

    if (i >= argc)
        usage(argv[0]);

    if ((Infile = fopen(argv[i], "r")) == NULL) {
        fprintf(stderr, "Unable to open %s: %s\n", argv[i], strerror(errno));
        exit(1);
    }
    if ((Outfile = fopen("out.s", "w")) == NULL) {
        fprintf(stderr, "Unable to create out.s: %s\n", strerror(errno));
        exit(1);
    }

    addglob("printint", P_VOID, S_FUNCTION, 0, 0);
    addglob("printchar", P_VOID, S_FUNCTION, 0, 0);

    scan(&Token);
    genpreamble();
    global_declarations();
    genpostamble();
    fclose(Outfile);
    return (0);
}