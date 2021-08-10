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
void dumpAST(struct ASTnode *n, int label, int level);

// gen.c
int genlabel(void);
int genAST(struct ASTnode *n, int label, int parentASTop);
void genpreamble(void);
void genpostamble(void);
void genfreeregs(void);
void genglobsym(int id);
int genglobstr(char *strvalue);
int genprimsize(int type);
void genreturn(int r, int id);

// cg.c
void cgtextseg(void);
void cgdataseg(void);
void freeall_registers(void);
void cgpreamble(void);
void cgpostamble(void);
void cgfuncpreamble(int id);
void cgfuncpostamble(int id);
int cgstorlocal(int r, int id);
int cgstorglob(int r, int id);
int cgstorderef(int r1, int r2, int type);
int cgor(int r1, int r2);
int cgxor(int r1, int r2);
int cgand(int r1, int r2);
int cgcompare_and_jump(int ASTop, int r1, int r2, int label);
int cgcompare_and_set(int ASTop, int r1, int r2);
int cgshl(int r1, int r2);
int cgshr(int r1, int r2);
int cgadd(int r1, int r2);
int cgsub(int r1, int r2);
int cgmul(int r1, int r2);
int cgdiv(int r1, int r2);
int cgloadint(int value, int type);
int cgloadglobstr(int id);
int cgloadlocal(int id, int op);
int cgloadglob(int id, int op);
int cgnegate(int r);
int cginvert(int r);
int cglognot(int r);
int cgboolean(int r, int op, int label);
void cgreturn(int r, int id);
int cgcall(int r, int numargs);
void cgcopyarg(int r, int argposn);
int cgaddress(int id);
int cgderef(int r, int type);
int cgwiden(int r, int oldtype, int newtype);
int cgshlconst(int r, int val);
int cgprimsize(int type);
void cgglobsym(int id);
void cgglobstr(int l, char *strvalue);
void cglabel(int l);
void cgjump(int l);

// expr.c
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
int findlocl(char *s);
int findsymbol(char *s);
void copyfuncparams(int slot);
int addglob(char *name, int type, int stype, int class, int endlabel, int size);
int addlocl(char *name, int type, int stype, int isparam, int size);
void freeloclsyms(void);

// decl.c
void var_declaration(int type, int class);
struct ASTnode *function_declaration(int type);
void global_declarations(void);

// types.c
int inttype(int type);
int parse_type(void);
int pointer_to(int type);
int value_at(int type);
struct ASTnode *modify_type(struct ASTnode *tree, int rtype, int op);