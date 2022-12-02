/**
 * @file regexp.c
 * @author François Cayre <francois.cayre@grenoble-inp.fr>
 * @date Fri Jul  28 09:56:03 2022
 * @brief regexp
 *
 * regexp code, as in the project's pdf document
 *
 */

#include <stdio.h>

#include <pyas/regexp.h>
#include <pyas/lexem.h>

int re_match_zero_or_more(char *tab, char *source, char **end){
  char *t = source;

  while(tab[(int)*t] && '\0' != *t){
    t++;
  }

  *end = t;

  return 1;
}
int re_match_one_or_more(char *tab, char *source, char **end){
  char *t = source;
  int is_here = 0;
  while(tab[(int)*t] && '\0' != *t){
    t++; //On passe de false a true et on incremente pour garder la fin
    is_here = 1; /// C'est pas opti de toujours mettre le meme nombre a la meme valeur non ?
  }

  *end = t;
  return is_here;
}

int re_match_zero_or_one(char *tab, char *source, char **end){
  char *t = source;
  int count = 0;
  while(tab[(int)*t] && '\0' != *t && !count){
    t++;
    count++;
  }
  *end = t;

  return 1;
}

int re_match_nothing(char *tab, char *source, char **end){
  if(tab[(int)*source]){
    *end = source+1;
    return 1;
  }
  return 0;
}
