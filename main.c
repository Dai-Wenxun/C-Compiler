#include "defs.h"
#define extern_
#include "data.h"
#undef extern_
#include "decl.h"
#include <unistd.h>

static void init() {
    Line = 1;
    Putback = '\n';
    addglob("printf", P_INT, NULL, S_FUNCTION, C_GLOBAL, 0, 0);
}

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
    Infilename = filename;
    Outfilename = alter_suffix(filename, 's');

    if (Outfilename == NULL) {
        fprintf(stderr, "[Error]: %s has no suffix, try .c on the end\n", filename);
        exit(1);
    }

    if ((Infile = fopen(Infilename, "r")) == NULL) {
        fprintf(stderr, "[Error]: unable to open %s: %s\n", Infilename, strerror(errno));
        exit(1);
    }

    if ((Outfile = fopen(Outfilename, "w")) == NULL) {
        fprintf(stderr, "[Error]: unable to create %s: %s\n", Outfilename, strerror(errno));
        exit(1);
    }

    clear_symtable();
    init();
    scan(&Token);
    genpreamble();
    global_declarations();
    genpostamble();
    fclose(Outfile);
    return (Outfilename);
}

int main(int argc, char* argv[]) {

    do_compile(argv[1]);
    return (0);
}