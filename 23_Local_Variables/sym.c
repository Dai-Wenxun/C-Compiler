#include "defs.h"
#include "data.h"
#include "decl.h"

int findglob(char *s) {
    int i;

    for (i = 0; i < Globs; ++i) {
        if (*s == *Symtable[i].name && !strcmp(s, Symtable[i].name))
            return (i);
    }
    return (-1);
}

static int newglob(void) {
    int p;

    if ((p = Globs++) >= Locls)
        fatal("Too many global symbols");
    return (p);
}

int findlocl(char *s) {
    int i;

    for (i = Locls + 1; i < NSYMBOLS; ++i) {
        if (*s == *Symtable[i].name && !strcmp(s, Symtable[i].name))
            return (i);
    }
    return (-1);
}

static int newlocl(void) {
    int p;

    if ((p = Locls--) <= Globs)
        fatal("Too many local symbols");
    return (p);
}

static void updatesym(int slot, char *name, int type, int stype,
                    int class, int endlabel, int size, int posn) {
    Symtable[slot].name = strdup(name);
    Symtable[slot].type = type;
    Symtable[slot].stype = stype;
    Symtable[slot].class = class;
    Symtable[slot].endlabel = endlabel;
    Symtable[slot].size = size;
    Symtable[slot].posn = posn;
}

int addglob(char *name, int type, int stype, int endlabel, int size) {
    int slot;

    if ((slot = findglob(name)) != -1)
        return (slot);

    slot = newglob();
    updatesym(slot, name, type, stype, C_GLOBAL, endlabel, size, 0);
    genglobsym(slot);

    return (slot);
}

int addlocl(char *name, int type, int stype, int endlabel, int size) {
    int slot, posn;

    if ((slot = findlocl(name)) != -1)
        return (slot);

    slot = newlocl();
    posn = gengetlocaloffset(type, 0);
    updatesym(slot, name, type, stype, C_LOCAL, endlabel, size, posn);
    return (slot);
}

int findsymbol(char *s) {
    int slot;

    slot = findlocl(s);
    if (slot == -1)
        slot = findglob(s);
    return (slot);
}