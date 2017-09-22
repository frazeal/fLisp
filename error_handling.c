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

/* Declare new lval struct */
typedef struct {
  int type;
  long num;
  int err;
} lval;

/* Create an enumeration of possible lval types */
enum { LVAL_NUM, LVAL_ERR };

/* Create an enumeration of possible error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Create a new number type lval */
lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

/* Create a new error type lval */
lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

/* Print an "lval" */
void lval_print(lval v) {
  switch (v.type) {
    /* In the case the type is a number print it */
    /* Then 'break' out of the switch */
  case LVAL_NUM: printf("%li", v.num); break;

    /* In the case the type is an error */
  case LVAL_ERR:
    /* Check what type of error it is and print it */
    if (v.err == LERR_DIV_ZERO) {
      printf("Error: Division by zero!");
    }
    if (v.err == LERR_BAD_OP) {
      printf("Error: Invalid operator!");
    }
    if (v.err == LERR_BAD_NUM) {
      printf("Error: Invalid number!");
    }
    break;
  }
}

/* Print an "lval" followed by a newline */
void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}

lval eval_op_unary(lval x, char* op) {
  if (strcmp(op, "-") == 0) { return lval_num(-x.num); }
  return lval_err(LERR_BAD_OP);
}

/* Use operator string to see which operation to perform */
lval eval_op(lval x, char* op, lval y) {

  /* If either value is an error, return it */
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  /* Otherwise do maths on the number values */
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); } 
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    /* If second operand is zero, return error */
    return (y.num == 0)
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  }
  if (strcmp(op, "%") == 0) {
    return (y.num == 0)
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num % y.num);
  }
  if (strcmp(op, "^") == 0) {
    return lval_num((long int) pow((double) x.num, y.num));
  }
  if (strcmp(op, "min") == 0) {
    return lval_num(
		    (x.num < y.num)
		    ? x.num
		    : y.num);
  }
  if (strcmp(op, "max") == 0) {
    return lval_num(
		    (x.num > y.num)
		    ? x.num
		    : y.num);
  }

  return lval_err(LERR_BAD_OP);
}
  
lval eval(mpc_ast_t* t) {

  /* If tagged as a number return it directly */
  if (strstr(t->tag, "number")) {
    /* Check if there is some error in the conversion */
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return (errno != ERANGE)
      ? lval_num(x)
      : lval_err(LERR_BAD_NUM);
  }

  /* The operator is always the second child. */
  char* op = t->children[1]->contents;

  /* We store the third child in 'x' */
  lval x = eval(t->children[2]);

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
      lval result = eval(r.output);
      lval_println(result);
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
