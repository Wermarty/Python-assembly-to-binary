
/**
 * @file test-list.c
 * @author François Cayre <francois.cayre@grenoble-inp.fr>
 * @date Fri Jul  2 17:58:47 2021
 * @brief Driver for list of lexems.
 *
 * Tests unitaires pour les "listes de lexems"
 */

#include <pyas/all.h>
#include <unitest/unitest.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static void list_basics( void ) {
  test_suite( "Basic test of list module, list of lexems (list_add_first, list_print, list_del_first)" );

  list_t l = list_new();

  // test sur liste vide
  test_assert( l == NULL,
	       "An empty list is NULL" );
  test_assert( list_length(l) == 0,
	       "An empty list has zero-length" );
  test_oracle( list_print( l, lexem_print ), "( )",
	       "An empty list should be printed as '( )'" );

  l = list_add_first( l, lexem_new( "int", "42", 1, 8,-1 ) );


  // test liste à un élément
  test_assert( l != NULL,
	       "A non-empty list should be not NULL" );
  test_assert( list_length(l) == 1,
	       "list_length() should be 1" );
  test_oracle( list_print( l, lexem_print ), "( [1:8:int] 42 )",
	       "Can print a list w/ 1 element");

  // ajout en tête d'un second élément
  l = list_add_first( l, lexem_new( "str", "ficelle", 1, 0 ,-1) );

  // test liste à 2 éléments
  test_assert( list_length(l) == 2,
	       "list_length() should be 2" );
  test_oracle( list_print( l, lexem_print ), "( [1:0:str] ficelle [1:8:int] 42 )",
	       "Can print a list w/ 2 elements");

  // suppression d'un élément en tête
  l = list_del_first( l, lexem_delete );

  // test de l'affichage après suppression en tête
  test_assert( list_length(l) == 1,
	       "list_length() should be 1 after list_del_first()" );
  test_oracle( list_print( l, lexem_print ), "( [1:8:int] 42 )",
	       "List is OK after list_del_first()");

  // free memory
  list_delete( l, lexem_delete );
}


int main ( int argc, char *argv[] ) {

  unit_test( argc, argv );

  list_basics();

  exit( EXIT_SUCCESS );
}
