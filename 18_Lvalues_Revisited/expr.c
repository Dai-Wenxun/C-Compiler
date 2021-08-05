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
        case T_IDENT:
            scan(&Token);

            if (Token.token == T_LPAREN)
                return (funccall());
            reject_token(&Token);

            if ((id = findglob(Text)) == -1)
                fatals("Undeclared variable", Text);
            n = mkastleaf(A_IDENT, Gsym[id].type, id);
            break;

        default:
            fatald("Syntax error, token", Token.token);
    }
    scan(&Token);
    return (n);
}

static int binastop(int tokentype) {
    if (tokentype > T_EOF && tokentype < T_VOID)
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
        0, 10,            // T_EOF, T_ASSIGN
        20, 20,           // T_EQ, T_NE
        30, 30, 30, 30,   // T_LT, T_GT, T_LE, T_GE
        40, 40,           // T_PLUS, T_MINUS
        50, 50            // T_STAR, T_SLASH
};

static int op_precedence(int tokentype) {
    int prec = OpPrec[tokentype];
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
    if (tokentype == T_SEMI || tokentype == T_RPAREN) {
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

            ltemp = left; left = right; right = ltemp;
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
        if (tokentype == T_SEMI || tokentype == T_RPAREN)
            left->rvalue = 1; return (left);
    }

    left->rvalue= 1; return (left);
}