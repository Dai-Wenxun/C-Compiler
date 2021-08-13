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

    if ((p = Globs++) > Locls)
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

    if ((p = Locls--) < Globs)
        fatal("Too many local symbols");
    return (p);
}

void freeloclsyms(void) {
    Locls = NSYMBOLS - 1;
}

static void updatesym(int slot, char *name, int type, int stype,
                    int class, int size, int posn) {
    Symtable[slot].name = strdup(name);
    Symtable[slot].type = type;
    Symtable[slot].stype = stype;
    Symtable[slot].class = class;
    Symtable[slot].size = size;
    Symtable[slot].posn = posn;
}

int addglob(char *name, int type, int stype, int class, int size) {
    int slot;

    if ((slot = findglob(name)) != -1)
        return (slot);

    slot = newglob();
    updatesym(slot, name, type, stype, class, size, 0);

    if (class == C_GLOBAL)
        genglobsym(slot);

    return (slot);
}

int addlocl(char *name, int type, int stype, int class, int size) {
    int slot;

    if ((slot = findlocl(name)) != -1)
        return (-1);

    slot = newlocl();

    updatesym(slot, name, type, stype, class, size, 0);
    return (slot);
}


void copyfuncparams(int slot) {
    int i, id = slot + 1;

    for (i = 0; i < Symtable[slot].nelems; ++i, ++id) {
        addlocl(Symtable[id].name, Symtable[id].type, Symtable[id].stype,
                Symtable[id].class, Symtable[id].size);
    }
}


int findsymbol(char *s) {
    int slot;

    slot = findlocl(s);
    if (slot == -1)
        slot = findglob(s);
    return (slot);
}

void clear_symtable(void) {
    Globs = 0;
    Locls = NSYMBOLS - 1;
}