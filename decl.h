// scan.c
int scan(struct token *t);


// tree.c
struct ASTnode *mkastnode(int op, int type,
                struct ASTnode *left, struct ASTnode *mid, struct ASTnode *right,
                struct symtable *sym, int intvalue);
struct ASTnode *mkastleaf(int op, int type,
                struct symtable *sym, int intvalue);
struct ASTnode *mkastunary(int op, int type, struct ASTnode *left,
                struct symtable *sym, int intvalue);
void dumpAST(struct ASTnode *n, int label, int level);

// gen.c
int genlabel(void);
int genAST(struct ASTnode *n, int label, int parentASTop);
void genpreamble(void);
void genpostamble(void);
void genfreeregs(void);
void genglobsym(struct symtable *node);
int genglobstr(char *strvalue);
int genprimsize(int type);
int genalign(int type, int offset, int direction);


// cg.c
void cgtextseg(void);
void cgdataseg(void);
void freeall_registers(void);
void cgpreamble(void);
void cgpostamble(void);
void cgfuncpreamble(struct symtable *sym);
void cgfuncpostamble(struct symtable *sym);
int cgstorglob(int r, struct symtable *sym);
int cgstorlocal(int r, struct symtable *sym);
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
int cgloadglobstr(int label);
int cgloadglob(struct symtable* sym, int op);
int cgloadlocal(struct symtable* sym, int op);
int cgnegate(int r);
int cginvert(int r);
int cglognot(int r);
int cgboolean(int r, int op, int label);
void cgreturn(int r, struct symtable *sym);     //verified
int cgcall(struct symtable *sym, int numargs);  //verified
void cgcopyarg(int r, int argposn);             //verified
int cgaddress(struct symtable *sym);            //verified
int cgderef(int r, int type);                   //verified
int cgwiden(int r, int oldtype, int newtype);
int cgshlconst(int r, int val);
void cgglobsym(struct symtable *node);
void cgglobstr(int l, char *strvalue);
int cgprimsize(int type);
int cglign(int type, int offset, int direction);
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
void comma(void);
void fatal(char *s);
void fatals(char *s1, char *s2);
void fatald(char *s, int d);
void fatalc(char *s, int c);

// sym.c
struct symtable *addglob(char *name, int type, struct symtable *ctype,
                int stype, int size);
struct symtable *addlocl(char *name, int type, struct symtable *ctype,
                int stype, int size) ;
struct symtable *addparm(char *name, int type, struct symtable *ctype,
                int stype, int size);
struct symtable *addmemb(char *name, int type, struct symtable *ctype,
                int stype, int size);
struct symtable *addstruct(char *name, int type, struct symtable *ctype,
                int stype, int size);
struct symtable *addunion(char *name, int type, struct symtable *ctype,
                int stype, int size);
struct symtable *addenum(char *name, int class, int value);
struct symtable *addtypedef(char *name, int type, struct symtable *ctype,
                int stype, int size);
struct symtable *findglob(char *s);
struct symtable *findlocl(char *s);
struct symtable *findsymbol(char *s);
struct symtable *findmember(char *s);
struct symtable *findstruct(char *s);
struct symtable *findunion(char *s);
struct symtable *findenumtype(char *s);
struct symtable *findenumval(char *s);
struct symtable *findtypedef(char *s);
void clear_symtable(void);
void freeloclsyms(void);


// decl.c
int parse_type(struct symtable **ctype);
struct symtable *var_declaration(int type, struct symtable *ctype, int class);
void global_declarations(void);

// types.c
int inttype(int type);
int ptrtype(int type);
int pointer_to(int type);
int value_at(int type);
int typesize(int type, struct symtable *ctype);
struct ASTnode *modify_type(struct ASTnode *tree, int rtype, int op);