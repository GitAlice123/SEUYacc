%token id

%start E
%%

E
	: E '+' T
	| T
	;

T
	: T '*' F
	| F
	;

F
	: '(' E ')'
	| id
	;

%%