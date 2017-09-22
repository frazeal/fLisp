#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpc.h"

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

long eval_op_unary(long x, char* op) {
  if (strcmp(op, "-") == 0) { return -x; }
  return x;
}

/* Use operator string to see which operation to perform */
long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; } 
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return (y != 0) ? (x / y) : 0; }
  if (strcmp(op, "%") == 0) { return x % y; }
  if (strcmp(op, "^") == 0) { return (long int) pow((double)x,y); }
  if (strcmp(op, "min") == 0) { return (x < y) ? x : y; }
  if (strcmp(op, "max") == 0) { return (x > y) ? x : y; }
  return 0;
}
  
long eval(mpc_ast_t* t) {

  /* If tagged as a number return it directly */
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  /* The operator is always the second child. */
  char* op = t->children[1]->contents;

  /* We store the third child in 'x' */
  long x = eval(t->children[2]);

  /* If the node has 4 children, we have a unary operator */
  if (t->children_num == 4) {
    x = eval_op_unary(x, op);
    return x;
  }
  
  /* Iterate the remaining children and combining. */
  int i = 3;

  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char* argv[]) {

  /* Create some Parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT,
	"                                                             \
         number   : /-?[0-9]+/ ;                                      \
         operator : '+' | '-' | '*' | '/' | '%' | '^' |               \
                    \"add\" | \"sub\" | \"mul\" | \"div\" |           \
                    \"mod\" | \" pwr\" | \"min\" | \"max\";           \
         expr     : <number> | '(' <operator> <expr>+ ')' ;	      \
         lispy    : /^/ <operator> <expr>+  /$/		;	      \
        ",
	    Number, Operator, Expr, Lispy);
  
  puts("fLisp Version 0.0.0.0.1");
  puts("Copyright ©frazeal 2017");
  puts("Press <Ctrl> + <c> to Exit\n");

  /* read-evaluate-print loop */
  while(1) {

    /* Output our prompt and get input */
    char* input = readline("fLisp> ");

    /* Add input to history */
    add_history(input);

    /* Attempt to parse the user input */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {

      /* On success print the AST */
      //mpc_ast_print(r.output);
      long result = eval(r.output);
      printf("%li\n", result);
      mpc_ast_delete(r.output);
      
    } else {

      /* Otherwise Print the error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);

    }

    /* Free retrieved input */
    free(input);
    
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);
  
  return 0;

}
