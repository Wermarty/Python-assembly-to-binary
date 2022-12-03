#include <stdio.h>
#include <stdlib.h>

#include <pyas/chargroup.h>
#include <pyas/list.h>


int main(int argc, char *argv[]){

  if (argc<2){
    printf("Need more inputs : '*.exe + regexp'\n");
    exit(EXIT_SUCCESS);
  }

  list_t chargroup_list = list_new();

  chargroup_list = chargroup_regexp2list(argv[1]);

  list_print(chargroup_list, chargroup_print);

  list_delete(chargroup_list, chargroup_delete);

  return 0;
}
