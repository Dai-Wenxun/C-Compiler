#include "defs.h"
#include "data.h"
#include "decl.h"

struct symtable *var_declaration(int type, struct symtable *ctype, int class) {
    struct symtable *sym = NULL;

    switch (class) {
        case C_GLOBAL:
            if (findglob(Text) != NULL)
                fatals("Duplicate global variable declaration", Text);
        case C_LOCAL:
        case C_PARAM:
            if (findlocl(Text) != NULL)
                fatals("Duplicate local variable declaration", Text);
        case C_MEMBER:
            if (findmember(Text) != NULL)
                fatals("Duplicate struct/union member declaration", Text);
    }

    if (Token.token == T_LBRACKET) {
        scan(&Token);

        if (Token.token == T_INTLIT) {
            switch (class) {
                case C_GLOBAL:
                    sym = addglob(Text, pointer_to(type), ctype, S_ARRAY, Token.intvalue);
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
                sym = addglob(Text, type, ctype, S_VARIABLE, 1);
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
        type = parse_type(&ctype);
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
        newfuncsym = addglob(Text, type, NULL, S_FUNCTION, endlabel);
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

struct symtable *struct_declaration(void) {
    struct symtable *ctype = NULL;
    struct symtable *m;
    int offset;

    scan(&Token);

    if (Token.token == T_IDENT) {
        ctype = findstruct(Text);
        scan(&Token);
    }

    if (Token.token != T_LBRACE) {
        if (ctype == NULL)
            fatals("unknown struct type", Text);
        return (ctype);
    }

    if (ctype)
        fatals("previously defined struct", Text);

    ctype = addstruct(Text, P_STRUCT, NULL, 0, 0);
    scan(&Token);

    var_declaration_list(NULL, C_MEMBER, T_SEMI, T_LBRACE);
    rbrace();

    ctype->member = Membhead;
    Membhead = Membtail = NULL;

    m = ctype->member;
    m->posn = 0;

    offset = typesize(m->type, m->ctype);

    for (m = m->next; m != NULL; m = m->next) {
        m->posn = genalign(m->type, offset, 1);

        offset += typesize(m->type, m->ctype);
    }

    ctype->size = offset;
    return (ctype);
}

void global_declarations(void) {
    struct ASTnode *tree;
    struct symtable *ctype;
    int type;

    while (1) {
        if (Token.token == T_EOF)
            break;

        type = parse_type(&ctype);
        if (type == P_STRUCT && Token.token == T_SEMI) {
            scan(&Token);
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
            var_declaration(type, ctype, C_GLOBAL);
            semi();
        }
    }
}