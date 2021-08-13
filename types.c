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

int inttype(int type) {
    return ((type & 0xf) == 0);
}

int ptrtype(int type) {
    return ((type & 0xf) != 0);
}

int pointer_to(int type) {
    if ((type & 0xf) == 0xf)
        fatald("Unrecognised in pointer_to(): type", type);
    return (type + 1);
}

int value_at(int type) {
    if ((type & 0xf) == 0x0)
        fatald("Unrecognised in value_at(): type", type);
    return (type - 1);
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
            if (rsize > 1)
                return (mkastunary(A_SCALE, rtype, tree, rsize));
            else
                return (tree);
        }
    }

    return (NULL);
}