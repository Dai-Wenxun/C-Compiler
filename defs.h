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
    C_EXTERN,
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
  T_VOID, T_CHAR, T_INT, T_LONG,    //26
  T_STRUCT, T_UNION, T_ENUM,    //29

  // Other keywords
  T_IF, T_ELSE, T_WHILE, T_FOR, T_RETURN,       //34
  T_TYPEDEF, T_EXTERN, T_BREAK, T_CONTINUE,     //38
  T_SWITCH, T_CASE, T_DEFAULT,              //41

  // Structural tokens
  T_INTLIT, T_STRLIT, T_IDENT,      //44
  T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN, T_LBRACKET, T_RBRACKET,   //50
  T_SEMI, T_COMMA, T_DOT, T_ARROW, T_COLON   //55
};

enum {
    A_ASSIGN=1, A_LOGOR, A_LOGAND, A_OR, A_XOR, A_AND,   //6
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
    A_WIDEN, A_SCALE, A_BREAK, A_CONTINUE,      //42
    A_CASE, A_DEFAULT                   //44
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