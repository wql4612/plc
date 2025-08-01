%{
  open Ast
%}

%token <int> NUMBER
%token <string> ID
%token TYPE_INT TYPE_VOID
%token IF ELSE WHILE BREAK CONTINUE RETURN
%token PLUS MINUS TIMES DIVIDE MOD 
%token ASSIGN NOT AND OR
%token LT GT LE GE EQ NE
%token LPAREN RPAREN LBRACE RBRACE SEMI COMMA
%token EOF

%nonassoc unary_minus
%nonassoc IF
%nonassoc ELSE
%left OR
%left AND
%left LT GT LE GE EQ NE
%left PLUS MINUS
%left TIMES DIVIDE MOD
%left NOT

%start program
%type <Ast.program> program

%%

program:
  | func_def+ EOF { $1 }

func_def:
  | func_type ID LPAREN params RPAREN block 
    { { fname = $2; rtype = $1; params = $4; body = $6 } }

func_type:
  | TYPE_INT  { Int }
  | TYPE_VOID { Void }

params:
  | nonempty_params              { $1 }
  | /* noting */                  { [] }

nonempty_params:
  | param { [$1] }
  | nonempty_params COMMA param { $1 @ [$3]}

param:
  | TYPE_INT ID { $2 }

block:
  | LBRACE stmt* RBRACE { Block $2 }

stmt:
  | block                                  { $1 }
  | SEMI                                   { EmptyStmt }
  | expr SEMI                              { ExprStmt $1 }
  | ID ASSIGN expr SEMI                    { Assign ($1, $3) }
  | TYPE_INT ID ASSIGN expr SEMI           { Decl ($2, $4) }
  | IF LPAREN expr RPAREN stmt %prec IF    { If ($3, $5, None) }
  | IF LPAREN expr RPAREN stmt ELSE stmt   { If ($3, $5, Some $7) }
  | WHILE LPAREN expr RPAREN stmt          { While ($3, $5) }
  | BREAK SEMI                             { Break }
  | CONTINUE SEMI                          { Continue }
  | RETURN SEMI                            { Return None }
  | RETURN expr SEMI                       { Return (Some $2) }

expr:
  | LOrExpr { $1 }

LOrExpr:
  | LAndExpr                 { $1 }
  | LOrExpr OR LAndExpr      { Binop ($1, Or, $3) }

LAndExpr:
  | RelExpr                  { $1 }
  | LAndExpr AND RelExpr     { Binop ($1, And, $3) }

RelExpr:
  | AddExpr                  { $1 }
  | RelExpr LT AddExpr       { Binop ($1, Lt, $3) }
  | RelExpr GT AddExpr       { Binop ($1, Gt, $3) }
  | RelExpr LE AddExpr       { Binop ($1, Le, $3) }
  | RelExpr GE AddExpr       { Binop ($1, Ge, $3) }
  | RelExpr EQ AddExpr       { Binop ($1, Eq, $3) }
  | RelExpr NE AddExpr       { Binop ($1, Ne, $3) }

AddExpr:
  | MulExpr                  { $1 }
  | AddExpr PLUS MulExpr     { Binop ($1, Add, $3) }
  | AddExpr MINUS MulExpr    { Binop ($1, Sub, $3) }


MulExpr:
  | UnaryExpr                { $1 }
  | MulExpr TIMES UnaryExpr  { Binop ($1, Mul, $3) }
  | MulExpr DIVIDE UnaryExpr { Binop ($1, Div, $3) }
  | MulExpr MOD UnaryExpr    { Binop ($1, Mod, $3) }

UnaryExpr:
  | PrimaryExpr              { $1 }
  | PLUS UnaryExpr           { $2 }
  | MINUS UnaryExpr %prec unary_minus { Unop (Neg, $2) }
  | NOT UnaryExpr                     { Unop (Not, $2) }

PrimaryExpr:
  | ID                         { Var $1 }
  | NUMBER                     { IntLit $1 }
  | LPAREN expr RPAREN         { $2 }
  | ID LPAREN args RPAREN      { Call ($1, $3) }

args:
  | nonempty_args { $1 }
  | /* nothing */                 { [] }

nonempty_args:
  | expr { [$1] }
  | nonempty_args COMMA expr { $1 @ [$3] }