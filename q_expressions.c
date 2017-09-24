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
typedef struct lval {
  int type;
  long num;
  /* Error and Symbol types have some string data */
  char* err;
  char* sym;
  /* Count and Pointer to a list of "lval*" */
  int count;
  struct lval** cell;
} lval;

/* Create an enumeration of possible lval types */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

/* Construct a pointer to a new number lval */
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* Construct a pointer to a new error type lval */
lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

/* Construct a pointer to a new symbol type lval */
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

/* Construct a pointer to a new Sexpr type lval */
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/* Construct a pointer to a new Qexpr type lval */
lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval* v) {

  switch (v->type) {
    /* Do nothing special for number type */
  case LVAL_NUM: break;

    /* For Err or Sym free the string data */
  case LVAL_ERR: free(v->err); break;
  case LVAL_SYM: free(v->sym); break;

    /* If Sexpr or Qexpr then delete all elements inside */
  case LVAL_SEXPR:
  case LVAL_QEXPR:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }
    /* Also free the memory allocated to contain the pointers */
    free(v->cell);
    break;
  }

  /* Free the memory allocated for the "lval" struct itself */
  free(v);
  
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return (errno != ERANGE)
    ? lval_num(x)
    : lval_err("Invalid number!");
}

lval* lval_read(mpc_ast_t* t) {

  /* If symbol or number return conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  /* If root (>), sexpr or qexpr then create an empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

  /* Fill this list with any valid expression contained within */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {

    /* Print value contained within */
    lval_print(v->cell[i]);

    /* Don't print trailing space if last element */
    if (i != v->count-1) {
      putchar(' ');
    }
  }
  putchar(close);
}

/* Print an "lval" */
void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM:   printf("%li", v->num); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
  }
}

/* Print an "lval" followed by a newline */
void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

lval* lval_pop(lval* v, int i) {
  /* Find the item at "i" */
  lval* x = v->cell[i];

  /* Shift memory after the item at "i" over the top */
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

  /* Decrease the count of items in the list */
  v->count--;

  /* Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* lval_join(lval* x, lval* y) {

  /* For each cell in 'y' add it to 'x' */
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  /* Delete the empty 'y' and return 'x' */
  lval_del(y);
  return x;
}

#define LASSERT(args, cond, err) \
  if (!(cond)) { lval_del(args); return lval_err(err); }

/*
#define LASSERT_NUMB_ARGS(args, cond,  expected)	\
  if (!(cond == expected)) { lavl_del(args);		\
    lval_err("Incorrect number of arguments!"); }

#define LASSERT_EMPTY(args, cond)			\
  if (cond == 0) { lval_del(args); lval_err("Empty list {}!");
*/

lval* lval_eval(lval* v);

lval* builtin_head(lval* a) {
  /* Check error conditions */
  LASSERT(a, a->count == 1,
	  "Function 'head' passed too many arguments!");

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
	  "Function 'head' passed incorrect types!");

  LASSERT(a, a->cell[0]->count != 0,
	  "Function 'head' passed {}!");

  /* Otherwise take first argument */
  lval* v = lval_take(a, 0);

  /* Delete all elements that are not head and return */
  while (v->count > 1) {
    lval_del(lval_pop(v, 1));
  }
  return v;
}

lval* builtin_tail(lval* a) {
  /* Check error conditions */
  LASSERT(a, a->count == 1,
	  "Function 'tail' passed too many arguments!");

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
	  "Function 'tail' passed incorrect types!");

  LASSERT(a, a->cell[0]->count != 0,
	  "Function 'tail' passed {}!");

  /* Take first argument */
  lval* v = lval_take(a, 0);

  /* Delete first element and return */
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lval* a) {
  /* Check error conditions */
  LASSERT(a, a->count == 1,
	  "Function 'eval' passed too many arguments!");

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
	  "Function 'eval' passed incorrect type!");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);
}

lval* builtin_join(lval* a) {

  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
	    "Function 'join' passed incorrect type!");
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* builtin_cons(lval* a) {
  /* Check error conditions */
  LASSERT(a, a->count == 2,
	  "Function 'cons' passed wrong arguments!");

  LASSERT(a, a->cell[1]->type == LVAL_QEXPR,
	  "Function 'cons' passed incorrect types!");
  
  LASSERT(a, a->cell[0]->count != 0,
	  "Function 'cons' passed {}!");

  /* Construct new Q-expr */
  lval* x = lval_qexpr();
  /* Add the value */
  x = lval_add(x, lval_pop(a, 0));
  /* Add the elements of the Q-expr */
  while (a->cell[1]->count) {
    x = lval_add(x, lval_pop(a->cell[1], 0));
  }
  
  lval_del(a);
  return x;
};

lval* builtin_len(lval* a) {
  /* Check error conditions */
  LASSERT(a, a->count == 1,
	  "Function 'last' passed too many arguments!");

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
	  "Function 'last' passed incorrect types!");

  LASSERT(a, a->cell[0]->count != 0,
	  "Function 'last' passed {}!");

  return lval_num(a->cell[0]->count);
};

lval* builtin_init(lval* a) {
     /* Check error conditions */
  LASSERT(a, a->count == 1,
	  "Function 'tail' passed too many arguments!");

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
	  "Function 'tail' passed incorrect types!");

  LASSERT(a, a->cell[0]->count != 0,
	  "Function 'tail' passed {}!");

  /* Take first argument */
  lval* v = lval_take(a, 0);

  /* Delete last element and return */
  lval_del(lval_pop(v, v->count-1));
  return v;
}

lval* builtin_last(lval* a) {
   /* Check error conditions */
  LASSERT(a, a->count == 1,
	  "Function 'last' passed too many arguments!");

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
	  "Function 'last' passed incorrect types!");

  LASSERT(a, a->cell[0]->count != 0,
	  "Function 'last' passed {}!");

  /* Otherwise take first argument */
  lval* v = lval_take(a, 0);

  /* Delete all elements that are not last and return */
  while (v->count > 1) {
   lval_del(lval_pop(v, 0));
  }
  return v;
}

lval* builtin_op(lval* a, char* op) {

  /* Ensure all arguments are number */
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }

  /* Pop the first element */
  lval* x = lval_pop(a, 0);

  /* If no arguments left and subtraction then perform unary negation */
  if (strcmp(op, "-") == 0 && a->count == 0) {
    x->num = -x->num;
  }

  /* While there are still elements remaining */
  while (a->count > 0) {

    /* Pop the next element */
    lval* y = lval_pop(a, 0);
    
    if (strcmp(op, "+") == 0) { x->num += y->num; } 
    if (strcmp(op, "-") == 0) { x->num -= y->num; } 
    if (strcmp(op, "*") == 0) { x->num *= y->num; } 
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
	lval_del(x);
	lval_del(y);
	x = lval_err("Division by zero!");
	break;
      }
      x->num /= y->num;
    }
    if (strcmp(op, "%") == 0) {
      if (y->num == 0) {
	lval_del(x);
	lval_del(y);
	x = lval_err("Division by zero!");
	break;
      }
      x->num %= y->num;
    }
    if (strcmp(op, "^") == 0) { x->num = (long) pow( x->num, y->num); }
    if (strcmp(op, "min") == 0) {
      x->num = (x->num < y->num) ? x->num : y->num;
    }
    if (strcmp(op, "max") == 0) {
      x->num = (x->num > y->num) ? x->num : y->num;
    }
    
    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval* builtin(lval* a, char* func) {
  if (strcmp("list", func) == 0) { return builtin_list(a); }
  if (strcmp("head", func) == 0) { return builtin_head(a); }
  if (strcmp("tail", func) == 0) { return builtin_tail(a); }
  if (strcmp("join", func) == 0) { return builtin_join(a); }
  if (strcmp("eval", func) == 0) { return builtin_eval(a); }
  if (strcmp("cons", func) == 0) { return builtin_cons(a); }
  if (strcmp("init", func) == 0) { return builtin_init(a); }
  if (strcmp("last", func) == 0) { return builtin_last(a); }
  if (strcmp("len" , func) == 0) { return builtin_len(a);  }
  if (strstr("+-*/%^", func)) { return builtin_op(a, func); }
  if (strcmp("min", func) == 0)  { return builtin_op(a, func); }
  if (strcmp("max", func) == 0)  { return builtin_op(a, func); }
  
  lval_del(a);
  return lval_err("Unknown function!");
}

lval* lval_eval(lval* v);

lval* lval_eval_sexpr(lval* v) {

  /* Evaluate children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  /* Error checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  /* Empty expression */
  if (v->count == 0) { return v; }
  
  /* Single expression */
  if (v->count == 1) { return lval_take(v,0); }

  /* Ensure the first element is a symbol */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression does not start with a symbol!");
  }

  /* Call built-in with operator */
  lval* result = builtin(v, f->sym);
  lval_del(f);
  return result;
}
  
lval* lval_eval(lval* v) {
  /* Evaluate S-Expressions */
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
  /* All other lval types remain the same */
  return v;
}

/**************************************************************************/

int main(int argc, char* argv[]) {

  /* Create some Parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr  = mpc_new("sexpr");
  mpc_parser_t* Qexpr  = mpc_new("qexpr");
  mpc_parser_t* Expr   = mpc_new("expr");
  mpc_parser_t* Lispy  = mpc_new("lispy");

  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT,
	"                                                             \
         number   : /-?[0-9]+/ ;                                      \
         symbol   : '+' | '-' | '*' | '/' | '%' | '^'                 \
                  | \"min\" | \"max\" | \"list\" | \"head\"           \
                  | \"tail\" | \"join\" | \"eval\" | \"cons\"         \
                  | \"len\" | \"init\" | \"last\"  ;		      \
         sexpr    : '(' <expr>* ')' ;                                 \
         qexpr    : '{' <expr>* '}' ;                                 \
         expr     : <number> | <symbol> | <sexpr> | <qexpr> ; 	      \
         lispy    : /^/ <expr>* /$/ ;				      \
        ",
	    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
  
  puts("fLisp Version 0.0.0.0.6");
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
      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    /* Free retrieved input */
    free(input);
    
  }

  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
  
  return 0;

}
