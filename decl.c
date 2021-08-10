#include "defs.h"
#include "data.h"
#include "decl.h"

void var_declaration(int type, int class) {
    if (Token.token == T_LBRACKET) {
        scan(&Token);

        if (Token.token == T_INTLIT) {
            if (class == C_LOCAL) {
                fatal("For now, declaration of local arrays is not implemented");
            } else {
                addglob(Text, pointer_to(type), S_ARRAY, class, 0, Token.intvalue);
            }
        }

        scan(&Token);
        match(T_RBRACKET, "]");

    } else {
        if (class == C_LOCAL) {
            if (addlocl(Text, type, S_VARIABLE, class, 1) == -1)
                fatals("Duplicate local variable declaration", Text);
        } else {
            addglob(Text, type, S_VARIABLE, class, 0, 1);
        }
    }

}

static int param_declaration(int id) {
    int type, param_id;
    int orig_paramcnt;
    int paramcnt = 0;

    param_id = id + 1;

    if (param_id)
        orig_paramcnt = Symtable[id].nelems;

    while (Token.token != T_RPAREN) {
        type = parse_type();
        ident();

        if (param_id) {
            if (type != Symtable[id].type)
                fatald("Type doesn't match prototype for parameter", paramcnt + 1);
            param_id++;
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

    if ((id != -1) && (paramcnt != orig_paramcnt))
        fatals("Parameter count mismatch for function", Symtable[id].name);

    return (paramcnt);
}

struct ASTnode *function_declaration(int type) {
    struct ASTnode *tree, *finalstmt;
    int id;
    int nameslot, endlabel, paramcnt;

    if ((id = findsymbol(Text)) != -1)
        if (Symtable[id].stype != S_FUNCTION)
            id = -1;

    if (id == -1) {
        endlabel = genlabel();
        nameslot = addglob(Text, type, S_FUNCTION, C_GLOBAL, endlabel, 0);
    }

    lparen();
    paramcnt = param_declaration(id);
    rparen();

    if (id == -1)
        Symtable[nameslot].nelems = paramcnt;

    if (Token.token == T_SEMI) {
        scan(&Token);
        return (NULL);
    }

    if (id == -1)
        id = nameslot;
    copyfuncparams(id);

    Functionid = id;

    tree = compound_statement();

    if (type != P_VOID) {

        if (tree == NULL)
            fatal("No statements in function with non-void type");

        finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
        if (finalstmt == NULL || finalstmt->op != A_RETURN)
            fatal("No return for function with non-void type");
    }
    return (mkastunary(A_FUNCTION, type, tree, id));
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