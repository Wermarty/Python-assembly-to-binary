#include <pyas/all.h>
#include <unitest/unitest.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pyas/chargroup.h>




static void chargroup_basics( void ) {
  test_suite( "Basic test of chargroup module" );

  list_t l = list_new();

  l = chargroup_regexp2list("a");
  test_oracle( list_print(l, chargroup_print), "(\nOne in : a\n)", "a\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("a?");
  test_oracle( list_print(l, chargroup_print), "(\nZero or one in : a\n)", "a?\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("a+");
  test_oracle( list_print(l, chargroup_print), "(\nOne or more in : a\n)", "a+\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("a*");
  test_oracle( list_print(l, chargroup_print), "(\nZero or more in : a\n)", "a*\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("ab");
  test_oracle( list_print(l, chargroup_print), "(\nOne in : a\nOne in : b\n)", "ab\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("ab*");
  test_oracle( list_print(l, chargroup_print), "(\nOne in : a\nZero or more in : b\n)", "ab*\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("[a-z]");
  test_oracle( list_print(l, chargroup_print), "(\nOne in : abcdefghijklmnopqrstuvwxyz\n)", "[a-z]\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("[a-z0-9]");
  test_oracle( list_print(l, chargroup_print), "(\nOne in : 0123456789abcdefghijklmnopqrstuvwxyz\n)", "[a-z0-9]\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("[a-z]+");
  test_oracle( list_print(l, chargroup_print), "(\nOne or more in : abcdefghijklmnopqrstuvwxyz\n)", "[a-z]+\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("\n");
  test_oracle( list_print(l, chargroup_print), "(\nOne in : \n\n)", "backslash_n\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("\t");
  test_oracle( list_print(l, chargroup_print), "(\nOne in : \t\n)", "backslash_t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("[\\*\\+\\?]");
  test_oracle( list_print(l, chargroup_print), "(\nOne in : *+?\n)", "[\\*\\+\\?]\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("a-z");
  test_oracle( list_print(l, chargroup_print), "(\nError parsing regexp : '-' unavailable outside brackets\n\n)", "a-z\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("[ab");
  test_oracle( list_print(l, chargroup_print), "(\nNeed a closing brackets in the expression\n)", "[ab\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("][a-z]");
  test_oracle( list_print(l, chargroup_print), "(\nError parsing regexp : ']' need an opening bracket first\n\n)", "][a-z]\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("[z-a]");
  test_oracle( list_print(l, chargroup_print), "(\nError with '-', z should be lower than a\n\n)", "[z-a]\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("[a-z*]");
  test_oracle( list_print(l, chargroup_print), "(\nError inside brackets, can't use * without espacing\n\n)", "[a-z*]\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("a**");
  test_oracle( list_print(l, chargroup_print), "(\nError parsing regexp : can't use operator alone\n\n", "a**\t\t");
  list_delete(l, chargroup_delete);

  l = chargroup_regexp2list("*");
  test_oracle( list_print(l, chargroup_print), "(\nError parsing regexp : can't use operator alone\n\n", "*\t\t");
  list_delete(l, chargroup_delete);
}


int main ( int argc, char *argv[] ) {

  unit_test( argc, argv );

  chargroup_basics();

  exit( EXIT_SUCCESS );
}
