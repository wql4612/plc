open Toyc_lib
open Ast


(* ===== Structured Print Helpers ===== *)
let rec print_expr indent expr =
  match expr with
  | IntLit i -> Printf.printf "%sIntLit(%d)\n" indent i
  | Var name -> Printf.printf "%sVar(%s)\n" indent name
  | Binop (e1, op, e2) ->
    let op_str =
      match op with
      | Add -> "+"
      | Sub -> "-"
      | Mul -> "*"
      | Div -> "/"
      | Mod -> "%"
      | Lt -> "<"
      | Gt -> ">"
      | Le -> "<="
      | Ge -> ">="
      | Eq -> "=="
      | Ne -> "!="
      | And -> "&&"
      | Or -> "||"
    in
    Printf.printf "%sBinop\n" indent;
    Printf.printf "%s  Operator %s\n" indent op_str;
    Printf.printf "%s  Left\n" indent;
    print_expr (indent ^ "    ") e1;
    Printf.printf "%s  Right\n" indent;
    print_expr (indent ^ "    ") e2
  | Unop (op, e) ->
    let op_str =
      match op with
      | Neg -> "-"
      | Not -> "!"
    in
    Printf.printf "%sUnop(%s)\n" indent op_str;
    print_expr (indent ^ "  ") e
  | Call (fname, args) ->
    Printf.printf "%sCall(%s)\n" indent fname;
    List.iteri
      (fun i arg ->
         Printf.printf "%s  Arg[%d]\n" indent i;
         print_expr (indent ^ "    ") arg)
      args
;;

let rec print_stmt indent stmt =
  match stmt with
  | Block stmts ->
    Printf.printf "%sBlock\n" indent;
    List.iter (print_stmt (indent ^ "  ")) stmts
  | Assign (var, expr) ->
    Printf.printf "%sAssign(%s)\n" indent var;
    print_expr (indent ^ "  ") expr
  | Decl (var, expr) ->
    Printf.printf "%sDecl(%s)\n" indent var;
    print_expr (indent ^ "  ") expr
  | If (cond, then_stmt, else_stmt) ->
    Printf.printf "%sIf:\n" indent;
    Printf.printf "%s  Condition\n" indent;
    print_expr (indent ^ "    ") cond;
    Printf.printf "%s  Then\n" indent;
    print_stmt (indent ^ "    ") then_stmt;
    (match else_stmt with
     | Some s ->
       Printf.printf "%s  Else\n" indent;
       print_stmt (indent ^ "    ") s
     | None -> ())
  | While (cond, body) ->
    Printf.printf "%sWhile\n" indent;
    Printf.printf "%s  Condition\n" indent;
    print_expr (indent ^ "    ") cond;
    Printf.printf "%s  Body\n" indent;
    print_stmt (indent ^ "    ") body
  | Return expr_opt ->
    Printf.printf "%sReturn\n" indent;
    (match expr_opt with
     | Some expr -> print_expr (indent ^ "  ") expr
     | None -> Printf.printf "%s  (void)\n" indent)
  | ExprStmt expr ->
    Printf.printf "%sExprStmt\n" indent;
    print_expr (indent ^ "  ") expr
  | Break -> Printf.printf "%sBreak\n" indent
  | Continue -> Printf.printf "%sContinue\n" indent
  | EmptyStmt -> Printf.printf "%sEmptyStmt\n" indent
;;


(* ==== AST Printing ==== *)
let print_ast ast =
  List.iter
    (fun func ->
       Printf.printf
         "Function %s (returns %s)\n"
         func.fname
         (match func.rtype with
          | Int -> "int"
          | Void -> "void");
       Printf.printf "Parameters [%s]\n" (String.concat "; " func.params);
       Printf.printf "Body\n";
       print_stmt "  " func.body;
       print_newline ())
    ast;
;;

(* ==== Semantic Analysis ==== *)
let check_program ast =
  let has_main = ref false in
  List.iter
    (fun func ->
       if func.fname = "main"
       then (
         has_main := true;
         if func.rtype <> Int then failwith "Main function must return int";
         if func.params <> [] then failwith "Main function cannot take parameters");
       let rec check_returns stmt_list =
         List.iter
           (function
             | Ast.Return (Some _) when func.rtype = Void ->
               failwith "Void function cannot return a value"
             | Ast.Return None when func.rtype = Int ->
               failwith "Int function must return a value"
             | Ast.Block stmts -> check_returns stmts
             | Ast.If (_, then_stmt, Some else_stmt) ->
               check_returns [ then_stmt; else_stmt ]
             | Ast.If (_, then_stmt, None) -> check_returns [ then_stmt ]
             | _ -> ()) 
           stmt_list
       in
       check_returns [ func.body ])
    ast;
  if not !has_main then failwith "Program must contain a main function"
;;

(* ==== Parsing Core ==== *)

let parse_stdin () =
  let lexbuf = Lexing.from_channel stdin in
  try
    let ast = Parser.program Lexer.token lexbuf in
    check_program ast;
    ast
  with
  | Failure msg ->
    Printf.eprintf "Semantic error: %s\n" msg;
    exit 1
  | Lexer.Lexical_error msg ->
    Printf.eprintf "Lexical error: %s\n" msg;
    exit 1
  | Parser.Error ->
    let pos = lexbuf.lex_curr_p in
    Printf.eprintf
      "Syntax error at line %d, column %d\n"
      pos.pos_lnum
      (pos.pos_cnum - pos.pos_bol);
    exit 1
;;

(* ==== Main Entry ==== *)

let () =
  try
    let ast = parse_stdin () in
    print_ast ast;
    (* ignore (execute_program ast) *)
  with
  | Failure msg ->
    Printf.eprintf "Semantic error: %s\n" msg;
    exit 1
  | Parsing.Parse_error ->
    Printf.eprintf "Syntax error: Failed to parse from stdin\n";
    exit 1
  | e ->
    Printf.eprintf "Unknown error: %s\n" (Printexc.to_string e);
    exit 1
;;