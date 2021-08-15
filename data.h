#ifndef extern_
#define extern_ extern
#endif


extern_ int Line;
extern_ int	Putback;
extern_ struct symtable *Functionid;
extern_ FILE *Infile;
extern_ FILE *Outfile;
extern_ char *Outfilename;
extern_ struct token Token;
extern_ char Text[TEXTLEN + 1];

extern_ struct symtable *Globhead, *Globtail;
extern_ struct symtable *Loclhead, *Locltail;
extern_ struct symtable *Parmhead, *Parmtail;
extern_ struct symtable *Membhead, *Membtail;
extern_ struct symtable *Structhead, *Structtail;
extern_ struct symtable *Unionhead, *Uniontal;


extern_ int O_dumpAST;
extern_ int O_keepasm;
extern_ int O_assemble;
extern_ int O_dolink;
extern_ int O_verbose;