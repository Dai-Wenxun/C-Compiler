#include "defs.h"
#include "data.h"
#include "decl.h"


int inttype(int type) {
    return (((type & 0xf) == 0) && (type >= P_CHAR && type <= P_LONG));
}

int ptrtype(int type) {
    return ((type & 0xf) != 0);
}

int pointer_to(int type) {
    if ((type & 0xf) == 0xf)
        fatald("unrecognised in pointer_to(): type", type);
    return (type + 1);
}

int value_at(int type) {
    if ((type & 0xf) == 0x0)
        fatald("unrecognised in value_at(): type", type);
    return (type - 1);
}

int typesize(int type, struct symtable *ctype) {
    if (type == P_STRUCT || type == P_UNION)
        return (ctype->size);
    return (genprimsize(type));
}

struct ASTnode *modify_type(struct ASTnode *tree, int rtype, int op) {
    int ltype;
    int lsize, rsize;

    ltype = tree->type;

    if (ltype == P_STRUCT || ltype == P_UNION)
        fatal("Don't know how to do this yet");
    if (rtype == P_STRUCT || rtype == P_UNION)
        fatal("Don't know how to do this yet");

    if (inttype(ltype) && inttype(rtype)) {
        if (ltype == rtype)
            return (tree);

        lsize = genprimsize(ltype);
        rsize = genprimsize(rtype);

        if (lsize < rsize)
            return (mkastunary(A_WIDEN, rtype, tree, NULL, 0));

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
                return (mkastunary(A_SCALE, rtype, tree, NULL, rsize));
            else
                return (tree);
        }
    }

    return (NULL);
}