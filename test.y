%token i

%start S
%%

S
	: L '=' R
	| R
	;

L
	: '*' R
	| i
	;

R
	: L
	;

%%