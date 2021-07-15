#include "data.h"
#include "defs.h"
#include "decl.h"

void match(int t, char *what) {
    if (Token.token == t) {
        scan(&Token);
    } else {
        fprintf(stderr, "%s expected on line %d\n", what, Line);
        exit(1);
    }
}

void semi(void) {
    match(T_SEMI, ";");
}