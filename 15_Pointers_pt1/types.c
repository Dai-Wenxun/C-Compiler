#include "defs.h"
#include "data.h"
#include "decl.h"

int type_compatible(int *left, int *right, int onlyright) {
    int leftsize, rightsize;

    leftsize = genprimsize(*left);
    rightsize = genprimsize(*right);

    if (leftsize == 0 || rightsize == 0)
        return (0);

    if (*left == *right) {
        *left = *right = 0;
        return (1);
    }

    if (leftsize < rightsize) {
        *left = A_WIDEN;
        *right = 0;
        return (1);
    }
    if (rightsize < leftsize) {
        if (onlyright)
            return (0);
        *left = 0;
        *right = A_WIDEN;
        return (1);
    }
    *left = *right = 0;
    return (1);
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
            fatald("Unrecognised in pointer_to: type", type);
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
            fatald("Unrecognised in value_at: type", type);
    }
    return (newtype);
}