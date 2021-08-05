#include "defs.h"
#include "data.h"
#include "decl.h"

int inttype(int type) {
    if (type == P_CHAR || type == P_INT || type == P_LONG)
        return (1);
    return (0);
}

int ptrtype(int type) {
    if (type == P_VOIDPTR || type == P_CHARPTR ||
        type == P_INTPTR || type == P_LONGPTR)
        return (1);
    return (0);
}

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

int pointer_to(int type) {
    int newtype;
    switch (type) {
        case P_VOID:
            newtype = P_VOIDPTR;
            break;
        case P_CHAR:
            newtype = P_CHARPTR;
            break;
        case P_INT:
            newtype = P_INTPTR;
            break;
        case P_LONG:
            newtype = P_LONGPTR;
            break;
        default:
            fatald("Unrecognised in pointer_to(): type", type);
    }
    return (newtype);
}

int value_at(int type) {
    int newtype;
    switch (type) {
        case P_VOIDPTR:
            newtype = P_VOID;
            break;
        case P_CHARPTR:
            newtype = P_CHAR;
            break;
        case P_INTPTR:
            newtype = P_INT;
            break;
        case P_LONGPTR:
            newtype = P_LONG;
            break;
        default:
            fatald("Unrecognised in value_at(): type", type);
    }
    return (newtype);
}

struct ASTnode *modify_type(struct ASTnode *tree, int rtype, int op) {
    int ltype;
    int lsize, rsize;

    ltype = tree->type;

    if (inttype(ltype) && inttype(rtype)) {
        if (ltype == rtype)
            return (tree);

        lsize = genprimsize(ltype);
        rsize = genprimsize(rtype);

        if (lsize < rsize)
            return (mkastunary(A_WIDEN, rtype, tree, 0));

        if (lsize > rsize)
            return (NULL);
    }

    if (ptrtype(ltype)) {
        if (op == 0 && ltype == rtype)
            return (tree);
    }

    if (op == A_ADD || op == A_SUBTRACT) {
        if (inttype(ltype) && ptrtype(rtype)) {
            rsize = genprimsize(value_at(rtype));
            return (mkastunary(A_SCALE, rtype, tree, rsize));
        }
    }

    return (NULL);
}