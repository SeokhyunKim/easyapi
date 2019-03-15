%{
/* Bison can generate c++ code. But, I would do that later. */
#include "func_util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void yyerror(char* s);

char error_message[1000];
double evaluation_result;

%}

%union { double num; char str[100]; }
%start statement
%token <str> identifier
%token <num> number
%type <num> func
%type <num> arg

%%

statement	: '=' func						{ evaluation_result = $2; }
			;
func		: identifier '(' arg_list ')'	{ struct func_call_result result = eval_func_call($1);
                                              if (result.is_success != 1) {
                                                yyerror("failed to evaluate function\n");
                                              } else {
                                                $$ = result.value;
                                              }
                                            }
			;
arg_list	: /* empty */ 
            | arg_list ',' arg				;
			| arg							;
			;
arg			: number						{ add_arg($1); }
            /* want to extend to support func call for getting an argument later. To do this, func_util should be extended.
			| func							{ add_arg($1); } */
            ;

%%

void yyerror(char* s) {
    strncpy(error_message, s, sizeof(error_message));
}

char* get_error_message() {
    return error_message;
}

int is_evaluation_succeeded() {
    if (error_message[0] == '\0') {
        return 1;
    }
    return 0;
}

long evaluate_function(char* str) {
    yy_scan_string(str);
    init_func_call();
    error_message[0] = '\0';
    yyparse();
    /* currently, casting double to int. will consider different evaluation type later, if needed */
    return (long)evaluation_result;
}

/* for testing
int main() {
    char line[1024];
    fgets(line, sizeof(line), stdin);
    long result = evaluate_function(line);
    if (1 == is_evaluation_succeeded()) {
        printf("%ld\n", result);
    } else {
        printf("Evaluation failed. %s\n", error_message);
    }
    return 0;
}*/


			
