%token a d x y w

%start S
%%

S
	: a B
	| C B d
	;

B
	: x 
	| C y
	| C
	;
C
	: 
	| w
	;

%%