#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define TEXTLEN 512
#define NSYMBOLS 1024

// Token types
enum {
  T_EOF,
  // Operators
  T_EQ, T_NE,
  T_LT, T_GT, T_LE, T_GE,
  T_PLUS, T_MINUS,
  T_STAR, T_SLASH,
  // Type keywords
  T_VOID, T_CHAR, T_INT, T_LONG,
  // Structural tokens
  T_INTLIT, T_SEMI, T_ASSIGN, T_IDENT,
  T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN,
  T_AMPER, T_LOGAND, T_COMMA,
  // Other keywords
  T_PRINT, T_IF, T_ELSE, T_WHILE, T_FOR, T_RETURN
};

// Token structure
struct token {
  int token;
  int intvalue;
};

// AST node types
enum {
    A_EQ=1, A_NE,
    A_LT, A_GT, A_LE, A_GE,
    A_ADD, A_SUBTRACT,
    A_MULTIPLY, A_DIVIDE,
    //
    A_IF, A_WHILE, A_GLUE, A_FUNCTION,
    //
    A_INTLIT, A_IDENT, A_LVIDENT, A_ASSIGN,
    A_PRINT, A_WIDEN, A_RETURN, A_FUNCCALL,
    A_ADDR, A_DEREF

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
    struct ASTnode *left;
    struct ASTnode *mid;
    struct ASTnode *right;
    union {
        int intvalue;
        int id;
    }v;
};

#define NOREG -1

// Structural types
enum {
    S_VARIABLE, S_FUNCTION
};

// Symbol table structure
struct symtable {
    char *name;
    int type;
    int stype;
    int endlabel;
};