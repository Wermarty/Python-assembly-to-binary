#ifndef CHARGROUP_H
#define CHARGROUP_H

#include <pyas/list.h>

typedef enum{
    CHARGROUP_ZERO_OR_MORE,
    CHARGROUP_ONE_OR_MORE,
    CHARGROUP_ZERO_OR_ONE,
    CHARGROUP_NOTHING
}chargroup_operator_t;

typedef struct chargroup *chargroup_t;

chargroup_t chargroup_new();
int chargroup_delete(void *chargroup_);

list_t chargroup_regexp2list(char *src);

int chargroup_print(void *chargroup_);

#endif
