#ifndef extern_
#define extern_ extern
#endif


extern_ int Line;
extern_ int	Putback;
extern_ struct symtable *Functionid;
extern_ FILE *Infile;
extern_ FILE *Outfile;
extern_ char *Infilename;
extern_ char *Outfilename;
extern_ struct token Token;
extern_ struct token Peektoken;
extern_ char Text[TEXTLEN + 1];
extern_ int Looplevel;
extern_ int Switchlevel;
extern_ int Switchexist;
extern_ int Casebreak;


extern_ struct symtable *Globhead, *Globtail;
extern_ struct symtable *Loclhead, *Locltail;
extern_ struct symtable *Parmhead, *Parmtail;
extern_ struct symtable *Membhead, *Membtail;
extern_ struct symtable *Structhead, *Structtail;
extern_ struct symtable *Unionhead, *Uniontail;
extern_ struct symtable *Enumhead, *Enumtail;
extern_ struct symtable *Typehead, *Typetail;
