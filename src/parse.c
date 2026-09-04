#include "parse.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

bool parse(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) { puts("unable to open config file"); return false; }

    fseek(f, 0, SEEK_END);
    size_t length = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *contents = malloc(length + 1);
    if (!contents) { puts("memory allocation failed"); fclose(f); return false; }
    fread(contents, length, 1, f);
    bool haserror = ferror(f);
    fclose(f);
    if (haserror) { puts("failed to read config file"); goto errorquit_alloc; }
    contents[length++] = '\0';
    
    // ===============================

    char *starttok = contents;
    bool skipline = false;
    for (char *curr = contents; *curr; curr++)
    {
        if (*curr == '#') skipline = true;
        else if (*curr == '\n') { skipline = false; starttok = curr; }
        else if (isspace(*curr)) 
    }

    return true;
    errorquit_alloc:
        free(contents);
    return false;
}