#include "defs.h"
#include "data.h"
#include "decl.h"

static struct symtable *composite_declaration(int type);
static void enum_declaration(void);
int typedef_declaration(struct symtable **ctype);
int type_of_typedef(char *name, struct symtable **ctype);

int parse_type(struct symtable **ctype, int *class) {
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

    while (1) {
        if (Token.token != T_STAR)
            break;
        type = pointer_to(type);
        scan(&Token);
    }

    return (type);
}

struct symtable *var_declaration(int type, struct symtable *ctype, int class) {
    struct symtable *sym = NULL;

    switch (class) {
        case C_GLOBAL:
        case C_EXTERN:
            if (findglob(Text) != NULL)
                fatals("Duplicate global variable declaration", Text);
            break;
        case C_LOCAL:
        case C_PARAM:
            if (findlocl(Text) != NULL)
                fatals("Duplicate local variable declaration", Text);
            break;
        case C_MEMBER:
            if (findmember(Text) != NULL)
                fatals("Duplicate struct/union member declaration", Text);
            break;
    }

    if (Token.token == T_LBRACKET) {
        scan(&Token);

        if (Token.token == T_INTLIT) {
            switch (class) {
                case C_GLOBAL:
                case C_EXTERN:
                    sym = addglob(Text, pointer_to(type), ctype, S_ARRAY, class, Token.intvalue);
                    break;
                case C_LOCAL:
                case C_PARAM:
                case C_MEMBER:
                    fatal("For now, declaration of non-global arrays is not implemented");
            }
        }

        scan(&Token);
        match(T_RBRACKET, "]");

    } else {
        switch (class) {
            case C_GLOBAL:
            case C_EXTERN:
                sym = addglob(Text, type, ctype, S_VARIABLE, class, 1);
                break;
            case C_LOCAL:
                sym = addlocl(Text, type, ctype, S_VARIABLE, 1);
                break;
            case C_PARAM:
                sym = addparm(Text, type, ctype, S_VARIABLE,  1);
                break;
            case C_MEMBER:
                sym = addmemb(Text, type, ctype, S_VARIABLE, 1);
                break;
        }
    }
    return (sym);
}

static int var_declaration_list(struct symtable *funcsym, int class,
                        int separate_token, int end_token) {
    int type;
    int paramcnt = 0;
    struct symtable *protoptr = NULL;
    struct symtable *ctype;

    if (funcsym != NULL)
        protoptr = funcsym->member;

    while (Token.token != end_token) {
        type = parse_type(&ctype, &class);
        ident();

        if (protoptr != NULL) {
            if (type != protoptr->type)
                fatald("Type doesn't match prototype for parameter", paramcnt + 1);
            protoptr = protoptr->next;
        } else {
            var_declaration(type, ctype, class);
        }

        paramcnt++;

        if ((Token.token != separate_token) && (Token.token != end_token))
            fatald("Unexpected token in parameter list", Token.token);
        if (Token.token == separate_token)
            scan(&Token);
    }

    if ((funcsym != NULL) && (paramcnt != funcsym->nelems))
        fatals("Parameter count mismatch for function",funcsym->name);

    return (paramcnt);
}

static struct ASTnode *function_declaration(int type) {
    struct ASTnode *tree, *finalstmt;
    struct symtable *oldfuncsym, *newfuncsym = NULL;
    int endlabel, paramcnt;

    if ((oldfuncsym = findsymbol(Text)) != NULL)
        if (oldfuncsym->stype != S_FUNCTION)
            oldfuncsym = NULL;

    if (oldfuncsym == NULL) {
        endlabel = genlabel();
        newfuncsym = addglob(Text, type, NULL, S_FUNCTION, C_GLOBAL, endlabel);
    }

    lparen();
    paramcnt = var_declaration_list(oldfuncsym, C_PARAM, T_COMMA, T_RPAREN);
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

    tree = compound_statement();

    if (type != P_VOID) {

        if (tree == NULL)
            fatal("No statements in function with non-void type");

        finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
        if (finalstmt == NULL || finalstmt->op != A_RETURN)
            fatal("No return for function with non-void type");
    }
    return (mkastunary(A_FUNCTION, type, tree, oldfuncsym, endlabel));
}

struct symtable *composite_declaration(int type) {
    struct symtable *ctype = NULL;
    struct symtable *m;
    int offset;

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
        ctype = addstruct(Text, P_STRUCT, NULL, 0, 0);
    else
        ctype = addunion(Text, P_UNION, NULL, 0, 0);

    scan(&Token);

    var_declaration_list(NULL, C_MEMBER, T_SEMI, T_RBRACE);
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

int typedef_declaration(struct symtable **ctype) {
    int type, class = 0;

    scan(&Token);

    type = parse_type(ctype, &class);

    if (findtypedef(Text) != NULL)
        fatals("redefinition of typedef", Text);
    addtypedef(Text, type, *ctype);
    scan(&Token);
    return (type);
}

int type_of_typedef(char *name, struct symtable **ctype) {
    struct symtable *t;

    t = findtypedef(name);
    if (t == NULL)
        fatals("unknown type", name);

    scan(&Token);
    *ctype = t->ctype;
    return (t->type);
}

void global_declarations(void) {
    struct ASTnode *tree;
    struct symtable *ctype;
    int type, class = C_GLOBAL;

    while (1) {
        if (Token.token == T_EOF)
            break;

        type = parse_type(&ctype, &class);
        if (type == -1) {
            semi();
            continue;
        }

        ident();
        if (Token.token == T_LPAREN) {
            tree = function_declaration(type);

            if (tree == NULL)
                continue;

            if (O_dumpAST) {
                dumpAST(tree, NOLABEL, 0);
                fprintf(stdout, "\n\n");
            }
            genAST(tree, NOLABEL, 0);

            freeloclsyms();
        } else {
            var_declaration(type, ctype, class);
            semi();
        }
    }
}