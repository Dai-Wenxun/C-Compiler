#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define TEXTLEN 512
#define NSYMBOLS 1024

// Token types
enum {
  T_EOF,
  T_EQ, T_NE,
  T_LT, T_GT, T_LE, T_GE,
  T_PLUS, T_MINUS,
  T_STAR, T_SLASH,
  T_INTLIT, T_SEMI, T_ASSIGN, T_IDENT,
  // Keywords
  T_PRINT, T_INT
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
    A_INTLIT,
    A_IDENT, A_LVIDENT, A_ASSIGN
};

// Abstract Syntax Tree structure
struct ASTnode {
    int op;
    struct ASTnode *left;
    struct ASTnode *right;
    union {
        int intvalue;   // For A_INTLIT
        int id;         // For A_IDENT
    }v;
};

// Symbol table structure
struct symtable {
    char *name;
};
