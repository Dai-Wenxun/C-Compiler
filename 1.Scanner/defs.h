#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

enum {
  T_PLUS, T_MINUS, T_STAR, T_SLASH, T_INTLIT
};

// Token structure
struct token {
  int token;
  int intvalue;
};
