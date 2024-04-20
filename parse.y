%{
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

        struct node{
                char *name;
                int value;
        };
        static struct node* sym_table[100];//符号表
%}
%union{
        char *str;
        int val;
}

%token <val> NUM
%token <str> NAME
%token PRINT    
%token DECL
%left '+' '-'
%left '*' '/'

%type <val> exp

%start input 

%%
input: /* empty */
        | input line ;

line:   
       exp '\n'          {}
     | DECL NAME '\n'    {return_value($2);}    
     | NAME '=' exp '\n' {assign_value($1,$3);}
     | PRINT NAME '\n'   {printf("%d\n",return_value($2));}
     | error ;

exp:      exp '+' exp {$$ = $1 + $3;}
        | exp '*' exp {$$ = $1 * $3;}
        | exp '-' exp {$$ = $1 - $3;}
        | exp '/' exp { if ($3 == 0)
                                $$ = 0;
                        else
                                $$ = $1/$3;}
        | NUM         {$$ = $1;}
        | NAME        {$$ = return_value($1);}; 
%%

yyerror()
{
        printf("Error detected in parsing\n");
}

int assign_value(char *s,int symvalue)
{
        char *symname;
        int len,i;
        len=strlen(s) + 1;
        symname=malloc(sizeof(char *) * len);
        strcpy(symname,s);
        for(i=0;sym_table[i];i++)
                if(!strcmp(symname,sym_table[i]->name)){
                        sym_table[i]->value=symvalue;
                        return symvalue;
                }
        sym_table[i]=malloc(sizeof(struct node));
        sym_table[i]->name=symname;
        sym_table[i]->value=symvalue;
        return symvalue;
}

int return_value(char *s)
{
        char *symname;
        int len,i;
        len=strlen(s) + 1;
        symname=malloc(sizeof(char *) * len);
        strcpy(symname,s);
        for(i=0;sym_table[i];i++)
                if(!strcmp(symname,sym_table[i]->name))
                        return sym_table[i]->value;
        sym_table[i]=malloc(sizeof(struct node));
        sym_table[i]->name=symname;
        sym_table[i]->value=0;
        return 0;
}

main()
{
        yyparse();
}