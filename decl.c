#include "defs.h"
#include "data.h"
#include "decl.h"

static struct symtable *composite_declaration(int type);
static void enum_declaration(void);
static void typedef_declaration(void);
static int type_of_typedef(struct symtable **ctype);
static struct symtable *symbol_declaration(int type, struct symtable *ctype,
                                        int class, struct ASTnode **tree);

int parse_type(struct symtable **ctype, int *class) {
    int type;

    if (Token.token == T_EXTERN) {
        if (*class == C_TYPEDEF || *class == C_PARAM)
            fatal("multiple storage classes conflict");
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
        case T_TYPEDEF:
            typedef_declaration();
            type = -1;
            break;
        case T_IDENT:
            type = type_of_typedef(ctype);
            break;
        default:
            fatals("illegal type, token", Token.tokptr);
    }

    return (type);
}

int parse_stars(int type) {
    while (1) {
        if (Token.token != T_STAR)
            break;
        type = pointer_to(type);
        scan(&Token);
    }
    return (type);
}

int parse_cast(void) {
    int type, class;
    struct symtable *ctype;

    type = parse_stars(parse_type(&ctype, &class));

    if (type == P_STRUCT || type == P_UNION || type == P_VOID)
        fatal("cannot cast to a struct, union or void type");
    return (type);
}

int parse_literal(int type) {
    struct ASTnode *tree;

    tree = optimise(binexpr(0));

    if (tree->op == A_CAST) {
        tree->left->type = tree->type;
        tree = tree->left;
    }
    if (tree->op != A_INTLIT && tree->op != A_STRLIT)
        fatal("Cannot initialise globals with a general expression");

    if (type == pointer_to(P_CHAR)) {
        if (tree->op == A_STRLIT)
            return (tree->intvalue);
        if (tree->op == A_INTLIT && tree->intvalue == 0)
            return (0);
    }

    if (inttype(type) && typesize(type, NULL) >= typesize(tree->type, NULL))
        return (tree->intvalue);

    fatal("type mismatch");
    return (0);
}

static struct symtable *scalar_declaration(char *varname, int type,
                                struct symtable *ctype, int class,
                                struct ASTnode **tree) {
    struct symtable *sym = NULL;
    struct ASTnode *varnode, *exprnode;

    switch (class) {
        case C_GLOBAL:
        case C_EXTERN:
            sym = addglob(varname, type, ctype, S_VARIABLE, class, 1, 0);
            break;
        case C_PARAM:
            sym = addparm(varname, type, ctype, S_VARIABLE);
            break;
        case C_LOCAL:
            sym = addlocl(varname, type, ctype, S_VARIABLE, 1);
            break;
        case C_MEMBER:
            sym = addmemb(varname, type, ctype, S_VARIABLE, 1);
            break;
    }

    if (Token.token == T_ASSIGN) {
        if (class != C_GLOBAL && class != C_LOCAL)
            fatals("variable can not be initialised", varname);
        scan(&Token);

        if (class == C_GLOBAL) {
            sym->initlist =(int *)malloc(sizeof(int));
            sym->initlist[0] = parse_literal(type);
        } else {
            varnode = mkastleaf(A_IDENT, type, sym, 0);
            exprnode = binexpr(0);
            exprnode->rvalue = 1;

            exprnode = modify_type(exprnode, varnode->type, 0);
            if (exprnode == NULL)
                fatal("incompatible type in assignment");

            *tree = mkastnode(A_ASSIGN, varnode->type, exprnode, NULL, varnode,  NULL, 0);
        }
    }

    if (class == C_GLOBAL)
        genglobsym(sym);

    return (sym);
}

static struct symtable *array_declaration(char *varname, int type,
                                struct symtable *ctype, int class) {
    struct symtable *sym;
    int i = 0, maxelems;
    int casttype;

    switch (class) {
        case C_GLOBAL:
        case C_EXTERN:
            sym = addglob(varname, pointer_to(type), ctype, S_ARRAY, class, 0, 0);
            break;
        default:
            fatal("declaration of non-global arrays is not implemented");
    }

    scan(&Token);

    if (Token.token != T_RBRACKET) {
        sym->nelems = parse_literal(type);
        if (sym->nelems <= 0)
            fatald("array size is illegal", sym->nelems);
    }

    match(T_RBRACKET, "]");

    if (Token.token == T_ASSIGN) {
        if (class != C_GLOBAL)
            fatals("variable can not be initialised", varname);

        // Skip the '='
        scan(&Token);

        lbrace();

        #define INCREMENT 2
        if (sym->nelems != 0)
            maxelems = sym->nelems;
        else
            maxelems = INCREMENT;

        sym->initlist = (int *)malloc(maxelems * sizeof(int));

        while (Token.token != T_RBRACE) {
            if (Token.token == T_LPAREN) {
                lparen();
                casttype = parse_cast();
                rparen();
                if (casttype == type ||casttype == pointer_to(P_VOID) && ptrtype(type))
                    type = P_NONE;
                else
                    fatal("type mismatch");
            }
            sym->initlist[i++] = parse_literal(type);

            if (sym->nelems == 0 && i == maxelems) {
                maxelems += INCREMENT;
                sym->initlist = (int *)realloc(sym->initlist, maxelems * sizeof(int));
            }

            if (sym->nelems != 0 && i == maxelems) {
                if (Token.token != T_RBRACE)
                    fatal("too many values in initialisation list");
                break;
            }

            if (Token.token == T_RBRACE) {
                if (sym->nelems == 0)
                    sym->nelems = i;
                break;
            }

            comma();
        }

        for (; i < sym->nelems;) {
            sym->initlist[i++] = 0;
        }

        rbrace();
    }

    if (sym->nelems == 0) {
        sym->nelems = 1;
        warn("array assumed to have one element");
    }


    if (class == C_GLOBAL)
        genglobsym(sym);

    return (sym);
}

static struct symtable *composite_declaration(int type) {
    struct symtable *ctype  = NULL;
    struct symtable *m;
    struct ASTnode *unused;
    char *name;
    int t, offset;
    int union_id = 0, f_posn;

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

    if (type == P_STRUCT) {
        ctype = addstruct(Text);
    } else {
        ctype = addunion(Text);
    }

    lbrace();

    while (Token.token != T_RBRACE) {
        if (Token.token == T_UNION) {
            scan(&Token);
            lbrace();
            while (Token.token != T_RBRACE) {
                t = declaration_list(C_MEMBER, T_SEMI, T_RBRACE, &unused);
                Membtail->posn = union_id++;
                if (t == -1)
                    fatal("bad type in member list");
                if (Token.token == T_SEMI)
                    semi();
                if (Token.token == T_RBRACE)
                    break;
            }
            rbrace();
            semi();
            union_id = 0;
        } else {
            t = declaration_list(C_MEMBER, T_SEMI, T_RBRACE, &unused);
            if (t == -1)
                fatal("bad type in member list");
            if (Token.token == T_SEMI)
                semi();
            if (Token.token == T_RBRACE)
                break;
        }

    }

    rbrace();

    if (Membhead == NULL)
        fatals("no members in struct/union", ctype->name);

    ctype->member = Membhead;

    Membhead = Membtail = NULL;

    m = ctype->member;
    m->posn = 0;
    f_posn = -1;
    offset = typesize(m->type, m->ctype);

    for (m = m->next; m != NULL; f_posn = m->posn, m = m->next) {
        if (type == P_STRUCT) {
            if (m->posn > 0)
                m->posn = f_posn;
            else
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
        symbol_declaration(t, ctype, C_GLOBAL, &unused);

        if (Token.token == T_SEMI)
            break;
        comma();
    }

    return (ctype);
}

static void enum_declaration(void) {
    struct symtable *etype = NULL;
    char *name = NULL;
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

    if (name != NULL)
        addenum(name, C_ENUMTYPE, 0);

    lbrace();

    while (Token.token != T_RBRACE) {
        ident();
        if (findenumval(Text) != NULL)
            fatals("enum value redeclared", Text);

        if (Token.token == T_ASSIGN) {
            scan(&Token);
            if (Token.token != T_INTLIT)
                fatal("integer literal expected after '='");
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

static void typedef_declaration(void) {
    struct symtable *ctype;
    int type;
    int class = C_TYPEDEF;

    scan(&Token);

    type = parse_stars(parse_type(&ctype, &class));

    if (findtypedef(Text) != NULL)
        fatals("redefinition of typedef", Text);

    ident();

    addtypedef(Text, type, ctype);
}

static int type_of_typedef(struct symtable **ctype) {
    struct symtable *t;

    t = findtypedef(Text);

    if (t == NULL)
        fatals("unknown type", Text);

    *ctype = t->ctype;
    ident();
    return (t->type);
}

static int param_declaration_list(struct symtable *oldfuncsym) {
    struct symtable *protoptr = NULL;
    struct ASTnode *unused;
    int type;
    int paramcnt = 0;

    if (oldfuncsym != NULL)
        protoptr = oldfuncsym->member;

    while (Token.token != T_RPAREN) {
        if (Token.token == T_VOID) {
            scan(&Peektoken);

            if (Peektoken.token == T_RPAREN) {
                scan(&Token);
                break;
            }
        }
        type = declaration_list(C_PARAM, T_COMMA, T_RPAREN, &unused);
        if (type == -1)
            fatal("bad type in param list");

        if (protoptr != NULL) {
            if (protoptr->type != type)
                fatals("type doesn't match prototype for parameter", Text);
            if (strcmp(protoptr->name, Text))
                protoptr->name = strdup(Text);
            protoptr = protoptr->next;
        }

        paramcnt++;

        if (Token.token == T_RPAREN)
            break;

        comma();
        }

    if ((oldfuncsym != NULL) && (paramcnt != oldfuncsym->nelems))
        fatals("parameter count mismatch for function", oldfuncsym->name);

    return (paramcnt);
}

static struct symtable *function_declaration(int type, struct symtable *ctype) {
    struct ASTnode *tree = NULL, *finalstmt;
    struct symtable *oldfuncsym, *newfuncsym = NULL;
    int endlabel, paramcnt;

    if((oldfuncsym = findglob(Text)) != NULL)
        if (oldfuncsym->stype != S_FUNCTION)
            fatals("redeclared as different kind of symbol", Text);

    if (oldfuncsym == NULL) {
        endlabel = genlabel();
        newfuncsym = addglob(Text, type, ctype, S_FUNCTION, C_GLOBAL, 0, endlabel);
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

    if (Token.token == T_SEMI)
        return (oldfuncsym);

    Functionid = oldfuncsym;

    Looplevel = Switchlevel = 0;

    lbrace();
    tree = compound_statement(NO_SWITCH);
    rbrace();

    if (type != P_VOID) {
        if (tree == NULL)
            fatal("no statements in function with non-void type");

        finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
        if (finalstmt == NULL || finalstmt->op != A_RETURN)
            fatal("no return for function with non-void type");
    }
    tree = mkastunary(A_FUNCTION, type, tree, oldfuncsym, endlabel);

    tree = optimise(tree);

    genAST(tree, NOLABEL, NOLABEL, NOLABEL, 0);
    freeloclsyms();
    return (oldfuncsym);
}

static struct symtable *symbol_declaration(int type, struct symtable *ctype,
                                           int class, struct ASTnode **tree) {
    struct symtable *sym = NULL;
    char *varname = strdup(Text);
    
    ident();

    if (Token.token == T_LPAREN) {
        if (class != C_GLOBAL)
            fatal("function declaration or definition not at global level");
        return (function_declaration(type, ctype));
    }

    switch (class) {
        case C_GLOBAL:
        case C_EXTERN:
            if (findglob(varname) != NULL)
                fatals("duplicate global variable declaration", varname);
            break;
        case C_PARAM:
        case C_LOCAL:
            if (findlocl(varname) != NULL)
                fatals("duplicate local variable declaration", varname);
            break;
        case C_MEMBER:
            if (findmember(varname) != NULL)
                fatals("duplicate struct/union member declaration", varname);
            break;
    }
    if (Token.token == T_LBRACKET)
        sym = array_declaration(varname, type, ctype, class);
    else
        sym = scalar_declaration(varname, type, ctype, class, tree);

    return (sym);
}

int declaration_list(int class, int et1, int et2, struct ASTnode **gluetree) {
    struct symtable *ctype, *sym;
    int inittype, type;
    struct ASTnode *tree = NULL;
    *gluetree = NULL;

    if ((inittype = parse_type(&ctype, &class)) == -1)
        return (inittype);

    while (1) {
        type = parse_stars(inittype);

        sym = symbol_declaration(type, ctype, class, &tree);

        if (sym->stype == S_FUNCTION)
            return (type);

        if (*gluetree == NULL)
            *gluetree = tree;
        else
            *gluetree = mkastnode(A_GLUE, P_NONE, *gluetree, NULL, tree, NULL, 0);

        if (Token.token == et1 || Token.token == et2)
            return (type);

        comma();
    }

}

void global_declarations(void) {
    struct ASTnode *unused;
    while (Token.token != T_EOF) {
        declaration_list(C_GLOBAL, T_SEMI, T_EOF, &unused);
        if (Token.token == T_SEMI)
            semi();
    }
}