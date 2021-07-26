#include "../defs.h"
#include "../data.h"
#include "../decl.h"

static int Globs = 0;

int findglob(char *s) {
    for (int i = 0; i < Globs; ++i) {
        if (*s == *Gsym[i].name && !strcmp(s, Gsym[i].name))
            return i;
    }
    return -1;
}

static int newglob(void) {
    int p;

    if ((p = Globs++) >= NSYMBOLS)
        fatal("Too many global symbols");
    return p;
}

int addglob(char *name) {
    int y;

    if ((y = findglob(name)) != -1)
        return y;

    y = newglob();
    Gsym[y].name = strdup(name);
    return y;
}