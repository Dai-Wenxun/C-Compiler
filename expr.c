#include "defs.h"
#include "data.h"
#include "decl.h"

static struct ASTnode *expression_list(void) {
    struct ASTnode *tree = NULL;
    struct ASTnode *child = NULL;
    int exprcount = 0;

    while (Token.token != T_RPAREN) {
        child = binexpr(0);
        exprcount++;

        tree = mkastnode(A_GLUE, P_NONE, tree, NULL, child, NULL, exprcount);

        switch (Token.token) {
            case T_COMMA:
                scan(&Token);
                break;
            case T_RPAREN:
                break;
            default:
                fatald("Unexpected token in expression list", Token.token);
        }
    }

    return (tree);
}

static struct ASTnode *funccall(void) {
    struct ASTnode *tree;
    struct symtable *funcptr;

    if ((funcptr = findsymbol(Text)) == NULL || funcptr->stype != S_FUNCTION)
        fatals("Undeclared function", Text);
    lparen();

    tree = expression_list();

    tree = mkastunary(A_FUNCCALL, funcptr->type, tree, funcptr, 0);

    rparen();
    return (tree);
}

static struct ASTnode *array_access(void) {
    struct ASTnode *left, *right;
    struct symtable *aryptr;

    if ((aryptr = findsymbol(Text)) == NULL || aryptr->stype != S_ARRAY)
        fatals("Undeclared array", Text);

    left = mkastleaf(A_ADDR, aryptr->type, aryptr, 0);

    scan(&Token);

    right = binexpr(0);

    match(T_RBRACKET, "]");

    if (!inttype(right->type))
        fatal("Array index is not of integer type");

    right = modify_type(right, left->type, A_ADD);

    left = mkastnode(A_ADD, aryptr->type, left, NULL, right, NULL, 0);
    left = mkastunary(A_DEREF, value_at(left->type), left, NULL, 0);

    return (left);
}

static struct ASTnode *member_access(int withpointer) {
    struct ASTnode *left, *right;
    struct symtable *compvar;
    struct symtable *typeptr;
    struct symtable *m;

    if ((compvar = findsymbol(Text)) == NULL)
        fatals("Undeclared variable", Text);
    if (withpointer && compvar->type != pointer_to(P_STRUCT)
        && compvar->type != pointer_to(P_UNION))
        fatals("Undeclared variable", Text);
    if (!withpointer && compvar->type != P_STRUCT
        && compvar->type != P_UNION)
        fatals("Undeclared variable", Text);

    if (withpointer)
        left = mkastleaf(A_IDENT, compvar->type, compvar, 0);
    else
        left = mkastleaf(A_ADDR, compvar->type, compvar, 0);

    left->rvalue = 1;

    typeptr = compvar->ctype;

    scan(&Token);
    ident();

    for (m = typeptr->member; m != NULL; m = m->next)
        if (!strcmp(m->name, Text))
            break;

    if (m == NULL)
        fatals("No member found in struct/union: ", Text);

    right = mkastleaf(A_INTLIT, P_INT, NULL, m->posn);

    left = mkastnode(A_ADD, pointer_to(m->type), left, NULL, right, NULL, 0);
    left = mkastunary(A_DEREF, m->type, left, NULL, 0);
    return (left);
}

static struct ASTnode *postfix(void) {
    struct ASTnode *n;
    struct symtable *varptr;
    struct symtable *enumptr;

    if ((enumptr = findenumval(Text)) != NULL) {
        scan(&Token);
        return (mkastleaf(A_INTLIT, P_INT, NULL, enumptr->posn));
    }

    scan(&Token);

    if (Token.token == T_LPAREN)
        return (funccall());

    if (Token.token == T_LBRACKET)
        return (array_access());

    if (Token.token == T_DOT)
        return (member_access(0));
    if (Token.token == T_ARROW)
        return (member_access(1));

    if ((varptr = findsymbol(Text)) == NULL || varptr->stype != S_VARIABLE)
        fatals("Undeclared variable", Text);

    switch (Token.token) {
        case T_INC:
            scan(&Token);
            n = mkastleaf(A_POSTINC, varptr->type, varptr, 0);
            break;

        case T_DEC:
            scan(&Token);
            n = mkastleaf(A_POSTDEC, varptr->type, varptr, 0);
            break;

        default:
            n = mkastleaf(A_IDENT, varptr->type, varptr, 0);
    }

    return (n);
}

static struct ASTnode *primary(void) {
    struct ASTnode *n;
    int label;

    switch (Token.token) {
        case T_INTLIT:
            if (Token.intvalue >= 0 && Token.intvalue <= 255)
                n = mkastleaf(A_INTLIT, P_CHAR, NULL, Token.intvalue);
            else
                n = mkastleaf(A_INTLIT, P_INT, NULL, Token.intvalue);
            break;

        case T_STRLIT:
            label = genglobstr(Text);
            n = mkastleaf(A_STRLIT, pointer_to(P_CHAR), NULL, label);
            break;

        case T_IDENT:
            return (postfix());

        case T_LPAREN:
            scan(&Token);
            n = binexpr(0);
            rparen();
            return (n);

        default:
            fatald("Expecting a primary expression, got token", Token.token);
    }
    scan(&Token);
    return (n);
}

static int binastop(int tokentype) {
    if (tokentype > T_EOF && tokentype <= T_SLASH)
        return (tokentype);
    fatald("Syntax error, token", tokentype);
    return (0);
}

static int rightassoc(int tokentype) {
    if (tokentype == T_ASSIGN)
        return (1);
    return (0);
}

static int OpPrec[] = {
        0, 10, 20, 30,    // T_EOF, T_ASSIGN, T_LOGOR, T_LOGAND
        40, 50, 60,        // T_OR, T_XOR, T_AMPER,
        70, 70,             // T_EQ, T_NE
        80, 80, 80, 80,      // T_LT, T_GT, T_LE, T_GE
        90, 90,               // T_LSHIFT, T_RSHIFT
        100, 100,              // T_PLUS, T_MINUS
        110, 110                // T_STAR, T_SLASH
};

static int op_precedence(int tokentype) {
    int prec;
    if (tokentype > T_SLASH)
        fatald("Token with no precedence in op_precedence", tokentype);
    prec = OpPrec[tokentype];

    if (prec == 0)
        fatald("Syntax error, token", tokentype);

    return (prec);
}

struct ASTnode *prefix(void) {
    struct ASTnode *tree;
    switch (Token.token) {
        case T_AMPER:
            scan(&Token);
            tree = prefix();

            if (tree->op != A_IDENT)
                fatal("& operator must be followed by an identifier");

            tree->op = A_ADDR;
            tree->type = pointer_to(tree->type);
            break;
        case T_STAR:
            scan(&Token);
            tree = prefix();

            if (tree->op != A_IDENT && tree->op != A_DEREF)
                fatal("* operator must be followed by an identifier or *");

            tree = mkastunary(A_DEREF, value_at(tree->type), tree, NULL, 0);
            break;
        case T_MINUS:
            scan(&Token);
            tree = prefix();

            tree->rvalue = 1;
            tree = modify_type(tree, P_INT, 0);
            tree = mkastunary(A_NEGATE, tree->type, tree, NULL, 0);
            break;

        case T_INVERT:
            scan(&Token);
            tree = prefix();

            tree->rvalue = 1;
            tree = mkastunary(A_INVERT, tree->type, tree, NULL, 0);
            break;

        case T_LOGNOT:
            scan(&Token);
            tree = prefix();

            tree->rvalue = 1;
            tree = mkastunary(A_LOGNOT, tree->type, tree, NULL, 0);
            break;

        case T_INC:
            scan(&Token);
            tree = prefix();

            if (tree->op != A_IDENT)
                fatal("++ operator must be followed by an identifier");

            tree->op = A_PREINC;
            break;

        case T_DEC:
            scan(&Token);
            tree = prefix();

            if (tree->op != A_IDENT)
                fatal("-- operator must be followed by an identifier");

            tree->op = A_PREDEC;
            break;

        default:
            tree = primary();
    }

    return (tree);
}

struct ASTnode *binexpr(int ptp) {
    struct ASTnode *left, *right;
    struct ASTnode *ltemp, *rtemp;
    int ASTop;
    int tokentype;

    left = prefix();

    tokentype = Token.token;
    if (tokentype == T_SEMI || tokentype == T_RPAREN
        || tokentype == T_RBRACKET || tokentype == T_COMMA) {
        left->rvalue = 1;
        return (left);
    }

    while ((op_precedence(tokentype) > ptp) ||
            (rightassoc(tokentype) && op_precedence(tokentype) == ptp)) {

        scan(&Token);
        right = binexpr(OpPrec[tokentype]);

        ASTop = binastop(tokentype);

        if (ASTop == A_ASSIGN) {
            right->rvalue = 1;
            right = modify_type(right, left->type, 0);
            if (right == NULL)
                fatal("Incompatible type in assignment");

            ltemp = left;
            left = right;
            right = ltemp;

        } else {
            left->rvalue = 1;
            right->rvalue = 1;

            ltemp = modify_type(left, right->type, ASTop);
            rtemp = modify_type(right, left->type, ASTop);
            if (ltemp == NULL && rtemp == NULL)
                fatal("Incompatible types in binary expression");
            if (ltemp != NULL)
                left = ltemp;
            if (rtemp != NULL)
                right = rtemp;
        }

        left = mkastnode(binastop(tokentype), left->type, left, NULL, right, NULL, 0);

        tokentype = Token.token;
        if (tokentype == T_SEMI || tokentype == T_RPAREN
            || tokentype == T_RBRACKET || tokentype == T_COMMA) {
            left->rvalue = 1;
            return (left);
        }

    }

    left->rvalue= 1;
    return (left);
}