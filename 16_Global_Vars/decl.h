// scan.c
void reject_token(struct token *t);
int scan(struct token *t);


// tree.c
struct ASTnode *mkastnode(int op, int type,
                        struct ASTnode *left,
                        struct ASTnode *mid,
                        struct ASTnode *right, int intvalue);
struct ASTnode *mkastleaf(int op, int type, int intvalue);
struct ASTnode *mkastunary(int op, int type, struct ASTnode *left, int intvalue);

// gen.c
int genlabel(void);
int genAST(struct ASTnode *n, int reg, int parentASTop);
void genpreamble();
void genfreeregs();
void genprintint(int reg);
void genglobsym(int id);
int genprimsize(int type);

// cg.c
void freeall_registers(void);
void cgpreamble();
void cgfuncpreamble(int id);
void cgfuncpostamble(int id);
int cgcompare_and_jump(int ASTop, int r1, int r2, int label);
int cgcompare_and_set(int ASTop, int r1, int r2);
int cgadd(int r1, int r2);
int cgsub(int r1, int r2);
int cgmul(int r1, int r2);
int cgdiv(int r1, int r2);
int cgloadint(int value, int type);
int cgloadglob(int id);
int cgstorglob(int r, int id);
void cgprintint(int r);
int cgwiden(int r, int oldtype, int newtype);
void cgreturn(int r, int id);
int cgcall(int r, int id);
int cgaddress(int id);
int cgderef(int r, int type);
int cgprimsize(int type);
void cgglobsym(int id);
void cglabel(int l);
void cgjump(int l);

// expr.c
struct ASTnode *funccall(void);
struct ASTnode *binexpr(int ptp);

// stmt.c
struct ASTnode *compound_statement(void);

// misc.c
void match(int t, char *what);
void semi(void);
void lbrace(void);
void rbrace(void);
void lparen(void);
void rparen(void);
void ident(void);
void fatal(char *s);
void fatals(char *s1, char *s2);
void fatald(char *s, int d);
void fatalc(char *s, int c);

// sym.c
int findglob(char *s);
int addglob(char *name, int type, int stype, int endlabel);

// decl.c
void var_declaration(int type);
struct ASTnode *function_declaration(int type);
void global_declarations(void);

// types.c
int parse_type(void);
int type_compatible(int *left, int *right, int onlyright);
int pointer_to(int type);
int value_at(int type);