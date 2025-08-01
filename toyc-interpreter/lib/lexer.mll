{
  open Parser
  exception Lexical_error of string
}

let digit = ['0'-'9']
let alpha = ['a'-'z' 'A'-'Z' '_']
let alnum = ['a'-'z' 'A'-'Z' '0'-'9' '_']

rule token = parse
  | [' ' '\t' '\n' '\r'] { token lexbuf }  
  | "//" [^ '\n']*       { token lexbuf }  
  | "/*"                 { comment 1 lexbuf }  
  
  | "int"       { TYPE_INT }
  | "void"      { TYPE_VOID }
  | "if"        { IF }
  | "else"      { ELSE }
  | "while"     { WHILE }
  | "break"     { BREAK }
  | "continue"  { CONTINUE }
  | "return"    { RETURN }
  
  | '+'         { PLUS }
  | '-'         { MINUS }
  | '*'         { TIMES }
  | '/'         { DIVIDE }
  | '%'         { MOD }
  | '='         { ASSIGN }
  | '!'         { NOT }
  | "&&"        { AND }
  | "||"        { OR }
  | "<"         { LT }
  | ">"         { GT }
  | "<="        { LE }
  | ">="        { GE }
  | "=="        { EQ }
  | "!="        { NE }
  
  | '('         { LPAREN }
  | ')'         { RPAREN }
  | '{'         { LBRACE }
  | '}'         { RBRACE }
  | ';'         { SEMI }
  | ','         { COMMA }
  
  | alpha alnum* as id { ID id }
  | '-'         { MINUS }
  | digit+ as num { NUMBER (int_of_string num) }
  
  | eof         { EOF }
  | _           { raise (Lexical_error ("Unexpected char: " ^ Lexing.lexeme lexbuf)) }

and comment depth = parse
  | "*/"        { token lexbuf }
  | "/*"        { comment (depth+1) lexbuf }
  | _           { comment depth lexbuf }
  | eof         { let msg = Printf.sprintf "Unterminated comment at depth %d" depth in raise (Lexical_error msg)  }