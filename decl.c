#include "defs.h"
#include "data.h"
#include "decl.h"

static struct symtable *composite_declaration(int type);
static void enum_declaration(void);
static int typedef_declaration(struct symtable **ctype);
static int type_of_typedef(char *name, struct symtable **ctype);

static int parse_type(struct symtable **ctype, int *class) {
    int type, exstatic = 1;

    while (exstatic) {
        switch (Token.token) {
            case T_EXTERN:
                *class = C_EXTERN;
                scan(&Token);
                break;
            default:
                exstatic = 0;
        }
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
        case T_TYPEDEF:
            *class = C_TYPEDEF;
            type = typedef_declaration(ctype);
            if (Token.token == T_SEMI)
                type = -1;
            break;
        case T_IDENT:
            type = type_of_typedef(Text, ctype);
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
                if (Token.intvalue < 0 || Token.intvalue > 255)
                    fatal("Integer literal value too big for char type");
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
        case C_LOCAL:
            sym = addlocl(varname, type, ctype, S_VARIABLE, 1);
            break;
        case C_PARAM:
            sym = addparm(varname, type, ctype, S_VARIABLE,  1);
            break;
        case C_MEMBER:
            sym = addmemb(varname, type, ctype, S_VARIABLE, 1);
            break;
        default:
            return (NULL);
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

static struct symtable *array_declaration(char *varname, int type,
                                struct symtable *ctype, int class) {
    struct symtable *sym;
    int nelems = -1;
    int maxelems;
    int *initlist;
    int i = 0, j;

    // Skip past the '['
    scan(&Token);

    if (Token.token == T_INTLIT) {
        if (Token.intvalue <= 0)
            fatald("Array size is illegal", Token.intvalue);
        nelems = Token.intvalue;
        scan(&Token);
    }

    match(T_RBRACKET, "]");

    switch (class) {
        case C_GLOBAL:
        case C_EXTERN:
            sym = addglob(varname, pointer_to(type), ctype, S_ARRAY, class, 0, 0);
            break;
        default:
            fatal("For now, declaration of non-global arrays is not implemented");
    }

    if (Token.token == T_ASSIGN) {
        if (class != C_GLOBAL)
            fatals("Variable can not be initialised", varname);
        scan(&Token);

        lbrace();

        #define TABLE_INCREMENT 10

        if (nelems != -1)
            maxelems = nelems;
        else
            maxelems = TABLE_INCREMENT;
        initlist= (int *)malloc(maxelems *sizeof(int));

        while (1) {
            if (nelems != -1 && i == maxelems)
                fatal("Too many values in initialisation list");

            initlist[i++] = parse_literal(type);
            scan(&Token);

            if (nelems == -1 && i == maxelems) {
                maxelems += TABLE_INCREMENT;
                initlist = (int *)realloc(initlist, maxelems * sizeof(int));
            }

            if (Token.token == T_RBRACE) {
                scan(&Token);
                break;
            }

            comma();
        }

        for (j = i; j < sym->nelems; ++j)
            initlist[j] = 0;
        if (i > nelems)
            nelems = i;
        sym->initlist = initlist;
    }

    sym->nelems = nelems;
    sym->size = sym->nelems * typesize(type, ctype);

    if (class == C_GLOBAL)
        genglobsym(sym);

    return (sym);
}

static int param_declaration_list(struct symtable *oldfuncsym) {
    int type, paramcnt = 0;
    struct symtable *ctype;
    struct symtable *protoptr = NULL;

    if (oldfuncsym != NULL)
        protoptr = oldfuncsym->member;

    while (Token.token != T_LPAREN) {
        type = declaration_list(&ctype, C_PARAM, T_COMMA, T_RPAREN);
        if (type == -1)
            fatal("Bad type in parameter list");

        if (protoptr != NULL) {
            if (type != protoptr->type)
                fatald("Type doesn't match prototype for parameter", paramcnt + 1);
            protoptr = protoptr->next;
        }
        paramcnt++;

        if (Token.token == T_LPAREN)
            break;
        comma();
    }

    if (oldfuncsym != NULL && paramcnt != oldfuncsym->nelems)
        fatals("Parameter count mismatch for function", oldfuncsym->name);

    return (paramcnt);
}

static struct symtable *function_declaration(char *funcname, int type,
                            struct symtable *ctype, int class) {
    struct ASTnode *tree, *finalstmt;
    struct symtable *oldfuncsym, *newfuncsym = NULL;
    int endlabel, paramcnt;

    if ((oldfuncsym = findsymbol(funcname)) != NULL)
        if (oldfuncsym->stype != S_FUNCTION)
            oldfuncsym = NULL;

    if (oldfuncsym == NULL) {
        endlabel = genlabel();
        newfuncsym = addglob(funcname, type, NULL, S_FUNCTION, C_GLOBAL, endlabel);
    }

    lparen();
    paramcnt = param_declaration_list(oldfuncsym);
    rparen();

    if (newfuncsym) {
        newfuncsym->nelems = paramcnt;
        newfuncsym->member = Parmhead;
        oldfuncsym = newfuncsym;
    }

    Parmhead = Parmtail = NULL;

    if (Token.token == T_SEMI) {
        scan(&Token);
        return (NULL);
    }

    Functionid = oldfuncsym;

    Looplevel = 0;
    Switchlevel = 0;
    lbrace();
    tree = compound_statement(0);
    rbrace();

    if (type != P_VOID) {

        if (tree == NULL)
            fatal("No statements in function with non-void type");

        finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
        if (finalstmt == NULL || finalstmt->op != A_RETURN)
            fatal("No return for function with non-void type");
    }
    return (mkastunary(A_FUNCTION, type, tree, oldfuncsym, endlabel));
}

static struct symtable *composite_declaration(int type) {
    struct symtable *ctype = NULL;
    struct symtable *m;
    int offset;
    int t;

    scan(&Token);

    if (Token.token == T_IDENT) {
        if (type == P_STRUCT)
            ctype = findstruct(Text);
        else
            ctype = findunion(Text);
        scan(&Token);
    }

    if (Token.token != T_LBRACE) {
        if (ctype == NULL)
            fatals("unknown struct/union type", Text);
        return (ctype);
    }

    if (ctype)
        fatals("previously defined struct", Text);

    if (type == P_STRUCT)
        ctype = addstruct(Text);
    else
        ctype = addunion(Text);

    scan(&Token);

    while (1) {
        t = declaration_list(&m, C_MEMBER, T_SEMI, T_RBRACE);
        if (t == -1)
            fatal("Bad type in member list");
        if (Token.token == T_SEMI)
            scan(&Token);
        if (Token.token == T_RBRACE)
            break;
    }

    rbrace();

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
    return (ctype);
}

static void enum_declaration(void) {
    struct symtable *etype = NULL;
    char *name;
    int intval = 0;

    scan(&Token);

    if (Token.token == T_IDENT) {
        etype = findenumtype(Text);
        name = strdup(Text);
        scan(&Token);
    }

    if (Token.token != T_LBRACE) {
        if (etype == NULL)
            fatals("undeclared enum type:", name);
        return;
    }

    scan(&Token);

    if (etype != NULL)
        fatals("enum type redeclared:", etype->name);
    else
        addenum(name, C_ENUMTYPE, 0);

    while (1) {
        ident();
        name = strdup(Text);

        etype = findenumval(name);
        if (etype != NULL)
            fatals("enum value redeclared:", Text);

        if (Token.token == A_ASSIGN) {
            scan(&Token);
            if (Token.token != T_INTLIT)
                fatal("Expected int literal after '='");
            intval = Token.intvalue;
            scan(&Token);
        }

        addenum(name, C_ENUMVAL, intval++);
        if (Token.token == T_RBRACE)
            break;
        comma();
    }
    scan(&Token);
}

static int typedef_declaration(struct symtable **ctype) {
    int type, class = 0;

    scan(&Token);

    type = parse_type(ctype, &class);

    if (findtypedef(Text) != NULL)
        fatals("redefinition of typedef", Text);
    addtypedef(Text, type, *ctype);
    scan(&Token);
    return (type);
}

static int type_of_typedef(char *name, struct symtable **ctype) {
    struct symtable *t;

    t = findtypedef(name);
    if (t == NULL)
        fatals("unknown type", name);

    scan(&Token);
    *ctype = t->ctype;
    return (t->type);
}

static struct symtable *symbol_declaration(int type, struct symtable *ctype,
                                           int class) {
    struct symtable *sym = NULL;
    char *varname = strdup(Text);
    int stype = S_VARIABLE;

    ident();

    if (Token.token == T_LPAREN)
        return (function_declaration(varname, type, ctype, class));

    switch (class) {
        case C_GLOBAL:
        case C_EXTERN:
            if (findglob(varname) != NULL)
                fatals("Duplicate global variable declaration", Text);
            break;
        case C_LOCAL:
        case C_PARAM:
            if (findlocl(varname) != NULL)
                fatals("Duplicate local variable declaration", Text);
            break;
        case C_MEMBER:
            if (findmember(varname) != NULL)
                fatals("Duplicate struct/union member declaration", Text);
            break;
    }

    if (Token.token == T_LBRACKET) {
        sym = array_declaration(varname, type, ctype, class);
        stype = S_ARRAY;
    } else
        sym = scalar_declaration(varname, type, ctype, class);

    return (sym);
}

int declaration_list(struct symtable **ctype, int class, int et1, int et2) {
    int inittype, type;
    struct symtable *sym;

    if ((inittype = parse_type(ctype, &class)) == -1)
        return (inittype);

    while (1) {
        type = parse_stars(inittype);

        sym = symbol_declaration(type, *ctype, class);

        if (sym->stype == S_FUNCTION) {
            fatal("Function definition not at global level");
            return (type);
        }

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