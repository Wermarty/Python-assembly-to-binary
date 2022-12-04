#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <pyas/list.h>
#include <pyas/queue.h>
#include <pyas/chargroup.h>
#include <pyas/deflex.h>

#define MAX_BUFFER_SIZE 256


struct deflex{
  char *type;
  list_t chargroup_list;
};

deflex_t deflex_new(){
  deflex_t deflex = calloc(1, sizeof(*deflex));

  return deflex;
}

deflex_t deflex_create(char *type, char *regexp){
  deflex_t deflex = deflex_new();

  deflex->type = strdup(type);
  deflex->chargroup_list = chargroup_regexp2list(regexp);

  return deflex;
}

void skip_space(char *buffer[MAX_BUFFER_SIZE]){
  while(**buffer == ' ' || **buffer == '\t' || **buffer == '\n'){
    if (**buffer == '\0'){
      printf("Reaching the end of the definition\n");
      exit(EXIT_FAILURE);
    }
    (*buffer)++;
  }
}

void skip_not_space(char **buffer){
  while(**buffer != ' ' && **buffer != '\t' && **buffer != '\n'){
    if (**buffer == '\0'){
      printf("Reaching the end of the definition\n");
      exit(EXIT_FAILURE);
    }
    (*buffer)++;
  }
}

list_t deflex_file2list(char *filename){

  FILE *f = fopen(filename, "r");
  if (!f)
  {
    fprintf(stderr, "Error opening file '%s'\n", filename);
    exit(EXIT_FAILURE);
  }

  if (getc(f) == '\0'){
    printf("Empty file\n");
    exit(EXIT_SUCCESS);
  }
  fseek(f, -1, 1);

  queue_t deflex_queue = queue_new();
  char buffer[MAX_BUFFER_SIZE];

  while (fgets(buffer, MAX_BUFFER_SIZE, f)){

    if (*buffer != '#' && *buffer != '\n'){
      char *end = buffer;
      skip_space(&end);
      char *begining = end;

      skip_not_space(&end);
      char temp_type[end-begining];
      strncpy(temp_type, begining, end-begining); temp_type[end-begining] = '\0';

      skip_space(&end);
      begining = end;

      skip_not_space(&end);
      if (*(end-1) == 't' && *(end-2) == '\\' && *(end+1) == ']'){
        end += 3;
      }
      char temp_regexp[end-begining];
      strncpy(temp_regexp, begining, end-begining); temp_regexp[end-begining] = '\0';

      //Opcode pas fini
      // while(*end != '\0'){
      //   end++;
      // }
      // char temp_regexp[end-begining-1];
      // strncpy(temp_regexp, begining, end-begining-1); temp_regexp[end-begining-1] = '\0';

      deflex_queue = enqueue(deflex_queue, deflex_create(temp_type, temp_regexp));
    }
  }

  list_t deflex_list = list_new();
  deflex_list = queue_to_list(&deflex_queue);

  fclose(f);
  return deflex_list;
}

int deflex_print(void *deflex_){
  deflex_t deflex = (deflex_t)deflex_;

  printf("%s\n", deflex->type);
  list_print(deflex->chargroup_list, &chargroup_print);

  return 1;
}

int deflex_delete(void *deflex_){
  deflex_t deflex = (deflex_t)deflex_;

  free(deflex->type);
  list_delete(deflex->chargroup_list, chargroup_delete);
  free(deflex);

  return 1;
}
