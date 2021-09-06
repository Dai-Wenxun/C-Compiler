#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

enum {
    NO_SWITCH = 1,
    IN_SWITCH,
    CBREAK,
    CNBREAK
};

enum {
    TEXTLEN = 512,
};

enum {
    NOREG = -1,
    NOLABEL = 0
};

enum {
    S_VARIABLE=1, S_FUNCTION, S_ARRAY
};

enum {
    C_GLOBAL = 1,
    C_EXTERN,
    C_STATIC,
    C_LOCAL,
    C_PARAM,
    C_STRUCT,
    C_UNION,
    C_MEMBER,
    C_ENUMTYPE,
    C_ENUMVAL,
    C_TYPEDEF
};

enum {
  T_EOF,

  // Binary operators
  T_ASSIGN, T_ASPLUS, T_ASMINUS, T_ASSTAR, T_ASSLASH,
  T_LOGOR, T_LOGAND,
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
  T_STRUCT, T_UNION, T_ENUM,

  // Other keywords
  T_IF, T_ELSE, T_WHILE, T_FOR, T_RETURN,
  T_TYPEDEF, T_EXTERN, T_BREAK, T_CONTINUE,
  T_SWITCH, T_CASE, T_DEFAULT, T_SIZEOF, T_STATIC,

  // Structural tokens
  T_INTLIT, T_STRLIT, T_IDENT,
  T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN, T_LBRACKET, T_RBRACKET,
  T_SEMI, T_COMMA, T_DOT, T_ARROW, T_COLON
};

enum {
    A_ASSIGN=1, A_ASPLUS, A_ASMINUS, A_ASSTAR, A_ASSLASH,
    A_LOGOR, A_LOGAND, A_OR, A_XOR, A_AND,   //6
    A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE, A_LSHIFT, A_RSHIFT,  //14
    A_ADD, A_SUBTRACT, A_MULTIPLY, A_DIVIDE,      //18
    A_PREINC, A_PREDEC, A_POSTINC, A_POSTDEC,       //22
    A_NEGATE, A_INVERT, A_LOGNOT, A_TOBOOL,         //26
    //
    A_IF, A_WHILE, A_SWITCH, A_FUNCCALL, A_GLUE, A_FUNCTION,    //32
    //
    A_INTLIT, A_STRLIT, A_IDENT,                //35
    //
    A_RETURN, A_ADDR, A_DEREF,              //38
    A_WIDEN, A_SCALE, A_CAST, A_BREAK, A_CONTINUE,      //42
    A_CASE, A_DEFAULT                   //44
};

enum {
    P_NONE, P_VOID=16, P_CHAR=32, P_INT=48, P_LONG=64,
    P_STRUCT=80, P_UNION=96
};

struct token {
    int token;
    int intvalue;
    char *tokptr;
};

struct symtable {
    char *name;
    int type;
    struct symtable *ctype;
    int stype;
    int class;
    int size;
    int nelems;
    union {
        int endlabel;
        int posn;
    };
    int *initlist;
    struct symtable *next;
    struct symtable *member;
};

struct ASTnode {
    int op;
    int type;
    int rvalue;
    struct ASTnode *left;
    struct ASTnode *mid;
    struct ASTnode *right;
    struct symtable *sym;
    union {
        int intvalue;
        int size;
    };
};