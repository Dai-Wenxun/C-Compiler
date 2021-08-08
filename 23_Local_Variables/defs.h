#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define TEXTLEN 512
#define NSYMBOLS 1024

// Token types
enum {
  T_EOF,

  // Binary operators
  T_ASSIGN, T_LOGOR, T_LOGAND,
  T_OR, T_XOR, T_AMPER,
  T_EQ, T_NE,
  T_LT, T_GT, T_LE, T_GE,
  T_LSHIFT, T_RSHIFT,
  T_PLUS, T_MINUS,
  T_STAR, T_SLASH,

  // Other operators
  T_INC, T_DEC, T_INVERT, T_LOGNOT,

  // Type keywords
  T_VOID, T_CHAR, T_INT, T_LONG,

  // Other keywords
  T_IF, T_ELSE, T_WHILE, T_FOR, T_RETURN,

  // Structural tokens
  T_INTLIT, T_STRLIT, T_IDENT,
  T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN,
  T_LBRACKET, T_RBRACKET,
  T_SEMI, T_COMMA,
};

// Token structure
struct token {
  int token;
  int intvalue;
};

// AST node types
enum {
    A_ASSIGN=1, A_LOGOR, A_LOGAND, A_OR, A_XOR, A_AND,
    A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE, A_LSHIFT, A_RSHIFT,
    A_ADD, A_SUBTRACT, A_MULTIPLY, A_DIVIDE,
    A_PREINC, A_PREDEC, A_POSTINC, A_POSTDEC,
    A_NEGATE, A_INVERT, A_LOGNOT, A_TOBOOL,
    //
    A_IF, A_WHILE, A_GLUE, A_FUNCTION,
    //
    A_INTLIT, A_STRLIT, A_IDENT,
    //
    A_RETURN, A_FUNCCALL, A_ADDR, A_DEREF,
    A_WIDEN, A_SCALE,

};

// Primitive types
enum {
    P_NONE, P_VOID, P_CHAR, P_INT, P_LONG,
    P_VOIDPTR, P_CHARPTR, P_INTPTR, P_LONGPTR
};

// Abstract Syntax Tree structure
struct ASTnode {
    int op;
    int type;
    int rvalue;
    struct ASTnode *left;
    struct ASTnode *mid;
    struct ASTnode *right;
    union {
        int intvalue;
        int id;
        int size;
    }v;
};

#define NOREG -1
#define NOLABEL 0

// Structural types
enum {
    S_VARIABLE, S_FUNCTION, S_ARRAY
};

enum {
    C_GLOBAL = 1,
    C_LOCAL
};

// Symbol table structure
struct symtable {
    char *name;
    int type;
    int stype;
    int class;
    int endlabel;
    int size;
    int posn;
};
