
/**
 * @file queue.c
 * @author François Cayre <francois.cayre@grenoble-inp.fr>
 * @date Fri Jul  2 19:02:17 2021
 * @brief Queue.
 *
 * Queue.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* NULL */

#include <pyas/queue.h>

struct link_t {
  void          *content;
  struct link_t *next;
};

queue_t queue_new( void ) {
  return NULL;
}

int     queue_empty( queue_t q ) {
  return queue_new() == q;
}

queue_t enqueue( queue_t q, void* object ) {
  struct link_t *new = queue_new();
  new = calloc(1, sizeof(*new));
  assert (new);
  new -> content = object;

  if (!queue_empty(q)){
    new -> next = q ->next;
    q->next = new;
  }
  else{
    new->next = new;
  }

  return new;
}

list_t  queue_to_list( queue_t *q ) {
  list_t l = list_new();
  assert (!queue_empty(*q) && l);

  l = (*q)->next;
  (*q)->next = NULL;

  *q=NULL;

  return l;
}
