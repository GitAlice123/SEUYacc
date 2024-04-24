%{
#include"parse.tab.h"
#include<stdio.h>
#include<string.h>
%}
%%

[0-9]+                  {yylval.val=atoi(yytext);return NUM;}
"print"                 {return PRINT;}
"declare"               {return DECL;}
[a-z]([0-9]|[a-z])*     {yylval.str= strdup(yytext);return NAME;}
"+"                     {return '+';}
"*"                     {return '*';}
"-"                     {return '-';}
"\n"                    {return '\n';}
"/"                     {return '/';}
"="                     {return '=';}

%%




// selection_statement
// 	: IF '(' expression ')' statement
// 	| IF '(' expression ')' statement ELSE statement
// 	| SWITCH '(' expression ')' statement
// 	;