#include "defs.h"
#include "data.h"
#include "decl.h"

struct symtable *var_declaration(int type, int class) {
    struct symtable *sym = NULL;

    switch (class) {
        case C_GLOBAL:
            if (findglob(Text) != NULL)
                fatals("Duplicate global variable declaration", Text);
        case C_LOCAL:
        case C_PARAM:
            if (findlocl(Text) != NULL)
                fatals("Duplicate local variable declaration", Text);
    }

    if (Token.token == T_LBRACKET) {
        scan(&Token);

        if (Token.token == T_INTLIT) {
            switch (class) {
                case C_GLOBAL:
                    sym = addglob(Text, pointer_to(type), S_ARRAY, class, Token.intvalue);
                    break;
                case C_LOCAL:
                case C_PARAM:
                    fatal("For now, declaration of local arrays is not implemented");
            }
        }

        scan(&Token);
        match(T_RBRACKET, "]");

    } else {
        switch (class) {
            case C_GLOBAL:
                sym = addglob(Text, type, S_VARIABLE, class, 1);
                break;
            case C_LOCAL:
                sym = addlocl(Text, type, S_VARIABLE, class, 1);
                break;
            case C_PARAM:
                sym = addparm(Text, type, S_VARIABLE, class, 1);
                break;
        }
    }
    return (sym);
}

static int param_declaration(struct symtable *funcsym) {
    int type;
    int paramcnt = 0;
    struct symtable *protoptr = NULL;

    if (funcsym != NULL)
        protoptr = funcsym->member;

    while (Token.token != T_RPAREN) {
        type = parse_type();
        ident();

        if (protoptr != NULL) {
            if (type != protoptr->type)
                fatald("Type doesn't match prototype for parameter", paramcnt + 1);
            protoptr = protoptr->next;
        } else {
            var_declaration(type, C_PARAM);
        }

        paramcnt++;

        switch (Token.token) {
            case T_COMMA:
                scan(&Token);
                break;
            case T_RPAREN:
                break;
            default:
                fatald("Unexpected token in parameter list", Token.token);
        }
    }

    if ((funcsym != NULL) && (paramcnt != funcsym->nelems))
        fatals("Parameter count mismatch for function",funcsym->name);

    return (paramcnt);
}

struct ASTnode *function_declaration(int type) {
    struct ASTnode *tree, *finalstmt;
    struct symtable *oldfuncsym, *newfuncsym = NULL;
    int endlabel, paramcnt;

    if ((oldfuncsym = findsymbol(Text)) != NULL)
        if (oldfuncsym->stype != S_FUNCTION)
            oldfuncsym = NULL;

    if (oldfuncsym == NULL) {
        endlabel = genlabel();
        newfuncsym = addglob(Text, type, S_FUNCTION, C_GLOBAL, endlabel);
    }

    lparen();
    paramcnt = param_declaration(oldfuncsym);
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

void global_declarations(void) {
    struct ASTnode *tree;
    int type;

    while (1) {
        type = parse_type();
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
            var_declaration(type, C_GLOBAL);
            semi();
        }

        if (Token.token == T_EOF)
            break;
    }
}