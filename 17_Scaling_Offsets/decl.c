#include "defs.h"
#include "data.h"
#include "decl.h"

void var_declaration(int type) {
    int id;

    while (1) {
        id = addglob(Text, type, S_VARIABLE, 0);
        genglobsym(id);

        if (Token.token == T_SEMI)
            break;

        if (Token.token == T_COMMA) {
            scan(&Token);
            ident();
            continue;
        }
        fatal("Missing , or ; after identifier");
    }
    semi();
}

struct ASTnode *function_declaration(int type) {
    struct ASTnode *tree, *finalstmt;
    int nameslot, endlabel;

    endlabel = genlabel();
    nameslot = addglob(Text, type, S_FUNCTION, endlabel);
    Functionid = nameslot;

    lparen();
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
            genAST(tree, NOREG, 0);
        } else {
            var_declaration(type);
        }

        if (Token.token == T_EOF)
            break;
    }
}