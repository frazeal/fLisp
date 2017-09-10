#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compiling on Windows */
#ifdef _WIN32
#include <string.h>

#define BUFFER_SIZE 2048
static char buffer[BUFFER_SIZE];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, BUFFER_SIZE, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}
#else
/* Otherwise include the editline header */
#include <editline/readline.h>
#endif

int main(int argc, char* argv[]) {

  puts("fLisp Version 0.0.0.0.1");
  puts("Copyright ©frazeal 2017");
  puts("Press <Ctrl> + <c> to Exit\n");

  /* read-evaluate-print loop */
  while(1) {

    /* Output our prompt and get input */
    char* input = readline("fLisp> ");

    /* Add input to history */
    add_history(input);

    /* Echo input back to user */
    printf("I heard you're a %s\n", input);

    /* Free retrieved input */
    free(input);
    
  }
  
  return 0;

}
