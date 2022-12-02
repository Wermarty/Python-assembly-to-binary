
/**
 * @file list.h
 * @author François Cayre <francois.cayre@grenoble-inp.fr>
 * @date Fri Jul  2 18:07:09 2021
 * @brief Lists.
 *
 * Lists.
 */

#ifndef _LIST_H_
#define _LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pyas/callbacks.h>

#include <stddef.h> /* size_t */

  typedef struct link_t *list_t;

  list_t list_new( void );
  int    list_empty( list_t l );
  void*  list_first( list_t l );
  list_t list_next( list_t l );
  list_t list_add_first( list_t l, void *object );
  list_t list_del_first( list_t l, action_t delete );
  size_t list_length( list_t l );
  int    list_print( list_t l, action_t print );
  void   list_delete( list_t l, action_t delete );



#ifdef __cplusplus
}
#endif

#endif /* _LIST_H_ */
