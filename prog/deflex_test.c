#include <stdio.h>
#include <stdlib.h>

#include <pyas/chargroup.h>
#include <pyas/deflex.h>
#include <pyas/list.h>


int main(int argc, char *argv[]){

  if (argc<2){
    printf("Need more inputs : '*.exe + filename'\n");
    exit(EXIT_SUCCESS);
  }

  list_t deflex_list = list_new();

  deflex_list = deflex_file2list(argv[1]);

  list_print(deflex_list, &deflex_print);

  list_delete(deflex_list, &deflex_delete);

  return 0;
}
