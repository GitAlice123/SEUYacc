%token ID EQUALS MUL

%start S
%%

S
    : L EQUALS R
    | R
    ;

L
    : MUL R
    | ID
    ;

R
    : L
    ;

%%