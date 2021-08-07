#include "defs.h"
#include "data.h"
#include "decl.h"

struct ASTnode *funccall(void) {
    struct ASTnode *tree;
    int id;

    if ((id = findglob(Text)) == -1)
        fatals("Undeclared function", Text);
    lparen();

    tree = binexpr(0);

    tree = mkastunary(A_FUNCCALL, Gsym[id].type, tree, id);

    rparen();
    return (tree);
}

static struct ASTnode *array_access(void) {
    struct ASTnode *left, *right;
    int id;

    if ((id = findglob(Text)) == -1 || Gsym[id].stype != S_ARRAY)
        fatals("Undeclared array", Text);

    left = mkastleaf(A_ADDR, Gsym[id].type, id);

    scan(&Token);

    right = binexpr(0);

    match(T_RBRACKET, "]");

    if (!inttype(right->type))
        fatal("Array index is not of integer type");

    right = modify_type(right, left->type, A_ADD);

    left = mkastnode(A_ADD, Gsym[id].type, left, NULL, right, 0);
    left = mkastunary(A_DEREF, value_at(left->type), left, 0);

    return (left);
}

static struct ASTnode *postfix(void) {
    struct ASTnode *n;
    int id;

    scan(&Token);

    if (Token.token == T_LPAREN)
        return (funccall());

    if (Token.token == T_LBRACKET)
        return (array_access());


    if ((id = findglob(Text)) == -1 || Gsym[id].stype != S_VARIABLE)
        fatals("Undeclared variable", Text);

    switch (Token.token) {
        case T_INC:
            scan(&Token);
            n = mkastleaf(A_POSTINC, Gsym[id].type, id);
            break;

        case T_DEC:
            scan(&Token);
            n = mkastleaf(A_POSTDEC, Gsym[id].type, id);
            break;

        default:
            n = mkastleaf(A_IDENT, Gsym[id].type, id);
    }

    return (n);
}

static struct ASTnode *primary(void) {
    struct ASTnode *n;
    int id;

    switch (Token.token) {
        case T_INTLIT:
            if (Token.intvalue >= 0 && Token.intvalue <= 255)
                n = mkastleaf(A_INTLIT, P_CHAR, Token.intvalue);
            else
                n = mkastleaf(A_INTLIT, P_INT, Token.intvalue);
            break;

        case T_STRLIT:
            id = genglobstr(Text);
            n = mkastleaf(A_STRLIT, P_CHARPTR, id);
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

            tree = mkastunary(A_DEREF, value_at(tree->type), tree, 0);
            break;
        case T_MINUS:
            scan(&Token);
            tree = prefix();

            tree->rvalue = 1;
            tree = modify_type(tree, P_INT, 0);
            tree = mkastunary(A_NEGATE, tree->type, tree, 0);
            break;

        case T_INVERT:
            scan(&Token);
            tree = prefix();

            tree->rvalue = 1;
            tree = mkastunary(A_INVERT, tree->type, tree, 0);
            break;

        case T_LOGNOT:
            scan(&Token);
            tree = prefix();

            tree->rvalue = 1;
            tree = mkastunary(A_LOGNOT, tree->type, tree, 0);
            break;

        case T_INC:
            scan(&Token);
            tree = prefix();

            if (tree->op != A_IDENT)
                fatal("++ operator must be followed by an identifier");

            tree = mkastunary(A_PREINC, tree->type, tree, 0);
            break;

        case T_DEC:
            scan(&Token);
            tree = prefix();

            if (tree->op != A_IDENT)
                fatal("-- operator must be followed by an identifier");

            tree = mkastunary(A_PREDEC, tree->type, tree, 0);
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
    if (tokentype == T_SEMI || tokentype == T_RPAREN || tokentype == T_RBRACKET) {
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

        left = mkastnode(binastop(tokentype), left->type, left, NULL, right, 0);

        tokentype = Token.token;
        if (tokentype == T_SEMI || tokentype == T_RPAREN || tokentype == T_RBRACKET) {
            left->rvalue = 1;
            return (left);
        }

    }

    left->rvalue= 1;
    return (left);
}