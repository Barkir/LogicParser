#include <stdlib.h>
#include <stdio.h>

#include "buff.h"

size_t GetFileSize(FILE * fp)
{
    fseek(fp, 0L, SEEK_END);
    long int sz = ftell(fp);
    rewind(fp);

    if (sz == -1L) return 0;
    return (size_t) sz;
}

char * CreateBuf(FILE * toRead)
{
    size_t fsize = GetFileSize(toRead);
    if (!fsize) return NULL;

    char * expression = (char*) calloc(fsize + 1, sizeof(char));
    if (!expression) return NULL;

    size_t read = fread((expression), sizeof(char), fsize, toRead);
    if (read < fsize) return NULL;
    if (expression[read - 1] == '\n') expression[read - 1] = 0;
    return expression;
}



