#define _POSIX_C_SOURCE 200809L /* strdup(3) */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <pyas/lexem.h>
#include <pyas/list.h>
#include <pyas/queue.h>

struct lexem {
  char *type;
  char *value;
  unsigned char opcode;
  int   line;
  int   column;
};

lexem_t lexem_new( char *type, char *value, int line, int column , int sizevalue) {
  lexem_t lex = calloc( 1, sizeof( *lex ) );

  assert( lex );

  if ( type  && *type  ) lex->type  = strdup( type );
  if(sizevalue == -1){
    if ( value && *value ) lex->value = strdup( value );
  }
  else{
    lex->value = strndup( value,sizevalue );
  }
  lex->line   = line;
  lex->column = column;

  return lex;
}

int     lexem_print( void *_lex ) {
  lexem_t lex = _lex;

  return printf( "[%d:%d:%s] %s",
		 lex->line,
		 lex->column,
		 lex->type,
		 lex->value );
}

int     lexem_delete( void *_lex ) {
  lexem_t lex = _lex;

  if ( lex ) {
    free( lex->type );
    free( lex->value );
  }

  free( lex );

  return 0;
}
