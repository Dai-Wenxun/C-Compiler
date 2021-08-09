#include "defs.h"
#include "data.h"
#include "decl.h"

int findglob(char *s) {
    int i;

    for (i = 0; i < Globs; ++i) {
        if (Symtable[i].class == C_PARAM) continue;
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

void freeloclsyms(void) {
    Locls = NSYMBOLS - 1;
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

int addlocl(char *name, int type, int stype, int isparam, int size) {
    int localslot, globalslot;

    if ((localslot = findlocl(name)) != -1)
        return (-1);

    localslot = newlocl();
    if (isparam) {
        updatesym(localslot, name, type, stype, C_PARAM, 0, size, 0);
        globalslot = newglob();
        updatesym(globalslot, name, type, stype, C_PARAM, 0, size, 0);
    } else {
        updatesym(localslot, name, type, stype, C_LOCAL, 0, size, 0);
    }
    return (localslot);
}

int findsymbol(char *s) {
    int slot;

    slot = findlocl(s);
    if (slot == -1)
        slot = findglob(s);
    return (slot);
}