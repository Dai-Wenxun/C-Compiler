#ifndef extern_
#define extern_ extern
#endif


extern_ int Line;
extern_ int	Putback;
extern_ int Functionid;
extern_ int Globs;
extern_ FILE *Infile;
extern_ FILE *Outfile;
extern_ struct token Token;
extern_ char Text[TEXTLEN + 1];
extern_ struct symtable Gsym[NSYMBOLS];

extern_ int O_dumpAST;