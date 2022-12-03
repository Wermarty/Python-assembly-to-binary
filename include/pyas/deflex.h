#ifndef DEFLEX_F
#define DEFLEX_H

#include <pyas/list.h>

typedef struct deflex *deflex_t;

list_t deflex_file2list(char *filename);

int deflex_print(void *deflex_);
int deflex_delete(void *deflex_);

#endif
