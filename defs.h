#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define AOUT "a.out"
#define ASCMD "as -o"
#define LDCMD "cc -o"
#define CPPCMD "cpp -nostdinc -isystem"
#define INCDIR "/tmp/include"

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
    C_LOCAL,
    C_PARAM,
    C_EXTERN,
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
  T_ASSIGN, T_LOGOR, T_LOGAND,  //3
  T_OR, T_XOR, T_AMPER,         //6
  T_EQ, T_NE,                   //8
  T_LT, T_GT, T_LE, T_GE,       //12
  T_LSHIFT, T_RSHIFT,           //14
  T_PLUS, T_MINUS,              //16
  T_STAR, T_SLASH,              //18

  // Other operators
  T_INC, T_DEC, T_INVERT, T_LOGNOT,  //22

  // Type keywords
  T_VOID, T_CHAR, T_INT, T_LONG, T_STRUCT, T_UNION, T_ENUM,    //29

  // Other keywords
  T_IF, T_ELSE, T_WHILE, T_FOR, T_RETURN, T_TYPEDEF,        //35

  // Structural tokens
  T_INTLIT, T_STRLIT, T_IDENT,      //38
  T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN, T_LBRACKET, T_RBRACKET,   //44
  T_SEMI, T_COMMA, T_DOT, T_ARROW       //48
};

enum {
    A_ASSIGN=1, A_LOGOR, A_LOGAND, A_OR, A_XOR, A_AND,
    A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE, A_LSHIFT, A_RSHIFT,
    A_ADD, A_SUBTRACT, A_MULTIPLY, A_DIVIDE,
    A_PREINC, A_PREDEC, A_POSTINC, A_POSTDEC,
    A_NEGATE, A_INVERT, A_LOGNOT, A_TOBOOL,
    //
    A_IF, A_WHILE, A_FUNCCALL, A_GLUE, A_FUNCTION,
    //
    A_INTLIT, A_STRLIT, A_IDENT,
    //
    A_RETURN, A_ADDR, A_DEREF,
    A_WIDEN, A_SCALE,
};

enum {
    P_NONE, P_VOID=16, P_CHAR=32, P_INT=48, P_LONG=64,
    P_STRUCT=80, P_UNION=96
};

struct token {
    int token;
    int intvalue;
};

struct symtable {
    char *name;
    int type;
    struct symtable *ctype;
    int stype;
    int class;
    union {
        int size;
        int endlabel;
    };
    union {
        int posn;
        int nelems;
    };
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