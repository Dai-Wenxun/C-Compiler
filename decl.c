#include "defs.h"
#include "data.h"
#include "decl.h"


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
    if ((type == pointer_to(P_CHAR)) && (Token.token == T_STRLIT))
        return (genglobstr(Text));

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
        fatal("Expecting an integer literal value");

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

static struct symtable *symbol_declaration(int type, struct symtable *ctype,
                                           int class) {
    struct symtable *sym = NULL;
    char *varname = strdup(Text);

    ident();

    switch (class) {
        case C_GLOBAL:
        case C_EXTERN:
            if (findglob(varname) != NULL)
                fatals("Duplicate global variable declaration", Text);
            break;
    }

    sym = scalar_declaration(varname, type, ctype, class);

    return (sym);
}

int declaration_list(struct symtable **ctype, int class, int et1, int et2) {
    int inittype, type;
    struct symtable *sym;

    inittype = parse_type(ctype, &class);

    while (1) {
        type = parse_stars(inittype);

        sym = symbol_declaration(type, *ctype, class);

        if (Token.token == et1 || Token.token == et2)
            return (type);

        comma();
    }

}

void global_declarations(void) {
    struct symtable *ctype;
    while (Token.token != T_EOF) {
        declaration_list(&ctype, C_GLOBAL, T_SEMI, T_EOF);
        if (Token.token == T_SEMI)
            scan(&Token);
    }
}