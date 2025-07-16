
(* The type of tokens. *)

type token = 
  | WHILE
  | TYPE_VOID
  | TYPE_INT
  | TIMES
  | SEMI
  | RPAREN
  | RETURN
  | RBRACE
  | PLUS
  | OR
  | NUMBER of (int)
  | NOT
  | NE
  | MOD
  | MINUS
  | LT
  | LPAREN
  | LE
  | LBRACE
  | IF
  | ID of (string)
  | GT
  | GE
  | EQ
  | EOF
  | ELSE
  | DIVIDE
  | CONTINUE
  | COMMA
  | BREAK
  | ASSIGN
  | AND

(* This exception is raised by the monolithic API functions. *)

exception Error

(* The monolithic API. *)

val program: (Lexing.lexbuf -> token) -> Lexing.lexbuf -> (Ast.program)
