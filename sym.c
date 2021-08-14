#include "defs.h"
#include "data.h"
#include "decl.h"

void appendsym(struct symtable **head, struct symtable **tail,
        struct symtable *node) {
    if (head == NULL || tail == NULL || node == NULL)
        fatal("Either head, tail or node is NULL in appendsym()");

    if (*tail) {
        (*tail)->next = node;
        *tail = node;
    } else
        *head = *tail = node;
    node->next = NULL;
}

static struct symtable *newsym(char *name, int type, int stype,
                               int class, int size, int posn) {
    struct symtable *node = (struct symtable *)malloc(sizeof(struct symtable));
    if (node == NULL)
        fatal("Unable to malloc a symbol table node in newsym()");

    node->name = strdup(name);
    node->type = type;
    node->stype = stype;
    node->class = class;
    node->size = size;
    node->posn = posn;
    node->next = NULL;
    node->member = NULL;

    if (class == C_GLOBAL)
        genglobsym(node);
    return (node);
}

struct symtable *addglob(char *name, int type, int stype, int class, int size) {
    struct symtable *sym = newsym(name, type, stype, class, size, 0);
    appendsym(&Globhead, &Globtail, sym);
    return (sym);
}

struct symtable *addlocl(char *name, int type, int stype, int class, int size) {
    struct symtable *sym = newsym(name, type, stype, class, size, 0);
    appendsym(&Loclhead, &Locltail, sym);
    return (sym);
}

struct symtable *addparm(char *name, int type, int stype, int class, int size) {
    struct symtable *sym = newsym(name, type, stype, class, size, 0);
    appendsym(&Parmhead, &Parmtail, sym);
    return (sym);
}

static struct symtable *findsyminlist(char *s, struct symtable *list) {
    for (; list != NULL; list = list->next) {
        if ((list->name != NULL) && !strcmp(s, list->name))
            return (list);
    }
    return (NULL);
}

struct symtable *findglob(char *s) {
    return (findsyminlist(s, Globhead));
}

struct symtable *findlocl(char *s) {
    struct symtable *node;

    if (Functionid) {
        node = findsyminlist(s, Functionid->member);
        if (node)
            return (node);
    }
    return (findsyminlist(s, Loclhead));
}

struct symtable *findsymbol(char *s) {
    struct symtable *node;

    if (Functionid) {
        node = findsyminlist(s, Functionid->member);
        if (node)
            return (node);
    }
    node = findsyminlist(s, Loclhead);
    if (node)
        return (node);
    return (findsyminlist(s, Globhead));
}

struct symtable *findcomposite(char *s) {
    return (findsyminlist(s, Comphead));
}

void clear_symtable(void) {
    Globhead = Globtail = NULL;
    Loclhead = Locltail = NULL;
    Parmhead = Parmtail = NULL;
    Comphead = Comptail = NULL;
}

void freeloclsyms(void) {
    Loclhead = Locltail = NULL;
    Parmhead = Parmtail = NULL;
    Functionid = NULL;
}