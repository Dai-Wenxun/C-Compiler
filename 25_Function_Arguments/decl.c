#include "defs.h"
#include "data.h"
#include "decl.h"

void var_declaration(int type, int islocal, int isparam) {
    if (Token.token == T_LBRACKET) {
        scan(&Token);

        if (Token.token == T_INTLIT) {
            if (islocal) {
                fatals("Duplicate local variable declaration", Text);
            } else {
                addglob(Text, pointer_to(type), S_ARRAY, 0, Token.intvalue);
            }
        }

        scan(&Token);
        match(T_RBRACKET, "]");

    } else {
        if (islocal) {
            if (addlocl(Text, type, S_VARIABLE, isparam, 1) == -1)
                fatals("Duplicate local variable declaration", Text);
        } else {
            addglob(Text, type, S_VARIABLE, 0, 1);
        }
    }

}

static int param_declaration(void) {
    int type;
    int paramcnt = 0;

    while (Token.token != T_RPAREN) {
        type = parse_type();
        ident();
        var_declaration(type, 1, 1);
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

    return (paramcnt);
}

struct ASTnode *function_declaration(int type) {
    struct ASTnode *tree, *finalstmt;
    int nameslot, endlabel, paramcnt;

    endlabel = genlabel();
    nameslot = addglob(Text, type, S_FUNCTION, endlabel, 0);
    Functionid = nameslot;

    lparen();
    paramcnt = param_declaration();
    Symtable[nameslot].nelems = paramcnt;
    rparen();

    tree = compound_statement();

    if (type != P_VOID) {

        if (tree == NULL)
            fatal("No statements in function with non-void type");

        finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
        if (finalstmt == NULL || finalstmt->op != A_RETURN)
            fatal("No return for function with non-void type");
    }
    return (mkastunary(A_FUNCTION, type, tree, nameslot));
}

void global_declarations(void) {
    struct ASTnode *tree;
    int type;

    while (1) {
        type = parse_type();
        ident();

        if (Token.token == T_LPAREN) {
            tree = function_declaration(type);
            if (O_dumpAST) {
                dumpAST(tree, NOLABEL, 0);
                fprintf(stdout, "\n\n");
            }
            genAST(tree, NOLABEL, 0);

            freeloclsyms();
        } else {
            var_declaration(type, 0, 0);
            semi();
        }

        if (Token.token == T_EOF)
            break;
    }
}