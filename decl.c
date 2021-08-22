#include "defs.h"
#include "data.h"
#include "decl.h"

static struct symtable *composite_declaration(int type);
static void enum_declaration(void);

static struct symtable *symbol_declaration(int type, struct symtable *ctype, int class);

static int parse_type(struct symtable **ctype, int *class) {
    int type;

    if (Token.token == T_EXTERN) {
        *class = C_EXTERN;
        scan(&Token);
    }
    *ctype = NULL;

    switch (Token.token) {
        case T_VOID:
            type = P_VOID;
            scan(&Token);
            break;
        case T_CHAR:
            type = P_CHAR;
            scan(&Token);
            break;
        case T_INT:
            type = P_INT;
            scan(&Token);
            break;
        case T_LONG:
            type = P_LONG;
            scan(&Token);
            break;

        case T_STRUCT:
            type = P_STRUCT;
            *ctype = composite_declaration(P_STRUCT);
            if (Token.token == T_SEMI)
                type = -1;
            break;
        case T_UNION:
            type = P_UNION;
            *ctype = composite_declaration(P_UNION);
            if (Token.token == T_SEMI)
                type = -1;
            break;
        case T_ENUM:
            type = P_INT;
            enum_declaration();
            if (Token.token == T_SEMI)
                type = -1;
            break;
        default:
            fatald("Illegal type, token", Token.token);
    }

    return (type);
}

static int parse_stars(int type) {
    while (1) {
        if (Token.token != T_STAR)
            break;
        type = pointer_to(type);
        scan(&Token);
    }
    return (type);
}

static int parse_literal(int type) {
    struct symtable *etype;

    if ((type == pointer_to(P_CHAR)) && (Token.token == T_STRLIT))
        return (genglobstr(Text));

    if ((Token.token == T_IDENT) && ((etype = findenumval(Text)) != NULL)) {
        Token.token = T_INTLIT;
        Token.intvalue = etype->posn;
    }

    if (Token.token == T_INTLIT) {
        switch (type) {
            case P_CHAR:
                if (Token.intvalue < -128 || Token.intvalue > 255)
                    warn("overflow in implicit constant conversion");
            case P_INT:
            case P_LONG:
                break;
            default:
                fatal("Type mismatch: integer literal vs. variable");
        }
    } else
        fatal("initializer element is not constant");

    return (Token.intvalue);
}

static struct symtable *scalar_declaration(char *varname, int type,
                                struct symtable *ctype, int class) {

    struct symtable *sym = NULL;

    switch (class) {
        case C_GLOBAL:
        case C_EXTERN:
            sym = addglob(varname, type, ctype, S_VARIABLE, class, 1, 0);
            break;
        case C_MEMBER:
            sym = addmemb(varname, type, ctype, S_VARIABLE, 1);
            break;
    }

    if (Token.token == T_ASSIGN) {
        if (class != C_GLOBAL && class != C_LOCAL)
            fatals("Variable can not be initialised", varname);
        scan(&Token);

        if (class == C_GLOBAL) {
            sym->initlist =(int *)malloc(sizeof(int));
            sym->initlist[0] = parse_literal(type);
            scan(&Token);
        }
    }

    if (class == C_GLOBAL)
        genglobsym(sym);

    return (sym);
}

static struct symtable *composite_declaration(int type) {
    struct symtable *ctype  = NULL;
    struct symtable *m;
    char *name;
    int t, offset;

    // Skip the struct/union keyword
    scan(&Token);

    if (Token.token == T_IDENT) {
        if (type == P_STRUCT)
            ctype = findstruct(Text);
        else
            ctype = findunion(Text);
        name = strdup(Text);
        scan(&Token);
    }

    if (Token.token != T_LBRACE) {
        if (ctype == NULL)
            fatals("undefined struct/union type", name);
        return (ctype);
    }

    if (ctype != NULL)
        fatals("previously defined struct/union", Text);

    if (type == P_STRUCT)
        ctype = addstruct(Text);
    else
        ctype = addunion(Text);

    lbrace();

    while (1) {
        t = declaration_list(C_MEMBER, T_SEMI, T_RBRACE);
        if (t == -1)
            fatal("Bad type in member list");
        if (Token.token == T_SEMI)
            semi();
        if (Token.token == T_RBRACE)
            break;
    }

    rbrace();

    if (Membhead == NULL)
        fatals("No members in struct", ctype->name);

    ctype->member = Membhead;

    Membhead = Membtail = NULL;

    m = ctype->member;
    m->posn = 0;

    offset = typesize(m->type, m->ctype);

    for (m = m->next; m != NULL; m = m->next) {
        if (type == P_STRUCT) {
            m->posn = genalign(m->type, offset, 1);
            offset = m->posn + (typesize(m->type, m->ctype)>4 ? typesize(m->type, m->ctype) : 4);
        } else {
            m->posn = 0;
            offset = typesize(m->type, m->ctype) > offset ? typesize(m->type, m->ctype) : offset;
        }
    }

    ctype->size = offset;

    while (Token.token != T_SEMI) {
        if (type == P_STRUCT)
            t = parse_stars(P_STRUCT);
        else
            t = parse_stars(P_UNION);
        symbol_declaration(t, ctype, C_GLOBAL);

        if (Token.token == T_SEMI)
            break;
        comma();
    }

    return (ctype);
}

static void enum_declaration(void) {
    struct symtable *etype = NULL;
    char *name;
    int intval = 0;

    // Skip the enum keyword
    scan(&Token);

    if (Token.token == T_IDENT) {
        etype = findenumtype(Text);
        name = strdup(Text);
        scan(&Token);
    }

    if (Token.token != T_LBRACE) {
        if (etype == NULL)
            fatals("undeclared enum type", name);
        return;
    }

    if (etype != NULL)
        fatals("enum type redeclared", name);

    addenum(name, C_ENUMTYPE, 0);

    lbrace();

    while (Token.token != T_RBRACE) {
        ident();
        if (findenumval(Text) != NULL)
            fatals("enum value redeclared", Text);

        if (Token.token == T_ASSIGN) {
            scan(&Token);
            if (Token.token != T_INTLIT)
                fatal("Expected int literal after '='");
            intval = Token.intvalue;
            scan(&Token);
        }

        addenum(Text, C_ENUMVAL, intval);
        intval++;

        if (Token.token == T_RBRACE)
            break;

        comma();
    }

    rbrace();
}

static struct symtable *symbol_declaration(int type, struct symtable *ctype,
                                           int class) {
    struct symtable *sym = NULL;
    char *varname = strdup(Text);
    
    ident();

    switch (class) {
        case C_GLOBAL:
        case C_EXTERN:
            if (findglob(varname) != NULL)
                fatals("Duplicate global variable declaration", varname);
            break;
        case C_MEMBER:
            if (findmember(varname) != NULL)
                fatals("Duplicate struct/union member declaration", varname);
    }

    sym = scalar_declaration(varname, type, ctype, class);

    return (sym);
}

int declaration_list(int class, int et1, int et2) {
    struct symtable *ctype;
    int inittype, type;

    if ((inittype = parse_type(&ctype, &class)) == -1)
        return (inittype);

    while (1) {
        type = parse_stars(inittype);

        symbol_declaration(type, ctype, class);

        if (Token.token == et1 || Token.token == et2)
            return (type);

        comma();
    }

}

void global_declarations(void) {
    while (Token.token != T_EOF) {
        declaration_list(C_GLOBAL, T_SEMI, T_EOF);
        if (Token.token == T_SEMI)
            scan(&Token);
    }
}