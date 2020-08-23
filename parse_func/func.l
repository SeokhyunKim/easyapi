%{
#include "func.tab.h"
#include <string.h>
#include <stdlib.h>

extern void yyerror(char* s);
%}

%%

[a-zA-Z][_a-zA-Z0-9]*           { strcpy(yylval.str, yytext); return identifier; }
[-+]?[0-9]*\.?[0-9]*            { yylval.num = strtod(yytext, NULL); return number; }
[ \t\n]                         ;
[=,\(\)]                        { return yytext[0]; }
.                               { yyerror(yytext); }

%%

int yywrap() { return 1; }

