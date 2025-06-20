#ifndef PEQCOMP_H
#define PEQCOMP_H

#include <stdio.h>

typedef int (*funcp)(int);

funcp peqcomp(FILE *f, unsigned char *codigo);

#endif