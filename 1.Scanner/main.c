#include <stdio.h>
#include <stdlib.h>

static void usage(char *prog) {
    fprintf(stderr, "Usage: %s infile\n", prog);
    exit(1);
}



void main(int argc, char *argv[]) {
    if (argc != 2)
        usage(argv[0]);
    
}