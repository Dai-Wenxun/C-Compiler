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
                        int stype, int class, int nelems, int posn) {
    struct symtable *node = (struct symtable *)malloc(sizeof(struct symtable));
    if (node == NULL)
        fatal("Unable to malloc a symbol table node in newsym()");

    node->name = strdup(name);
    node->type = type;
    node->ctype = ctype;
    node->stype = stype;
    node->class = class;
    node->nelems = nelems;
    node->posn = posn;

    if (ptrtype(type) || inttype(type))
        node->size =  nelems * typesize(type, ctype);

    node->initlist = NULL;
    node->next = NULL;
    node->member = NULL;

    return (node);
}

struct symtable *addglob(char *name, int type, struct symtable *ctype,
                int stype, int class, int nelems, int posn) {
    struct symtable *sym = newsym(name, type, ctype, stype, class, nelems, posn);
    appendsym(&Globhead, &Globtail, sym);
    return (sym);
}

struct symtable *addlocl(char *name, int type, struct symtable *ctype,
                int stype, int nelems) {
    struct symtable *sym = newsym(name, type, ctype, stype, C_LOCAL, nelems, 0);


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
                int stype, int nelems) {
    struct symtable *sym = newsym(name, type, ctype, stype, C_MEMBER, nelems, 0);
    if (type == P_STRUCT || type == P_UNION)
        sym->size = ctype->size;
    appendsym(&Membhead, &Membtail, sym);
    return (sym);
}

struct symtable *addstruct(char *name) {
    struct symtable *sym = newsym(Text, P_STRUCT, NULL, 0, C_STRUCT, 0, 0);
    appendsym(&Structhead, &Structtail, sym);
    return (sym);
}

struct symtable *addunion(char *name) {
    struct symtable *sym = newsym(name, P_UNION, NULL, 0, C_UNION, 0, 0);
    appendsym(&Unionhead, &Uniontail, sym);
    return (sym);
}

struct symtable *addenum(char *name, int class, int value) {
    struct symtable *sym = newsym(name, P_INT, NULL, 0, class, 0, value);
    appendsym(&Enumhead, &Enumtail, sym);
    return (sym);
}

struct symtable *addtypedef(char *name, int type, struct symtable *ctype) {
    struct symtable *sym = newsym(name, type, ctype, 0, C_TYPEDEF, 0, 0);
    appendsym(&Typehead, &Typetail, sym);
    return (sym);
}

static struct symtable *findsyminlist(char *s, struct symtable *list, int class) {
    for (; list != NULL; list = list->next) {
        if ((list->name != NULL) && !strcmp(s, list->name))
            if (class == 0 || class == list->class)
                return (list);
    }
    return (NULL);
}

struct symtable *findglob(char *s) {
    return (findsyminlist(s, Globhead, 0));
}

struct symtable *findlocl(char *s) {
    struct symtable *node;

    if (Functionid) {
        node = findsyminlist(s, Functionid->member, 0);
        if (node)
            return (node);
    }
    return (findsyminlist(s, Loclhead, 0));
}

struct symtable *findsymbol(char *s) {
    struct symtable *node;

    if (Functionid) {
        node = findsyminlist(s, Functionid->member, 0);
        if (node)
            return (node);
    }
    node = findsyminlist(s, Loclhead, 0);
    if (node)
        return (node);
    return (findsyminlist(s, Globhead, 0));
}

struct symtable *findmember(char *s) {
    return (findsyminlist(s, Membhead, 0));
}

struct symtable *findstruct(char *s) {
    return (findsyminlist(s, Structhead, 0));
}

struct symtable *findunion(char *s) {
    return (findsyminlist(s, Unionhead, 0));
}

struct symtable *findenumtype(char *s) {
    return (findsyminlist(s, Enumhead, C_ENUMTYPE));
}

struct symtable *findenumval(char *s) {
    return (findsyminlist(s, Enumhead, C_ENUMVAL));
}

struct symtable *findtypedef(char *s) {
    return (findsyminlist(s, Typehead, 0));
}

void clear_symtable(void) {
    Globhead = Globtail = NULL;
    Loclhead = Locltail = NULL;
    Parmhead = Parmtail = NULL;
    Membhead = Membtail = NULL;
    Structhead = Structtail = NULL;
    Unionhead = Uniontail = NULL;
    Enumhead = Enumtail = NULL;
    Typehead = Typetail = NULL;
}

void freeloclsyms(void) {
    Loclhead = Locltail = NULL;
    Functionid = NULL;
}