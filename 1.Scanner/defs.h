//
// Created by 戴文勋 on 2021/7/7.
//

#ifndef C_COMPILER_DEFS_H
#define C_COMPILER_DEFS_H

#endif //C_COMPILER_DEFS_H

enum {
    T_PLUS, T_MINUS, T_STAR, T_INTLIT
};

struct token {
    int token;
    int intvalue;
};