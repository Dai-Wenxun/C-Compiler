#include "defs.h"
#include "data.h"
#include "decl.h"

int type_compatible(int *left, int *right, int onlyright) {
    if (*left == P_VOID || *right == P_VOID)
        return 0;

    if (*left == *right) {
        *left = *right = 0;
        return 1;
    }

    if (*left == P_CHAR && *right == P_INT) {
        *left = A_WIDEN;
        *right = 0;
        return 1;
    }
    if (*left == P_INT && *right == P_CHAR) {
        if (onlyright)
            return 0;
        *left = 0;
        *right = A_WIDEN;
        return 1;
    }
    *left = *right = 0;
    return 1;
}