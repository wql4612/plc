type binop = 
  | Add | Sub | Mul | Div | Mod 
  | Lt | Gt | Le | Ge | Eq | Ne
  | And | Or

type unop = 
  | Neg | Not

type expr = 
  | IntLit of int
  | Var of string
  | Binop of expr * binop * expr
  | Unop of unop * expr
  | Call of string * expr list

type stmt = 
  | Block of stmt list 
  | EmptyStmt
  | ExprStmt of expr
  | Assign of string * expr
  | Decl of string * expr
  | If of expr * stmt * stmt option
  | While of expr * stmt
  | Break
  | Continue
  | Return of expr option

type ret_type = Int | Void

type func_def = {
  fname: string;
  rtype: ret_type;
  params: string list;
  body: stmt;
}

type program = func_def list