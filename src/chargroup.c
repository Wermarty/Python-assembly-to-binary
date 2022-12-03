#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <pyas/chargroup.h>
#include <pyas/list.h>
#include <pyas/queue.h>

#define TAB_DIMENSION 128

struct chargroup{
  char tab[TAB_DIMENSION];
  chargroup_operator_t operator;
};


//Need to check
chargroup_t chargroup_new(){
  chargroup_t chargroup = calloc(1, sizeof(*chargroup));

  int i;
  for (i=0; i<TAB_DIMENSION; i++){
    chargroup->tab[i] = 0;
  }

  return chargroup;
}

int chargroup_delete(void *chargroup_){
  chargroup_t chargroup = (chargroup_t)chargroup_;
  free(chargroup);

  return 1;
}


//I think it works but I need to verify
void chargroup_reverse(chargroup_t chargroup){
  int i;
  for (i=0; i<TAB_DIMENSION; i++){
    chargroup->tab[i] = !chargroup->tab[i];
  }
}

int get_operator(char c, chargroup_operator_t *op){
  switch(c){
    case '*': *op = CHARGROUP_ZERO_OR_ONE; return 1;
    case '+': *op = CHARGROUP_ONE_OR_MORE; return 1;
    case '?': *op = CHARGROUP_ZERO_OR_ONE; return 1;
    default : *op = CHARGROUP_NOTHING;     return 0;
  }
}

void chargroup_fill_unique(chargroup_t chargroup, char **src){
  chargroup->tab[(int)**src] = (char)1;

  chargroup_operator_t op;
  if (get_operator(**(src+1), &op)){
    (*src)++;
  }
  chargroup->operator = op;
  (*src)++;

}

int chargroup_fill_escaped(chargroup_t chargroup, char **src){

  int filled = 0;
  if (**src == 'n'){
    chargroup->tab[(char)10] = (char)1;
    filled++;
  }
  else if(**src == 't'){
    chargroup->tab[(char)9] = (char)1;
    filled++;
  }

  if (filled){
    chargroup_operator_t op;
    if (get_operator(**(src+1), &op)){
      (*src)++;
    }
    chargroup->operator = op;
    (*src)++;
  }
  return filled;
}


int chargroup_fill_multiple(chargroup_t chargroup, char **src){
  char car1 = **src;
  char car2 = **(src+2);

  if (car2 == '\0'){
    printf("Reaching the end of the line\n");
    return 0;
  }

  if((int)car1 >= (int)car2){
    printf("Error with '-', %c should be lower than %c\n", car1, car2);
    return 0;
  }
  int i;
  for (i = (int)car1; i<(int)car2 +1; i++){
    chargroup->tab[i] = (char)1;
  }

  *src += 3;
  return 1;
}

list_t chargroup_reg2chargroup(char *src){

  chargroup_t chargroup;

  int reverse_chargroup = 0;

  queue_t chargroup_queue = queue_new();
  assert (chargroup_queue);

  char c = *src;
  while(c != '\0'){

    chargroup = chargroup_new();
    chargroup_operator_t op;

    //Si on a une negation
    if (c == '^'){
      reverse_chargroup = 1;
      src++;
    }

    //On garde un if pour continuer
    if (c == '\\'){
      src++;
      if (!chargroup_fill_escaped(chargroup, &src)){
        //Si on a pas de caractère spéciaux à ne pas echapper
        chargroup_fill_unique(chargroup, &src);
      }
    }

    else if(c == '-'){
      //Cas d'erreur
      printf("Error parsing regexp : '-' unavailable outside brackets\n");
      chargroup_delete(chargroup);
      exit(EXIT_FAILURE);
    }

    else if(c == '.'){
      //All is good
      int i;
      for (i=0; i<TAB_DIMENSION; i++){
        chargroup->tab[i] = (char)1;
        }
      if (get_operator(c, &op)){
        src++;
      }
      chargroup->operator = op;
      src++;
    }

    else if(c != '['){
      chargroup_fill_unique(chargroup, &src);
    }

    //Bracket's case
    else{
      src++;
      while (*src != ']' || *src != '\0'){
        //Check if there's a backslash
        if (c == '\\'){
          src++;
          if (!chargroup_fill_escaped(chargroup, &src)){
            //Si on a pas de caractère spéciaux à ne pas echapper
            chargroup_fill_unique(chargroup, &src);
          }
        }
        else if( *src == '*' || *src == '?' || *src == '+'){
          printf("Error inside brackets, can't use %c without espacing\n", *src);
          exit(EXIT_SUCCESS);
        }

        else if(*(src+1) == '-'){
          if (!chargroup_fill_multiple(chargroup, &src)){
            printf("Error inside brackets : %c%c%c\n", *(src-1), *src, *(src+1));
            exit(EXIT_SUCCESS);
          }
        }

        else{
          chargroup_fill_unique(chargroup, &src);
        }
      }
      if (*src == '\0'){
        printf("Need a closing brackets in the expression\n");
        exit(EXIT_FAILURE);
      }
    }

    if (reverse_chargroup){
      chargroup_reverse(chargroup);
      reverse_chargroup = 0;
    }

    chargroup_queue = enqueue(chargroup_queue, chargroup);


  }
  list_t chargroup_list = list_new();
  chargroup_list = queue_to_list(&chargroup_queue);

  return chargroup_list;
}

int chargroup_print(void *chargroup_){
chargroup_t chargroup = (chargroup_t) chargroup_;
  switch(chargroup->operator){
    case CHARGROUP_ZERO_OR_MORE : printf("Zero or more in : "); break;
    case CHARGROUP_ONE_OR_MORE  : printf("One or more in : "); break;
    case CHARGROUP_ZERO_OR_ONE  : printf("Zero or one in : "); break;
    default : printf("One in : "); break;
  }
  int i;
  for (i=0; i<TAB_DIMENSION; i++){
    if (chargroup->tab[i]){
      printf("%c", (char)i);
    }
  }
  return 1;
}
