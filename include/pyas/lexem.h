#ifndef _LEXEM_H_
#define _LEXEM_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <pyas/list.h>

typedef struct lexem *lexem_t;

lexem_t lexem_new( char *type, char *value, int line, int column, int sizevalue );

int     lexem_print( void *_lex );

int     lexem_delete( void *_lex );

#ifdef __cplusplus
}
#endif

#endif /* _LEXEM_H_ */
