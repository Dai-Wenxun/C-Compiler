#include "defs.h"
#include "data.h"
#include "decl.h"

void var_declaration(void) {
    match(T_INT, "int");
    ident();
    addglob(Text);
    genglobsym(Text);
    semi();
}