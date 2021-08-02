#include "defs.h"
#include "data.h"
#include "decl.h"

int parse_type(void) {
    int type;

    switch (Token.token) {
        case T_VOID:
            type = P_VOID;
            break;
        case T_CHAR:
            type = P_CHAR;
            break;
        case T_INT:
            type = P_INT;
            break;
        case T_LONG:
            type = P_LONG;
            break;
        default:
            fatald("Illegal type, token", Token.token);
    }

    while (1) {
        scan(&Token);
        if (Token.token != T_STAR)
            break;
        type = pointer_to(type);
    }

    return (type);
}

void var_declaration(void) {
    int id, type;

    type = parse_type();
    ident();

    id = addglob(Text, type, S_VARIABLE, 0);
    genglobsym(id);
    semi();
}

struct ASTnode *function_declaration(void) {
    struct ASTnode *tree, *finalstmt;
    int nameslot, type, endlabel;

    type = parse_type();
    ident();

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