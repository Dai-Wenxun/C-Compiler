#include "defs.h"
#include "data.h"
#include "decl.h"

struct ASTnode *expression_list(int endtoken) {
    struct ASTnode *tree = NULL;
    struct ASTnode *child = NULL;
    int exprcount = 0;

    while (Token.token != endtoken) {
        child = binexpr(0);
        exprcount++;

        tree = mkastnode(A_GLUE, P_NONE, tree, NULL, child, NULL, exprcount);

        if (Token.token == endtoken) break;

        comma();
    }

    return (tree);
}

static struct ASTnode *funccall(void) {
    struct ASTnode *tree;
    struct symtable *funcptr;

    if ((funcptr = findsymbol(Text)) == NULL || funcptr->stype != S_FUNCTION)
        fatals("undeclared function", Text);
    lparen();

    tree = expression_list(T_RPAREN);

    tree = mkastunary(A_FUNCCALL, funcptr->type, tree, funcptr, 0);

    rparen();
    return (tree);
}

static struct ASTnode *array_access(void) {
    struct ASTnode *left, *right;
    struct symtable *aryptr;

    if ((aryptr = findsymbol(Text)) == NULL || aryptr->stype != S_ARRAY)
        fatals("undeclared array", Text);

    left = mkastleaf(A_ADDR, aryptr->type, aryptr, 0);

    scan(&Token);

    right = binexpr(0);

    match(T_RBRACKET, "]");

    if (!inttype(right->type))
        fatal("array index is not of integer type");

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
        fatals("undeclared variable", Text);
    if (withpointer && compvar->type != pointer_to(P_STRUCT)
        && compvar->type != pointer_to(P_UNION))
        fatals("undeclared variable", Text);
    if (!withpointer && compvar->type != P_STRUCT
        && compvar->type != P_UNION)
        fatals("undeclared variable", Text);

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
        fatals("no member found in struct/union: ", Text);

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
        fatals("undeclared variable", Text);

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
    int type = 0;
    int size, class = 0;
    struct symtable *ctype;

    switch (Token.token) {
        case T_INTLIT:
            if (Token.intvalue >= -128 && Token.intvalue <= 255)
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

            switch (Token.token) {
                case T_IDENT:
                    if (findtypedef(Text) == NULL) {
                        n = binexpr(0); break;
                    }
                case T_VOID:
                case T_CHAR:
                case T_INT:
                case T_LONG:
                case T_STRUCT:
                case T_UNION:
                case T_ENUM:
                    type = parse_cast();
                    rparen();
                default:
                    n = binexpr(0);
            }

            if (type == 0)
                rparen();
            else
                n = mkastunary(A_CAST, type, n, NULL, 0);

            return (n);

        case T_SIZEOF:
            scan(&Token);
            lparen();

            type = parse_stars(parse_type(&ctype, &class));

            size = typesize(type, ctype);

            n = mkastleaf(A_INTLIT, P_INT, NULL, size);
            break;

        default:
            fatals("primary expression expected, got token", Token.tokptr);
    }
    scan(&Token);
    return (n);
}

static int rightassoc(int tokentype) {
    if (tokentype >= T_ASSIGN && tokentype <= T_ASSLASH)
        return (1);
    return (0);
}

static int OpPrec[] = {
        0, 10, 10, 10, 10, 10,         // T_EOF, T_ASSIGN~
        15, 20, 30,                     // T_QUESTION, T_LOGOR, T_LOGAND
        40, 50, 60,                      // T_OR, T_XOR, T_AMPER
        70, 70,                           // T_EQ, T_NE
        80, 80, 80, 80,                    // T_LT, T_GT, T_LE, T_GE
        90, 90,                             // T_LSHIFT, T_RSHIFT
        100, 100,                            // T_PLUS, T_MINUS
        110, 110                              // T_STAR, T_SLASH
};

static int op_precedence(struct token *t, int *tokentype) {
    int prec;
    if (t->token > T_SLASH || t->token == T_EOF) {
        if (Token.token == T_INTLIT && Token.intvalue < 0) {
            dealing_minus(t);
            *tokentype = T_MINUS;
            return (OpPrec[T_MINUS]);
        }
        fatals("binary operator required, got", t->tokptr);
    }
    prec = OpPrec[t->token];

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
    int tokentype, ASTop, oprec;

    left = prefix();

    tokentype = Token.token;
    if (tokentype == T_SEMI || tokentype == T_RPAREN
        || tokentype == T_RBRACKET || tokentype == T_COMMA
        || tokentype == T_COLON || tokentype == T_RBRACE) {
        left->rvalue = 1;
        return (left);
    }

    while ((op_precedence(&Token, &tokentype) > ptp) ||
            (rightassoc(tokentype) && op_precedence(&Token, &tokentype) == ptp)) {

        oprec = OpPrec[tokentype];
        if (Token.token == T_QUESTION)
            oprec = 0;
        scan(&Token);
        right = binexpr(oprec);

        ASTop = tokentype;
        switch (ASTop) {
            case A_ASPLUS:
            case A_ASMINUS:
            case A_ASSTAR:
            case A_ASSLASH:
            case A_ASSIGN:
                right->rvalue = 1;
                right = modify_type(right, left->type, 0);
                if (right == NULL)
                    fatal("incompatible type in assignment");

                ltemp = left;
                left = right;
                right = ltemp;
                break;
            case A_TERNARY:
                match(T_COLON, ":");
                ltemp = binexpr(0);

                if (left->op < A_EQ || left->op > A_GE)
                    left = mkastunary(A_TOBOOL, left->type, left, NULL, 0);

                return (mkastnode(A_TERNARY, right->type, left, right, ltemp, NULL, 0));

            default:
                left->rvalue = 1;
                right->rvalue = 1;
                ltemp = modify_type(left, right->type, ASTop);
                rtemp = modify_type(right, left->type, ASTop);
                if (ltemp == NULL && rtemp == NULL)
                    fatal("incompatible types in binary expression");
                if (ltemp != NULL)
                    left = ltemp;
                if (rtemp != NULL)
                    right = rtemp;
        }

        left = mkastnode(ASTop, left->type, left, NULL, right, NULL, 0);

        tokentype = Token.token;
        if (tokentype == T_SEMI || tokentype == T_RPAREN
            || tokentype == T_RBRACKET || tokentype == T_COMMA
            || tokentype == T_COLON || tokentype == T_RBRACE) {
            left->rvalue = 1;
            return (left);
        }

    }

    left->rvalue= 1;
    return (left);
}