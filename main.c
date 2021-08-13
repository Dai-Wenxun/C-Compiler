#include "defs.h"
#define extern_
#include "data.h"
#undef extern_
#include "decl.h"
#include <unistd.h>

char *alter_suffix(char *str, char suffix) {
    char *posn;
    char *newstr;

    if ((newstr = strdup(str)) == NULL)
        return (NULL);

    if ((posn = strrchr(newstr, '.')) == NULL)
        return (NULL);

    posn++;
    if (*posn == '\0')
        return (NULL);

    *posn++ = suffix;
    *posn = '\0';

    return (newstr);
}

static char *do_compile(char *filename) {
    Outfilename = alter_suffix(filename, 's');

    if (Outfilename == NULL) {
        fprintf(stderr, "Error: %s has no suffix, try .c on the end\n", filename);
        exit(1);
    }

    if ((Infile = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Unable to open %s: %s\n", filename, strerror(errno));
        exit(1);
    }

    if ((Outfile = fopen(Outfilename, "w")) == NULL) {
        fprintf(stderr, "Unable to create %s: %s\n", Outfilename, strerror(errno));
        exit(1);
    }

    Line = 1;
    Putback = '\n';
    clear_symtable();
    if (O_verbose)
        printf("compiling %s\n", filename);
    scan(&Token);
    genpreamble();
    global_declarations();
    genpostamble();
    fclose(Outfile);
    return (Outfilename);
}

char *do_assemble(char *filename) {
    char cmd[TEXTLEN];
    int err;

    char *outfilename = alter_suffix(filename, 'o');
    if (outfilename == NULL) {
        fprintf(stderr, "Error: %s has no suffix, try .s on the end\n", filename);
        exit(1);
    }

    snprintf(cmd, TEXTLEN, "%s %s %s", ASCMD, outfilename, filename);

    if (O_verbose)
        printf("%s\n", cmd);

    err = system(cmd);
    if (err != 0) {
        fprintf(stderr, "Assembly of %s failed\n", filename);
        exit(1);
    }

    return (outfilename);
}

void do_link(char *outfilename, char *objlist[]) {
    int cnt, size = TEXTLEN;
    char cmd[TEXTLEN], *cptr;
    int err;

    cptr = cmd;
    cnt = snprintf(cptr, size, "%s %s ", LDCMD, outfilename);
    cptr += cnt;
    size -= cnt;

    while (*objlist != NULL) {
        cnt = sprintf(cptr, size, "%s ", *objlist);
        cptr += cnt;
        size -= cnt;
        objlist++;
    }

    if (O_verbose)
        printf("%s\n", cmd);
    err = system(cmd);
    if (err != 0) {
        fprintf(stderr, "Linking failed\n");
        exit(1);
    }
}

static void usage(char *prog) {
    fprintf(stderr, "Usage: %s [-vcST] [-o outfile] file [file ...]\n", prog);
    fprintf(stderr, "       -v give verbose output of the compilation stages\n");
    fprintf(stderr, "       -c generate object files but don't link them\n");
    fprintf(stderr, "       -S generate assembly files but don't link them\n");
    fprintf(stderr, "       -T dump the AST trees for each input file\n");
    fprintf(stderr, "       -o outfile, produce the outfile executable file\n");
    exit(1);
}

#define MAXOBJ 100
int main(int argc, char* argv[]) {
    char *outfilename = AOUT;
    char *asmfile, *objfile;
    char *objlist[MAXOBJ];
    int i, objcnt = 0;

    O_dumpAST = 0;
    O_keepasm = 0;
    O_assemble = 0;
    O_verbose = 0;
    O_dolink = 1;

    for (i = 1; i < argc; ++i) {
        if (*argv[i] != '-')
            break;
        for (int j = 1; argv[i][j]; ++j) {
            switch (argv[i][j]) {
                case 'o':
                    outfilename = argv[++i];
                    break;
                case 'T':
                    O_dumpAST = 1;
                    break;
                case 'c':
                    O_assemble = 1;
                    O_keepasm = 0;
                    O_dolink = 0;
                    break;
                case 'S':
                    O_keepasm = 1;
                    O_assemble = 0;
                    O_dolink = 0;
                    break;
                case 'v':
                    O_verbose = 1;

                default:
                    usage(argv[0]);
            }
        }
    }

    while (i < argc) {
        asmfile = do_compile(argv[i]);

        if (O_dolink || O_assemble) {
            objfile = do_assemble(asmfile);
            if (objcnt == (MAXOBJ - 2)) {
                fprintf(stderr, "Too many object files for the compiler to handle\n");
                exit(1);
            }

            objlist[objcnt++] = objfile;
            objlist[objcnt] = NULL;
        }

        if (!O_keepasm)
            unlink(asmfile);
        i++;
    }

    if (O_dolink) {
        do_link(outfilename, objlist);

        if (!O_assemble) {
            for (i = 0; objlist[i] != NULL; ++i) {
                unlink(objlist[i]);
            }
        }
    }


    return (0);
}