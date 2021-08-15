#include "defs.h"
#include "data.h"
#include "decl.h"

static void appendsym(struct symtable **head, struct symtable **tail,
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

static struct symtable *newsym(char *name, int type, struct symtable *ctype,
                        int stype, int class, int size, int posn) {
    struct symtable *node = (struct symtable *)malloc(sizeof(struct symtable));
    if (node == NULL)
        fatal("Unable to malloc a symbol table node in newsym()");

    node->name = strdup(name);
    node->type = type;
    node->ctype = ctype;
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

struct symtable *addglob(char *name, int type, struct symtable *ctype,
                int stype, int size) {
    struct symtable *sym = newsym(name, type, ctype, stype, C_GLOBAL, size, 0);
    appendsym(&Globhead, &Globtail, sym);
    return (sym);
}

struct symtable *addlocl(char *name, int type, struct symtable *ctype,
                int stype, int size) {
    struct symtable *sym = newsym(name, type, ctype, stype, C_LOCAL, size, 0);
    appendsym(&Loclhead, &Locltail, sym);
    return (sym);
}

struct symtable *addparm(char *name, int type, struct symtable *ctype,
                int stype, int size) {
    struct symtable *sym = newsym(name, type, ctype, stype, C_PARAM, size, 0);
    appendsym(&Parmhead, &Parmtail, sym);
    return (sym);
}

struct symtable *addmemb(char *name, int type, struct symtable *ctype,
                int stype, int size) {
    struct symtable *sym = newsym(name, type, ctype, stype, C_MEMBER, size, 0);
    appendsym(&Membhead, &Membtail, sym);
    return (sym);
}

struct symtable *addstruct(char *name, int type, struct symtable *ctype,
                int stype, int size) {
    struct symtable *sym = newsym(name, type, ctype, stype, C_STRUCT, size, 0);
    appendsym(&Structhead, &Structtail, sym);
    return (sym);
}

struct symtable *addunion(char *name, int type, struct symtable *ctype,
                int stype, int size) {
    struct symtable *sym = newsym(name, type, ctype, stype, C_UNION, size, 0);
    appendsym(&Unionhead, &Uniontal, sym);
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

struct symtable *findmember(char *s) {
    return (findsyminlist(s, Membhead));
}

struct symtable *findstruct(char *s) {
    return (findsyminlist(s, Structhead));
}

struct symtable *findunion(char *s) {
    return (findsyminlist(s, Unionhead));
}

void clear_symtable(void) {
    Globhead = Globtail = NULL;
    Loclhead = Locltail = NULL;
    Parmhead = Parmtail = NULL;
    Membhead = Membtail = NULL;
    Structhead = Structtail = NULL;
}

void freeloclsyms(void) {
    Loclhead = Locltail = NULL;
    Parmhead = Parmtail = NULL;
    Functionid = NULL;
}