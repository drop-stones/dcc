#include "dcc.h"


// All local variable instances created during parsing are
// accumulated to this list.
static VarList *locals;
static VarList *globals;
static VarList *scope;

// find a variable by name.
static Var *find_var (Token *tok) {
  for (VarList *vl = scope; vl; vl = vl->next) { 
    Var *var = vl->var;
    if (strlen (var->name) == tok->len && !strncmp (tok->str, var->name, tok->len))
      return var;
  }

  return NULL;
}

static Node *new_node (NodeKind kind, Token *tok) {
  Node *node = calloc (1, sizeof (Node));
  node->kind = kind;
  node->tok  = tok;
  return node;
}

static Node *new_binary (NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
  Node *node = new_node (kind, tok);
  node->lhs  = lhs;
  node->rhs  = rhs;
  return node;
}

static Node *new_unary (NodeKind kind, Node *expr, Token *tok) {
  Node *node = new_node (kind, tok);
  node->lhs  = expr;
  return node;
}

static Node *new_num (int val, Token *tok) {
  Node *node = new_node (ND_NUM, tok);
  node->val  = val;
  return node;
}

static Node *new_var_node (Var *var, Token *tok) {
  Node *node = new_node (ND_VAR, tok);
  node->var = var;
  return node;
}

static Var *new_var (char *name, Type *ty, bool is_local) {
  Var *var  = calloc (1, sizeof (Var));
  var->name = name;
  var->ty   = ty;
  var->is_local = is_local;

  VarList *sc = calloc (1, sizeof (VarList));
  sc->var  = var;
  sc->next = scope;
  scope    = sc;
  return var;
}

static Var *new_lvar (char *name, Type *ty) {
  Var *var = new_var (name, ty, true);

  VarList *vl = calloc (1, sizeof (VarList));
  vl->var = var;
  vl->next = locals;
  locals = vl;
  return var;
}

static Var *new_gvar (char *name, Type *ty) {
  Var *var = new_var (name, ty, false);

  VarList *vl = calloc (1, sizeof (VarList));
  vl->var = var;
  vl->next = globals;
  globals = vl;
  return var;
}

static char *new_label (void) {
  static unsigned int cnt = 0;
  char label [20];
  sprintf (label, ".L.data.%d", cnt++);
  return strndup (label, 20);
}

static Node *new_add (Node *lhs, Node *rhs, Token *tok) {
  add_type (lhs);
  add_type (rhs);

  if (is_integer (lhs->ty) && is_integer (rhs->ty))
    return new_binary (ND_ADD, lhs, rhs, tok);
  if (lhs->ty->base && is_integer (rhs->ty))
    return new_binary (ND_PTR_ADD, lhs, rhs, tok);
  if (is_integer (lhs->ty) && rhs->ty->base)
    return new_binary (ND_PTR_ADD, rhs, lhs, tok);	// lhs: PTR, rhs: integer

  error_tok (tok, "invalid operands");
  return NULL;
}

static Node *new_sub (Node *lhs, Node *rhs, Token *tok) {
  add_type (lhs);
  add_type (rhs);

  if (is_integer (lhs->ty) && is_integer (rhs->ty))
    return new_binary (ND_SUB, lhs, rhs, tok);
  if (lhs->ty->base && is_integer (rhs->ty))
    return new_binary (ND_PTR_SUB, lhs, rhs, tok);
  if (lhs->ty->base && rhs->ty->base)
    return new_binary (ND_PTR_DIFF, lhs, rhs, tok);

  error ("invalid operands");
  return NULL;
}

static Node *new_sizeof (Node *node, Token *tok) {
  add_type (node);
  return new_num (node->ty->size, tok);
}

static Function *function (void);
static Type *basetype (void);
static void global_var (void);
static Node *declaration (void);
static Node *stmt (void);
static Node *stmt2 (void);
static Node *expr (void);
static Node *assign (void);
static Node *equality (void);
static Node *relational (void);
static Node *add (void);
static Node *mul (void);
static Node *unary (void);
static Node *suffix (void);
static Node *primary (void);


bool is_function (void) {
  Token *tok = token;	// Save current token
  basetype ();
  bool is_func = consume_ident () && consume ("(");
  token = tok;		// Restore token
  return is_func;
}

// program = ( function | global_var )*
Program *program (void) {
  Program *prog = calloc (1, sizeof (Program));
  Function head = {};
  Function *cur = &head;

  while (!at_eof ()) {
    if (is_function ()) {
      cur->next = function ();
      cur = cur->next;
    } else {
      global_var ();
    }
  }

  prog->globals = globals;
  prog->fns = head.next;
  return prog;
}



// basetype = ( "char" | "int" ) "*"*
static Type *basetype (void) {
  Token *tok;
  Type *ty;

  if (tok = consume ("char"))
    ty = char_type;
  else if (tok = consume ("int"))
    ty = int_type;
  else
    error_tok (token, "type error");

  while (consume ("*"))
    ty = pointer_to (ty);
  return ty;
}

// type_suffix = ( "[" num "]" )*
static Type *read_type_suffix (Type *base) {
  if (!consume ("["))
    return base;
  int size = expect_number ();
  expect ("]");
  base = read_type_suffix (base);
  return array_of (base, size);
}

// param = basetype ident type_suffix
static VarList *read_func_param (void) {
  Type *ty = basetype ();
  char *name = expect_ident ();
  ty = read_type_suffix (ty);

  VarList *vl = calloc (1, sizeof (VarList));
  vl->var = new_lvar (name, ty);
  return vl;
}

// params = param ( "," param )*
static VarList *read_func_params (void) {
  if (consume (")"))
    return NULL;

  VarList *head = read_func_param ();
  VarList *cur = head;

  while (!consume (")")) {
    expect (",");
    cur->next = read_func_param ();
    cur = cur->next;
  }

  return head;
}

// function = basetype ident "(" params? ")" "{" stmt* "}"
// params   = param ( "," param )*
// param    = basetype ident type_suffix
static Function *function (void) {
  locals = NULL;

  Function *fn = calloc (1, sizeof (Function));
  fn->ty   = basetype ();
  fn->name = expect_ident ();
  expect ("(");

  VarList *sc = scope;
  fn->params = read_func_params ();
  expect ("{");

  Node head = {};
  Node *cur = &head;

  while (!consume ("}")) {
    cur->next = stmt ();
    cur = cur->next;
  }
  scope = sc;	// restore

  fn->node = head.next;
  fn->locals = locals;
  return fn;
}

static void global_var (void) {
  Token *tok  = token;
  Type  *ty   = basetype ();
  char  *name = expect_ident ();
  ty = read_type_suffix (ty);
  Var *gvar = new_gvar (name, ty);

  if (consume (";"))
    return;

  // initialize
  expect ("=");
  switch (ty->kind) {
  case TY_CHAR:
  case TY_INT:
    if (consume ("'"))
      gvar->val = 0;
    else
      gvar->val = expect_number ();
    break;
  case TY_PTR:
    gvar->int_ptr = (int *) expect_number ();
    break;
  case TY_ARRAY:
    expect ("{");
    gvar->int_arr = calloc (gvar->ty->array_len, sizeof (int));
    for (int i = 0; i < gvar->ty->size; i++) {
      if (consume ("}"))
        break;
      consume (",");
      gvar->int_arr [i] = expect_number ();
    }
    break;
  default:
    error_tok (tok, "invalid global variable initialization");
  }
  expect (";");
  return;
}

// declaration = basetype ident ("[" num "]")* ("=" expr) ";"
static Node *declaration (void) {
  Token *tok = token;
  Type *ty   = basetype ();
  char *name = expect_ident ();
  ty = read_type_suffix (ty);
  Var *var = new_lvar (name, ty);

  if (consume (";"))
    return new_node (ND_NULL, tok);

  // initialize
  expect ("=");
  Node *lhs = new_var_node (var, tok);
  Node *rhs = expr ();
  expect (";");
  Node *node = new_binary (ND_ASSIGN, lhs, rhs, tok);
  return new_unary (ND_EXPR_STMT, node, tok);
}

static Node *read_expr_stmt (void) {
  Token *tok = token;
  return new_unary (ND_EXPR_STMT, expr (), tok);
}

// Returns true if the next token represents a type.
static bool is_typename (void) {
  static char *ty[] = {"int", "char"};
  Token *tok;

  for (int i = 0; i < sizeof (ty) / sizeof (*ty); i++) {
    if (tok = peek (ty [i]))
      return true;
  }
  return false;
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | declaration
//      | expr ";"
static Node *stmt (void) {
  Node *node = stmt2 ();
  add_type (node);
  return node;
}

static Node *stmt2 (void) {
  Node *node;
  Token *tok;

  if (tok = consume ("return")) {
    node = new_unary (ND_RETURN, expr (), tok);
    expect (";");
  } else if (tok = consume ("if")) {
    node = new_node (ND_IF, tok);
    expect ("(");
    node->cond = expr ();
    expect (")");
    node->then = stmt ();
    if (consume ("else")) {
      node->els = stmt ();
    }
  } else if (tok = consume ("while")) {
    node = new_node (ND_WHILE, tok);
    expect ("(");
    node->cond = expr ();
    expect (")");
    node->then = stmt ();
  } else if (tok = consume ("for")) {
    node = new_node (ND_FOR, tok);
    expect ("(");
    if (!consume (";")) {
      node->init = read_expr_stmt ();
      expect (";");
    }
    if (!consume (";")) {
      node->cond = expr ();
      expect (";");
    }
    if (!consume (")")) {
      node->inc = read_expr_stmt ();
      expect (")");
    }
    node->then = stmt ();
  } else if (tok = consume ("{")) {
    Node head = {};
    Node *cur = &head;

    VarList *sc = scope;
    while (!consume ("}")) {
      cur->next = stmt ();
      cur = cur->next;
    }
    scope = sc;
    node = new_node (ND_BLOCK, tok);
    node->body = head.next;
  } else if (is_typename ()) {
    node = declaration ();
  } else {
    node = read_expr_stmt ();
    expect (";");
  }

  return node;
}

// expr = assign
static Node *expr (void) {
  return assign ();
}

// assign = equality ("=" assign)?
static Node *assign (void) {
  Node *node = equality ();
  Token *tok;
  if (tok = consume ("="))
    node = new_binary (ND_ASSIGN, node, assign (), tok);
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality (void) {
  Node *node = relational ();
  Token *tok;

  for (;;) {
    if (tok = consume ("=="))
      node = new_binary (ND_EQ, node, relational (), tok);
    else if (tok = consume ("!="))
      node = new_binary (ND_NE, node, relational (), tok);
    else
      return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational (void) {
  Node *node = add ();
  Token *tok;

  for (;;) {
    if (tok = consume ("<"))
      node = new_binary (ND_LT, node, add (), tok);
    else if (tok = consume ("<="))
      node = new_binary (ND_LE, node, add (), tok);
    else if (tok = consume (">"))
      node = new_binary (ND_LT, add (), node, tok);
    else if (tok = consume (">="))
      node = new_binary (ND_LE, add (), node, tok);
    else
      return node;
  }
}

// add = mul ("+" mul | "-" mul)*
static Node *add (void) {
  Node *node = mul ();
  Token *tok;

  for (;;) {
    if (tok = consume ("+"))
      node = new_add (node, mul (), tok);
    else if (tok = consume ("-"))
      node = new_sub (node, mul (), tok);
    else
      return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul (void) {
  Node *node = unary ();
  Token *tok;

  for (;;) {
    if (tok = consume ("*"))
      node = new_binary (ND_MUL, node, unary (), tok);
    else if (tok = consume ("/"))
      node = new_binary (ND_DIV, node, unary (), tok);
    else
      return node;
  }
}

// unary = "sizeof" unary
//       | ("+" | "-" | "*" | "&")? unary
//       | suffix
static Node *unary (void) {
  Token *tok;

  if (tok = consume ("sizeof"))
    return new_sizeof (unary (), tok);
  if (consume ("+"))
    return unary ();
  if (tok = consume ("-"))
    return new_binary (ND_SUB, new_num (0, tok), unary (), tok);
  if (tok = consume ("*"))
    return new_unary (ND_DEREF, unary (), tok);
  if (tok = consume ("&"))
    return new_unary (ND_ADDR, unary (), tok);
  return suffix ();
}

// suffix = primary ( "[" expr "]" )*
static Node *suffix (void) {
  Node *node = primary ();
  Token *tok;

  while (tok = consume ("[")) {
    // x[y] is short for *(x+y)
    Node *exp = new_add (node, expr (), tok);
    expect ("]");
    node = new_unary (ND_DEREF, exp, tok);
  }

  return node;
}

// stmt-expr = "(" "{" stmt stmt* "}" ")"
//
// Statement expression is a GNU C extension.
static Node *stmt_expr (Token *tok) {
  VarList *sc = scope;

  Node *node = new_node (ND_STMT_EXPR, tok);
  node->body = stmt ();
  Node *cur  = node->body;

  while (!consume ("}")) {
    cur->next = stmt ();
    cur = cur->next;
  }
  expect (")");

  scope = sc;

  if (cur->kind != ND_EXPR_STMT) {
    exit (1);
    error_tok (cur->tok, "stmt expr returning void is not supported");
  }
  memcpy (cur, cur->lhs, sizeof (Node));
  return node;
}

// func-args = "(" (assign ("," assign)*)? ")"
static Node *func_args (void) {
  if (consume (")"))
    return NULL;

  Node *head = assign ();
  Node *cur = head;
  while (consume (",")) {
    cur->next = assign ();
    cur = cur->next;
  }

  expect (")");
  return head;
}

// primary = "(" "{" stmt-expr-tail
//         | "(" expr ")"
//         | ident func-args?
//         | str
//         | num
static Node *primary (void) {
  Node *node;
  Token *tok;

  if (tok = consume ("(")) {
    if (consume ("{"))
      return stmt_expr (tok);

    node = expr ();
    expect (")");
    return node;
  } else if (tok = consume_ident ()) {
    // Function call
    if (consume ("(")) {
      node = new_node (ND_FUNCALL, tok);
      node->funcname = strndup (tok->str, tok->len);
      node->args = func_args ();
      return node;
    }

    // Variable
    Var *var = find_var (tok);
    if (!var)
      error_tok (tok, "undeclared variable\n");
    return new_var_node (var, tok);
  } else if (token->kind == TK_STR) {
    tok = token;
    token = token->next;

    Type *ty = array_of (char_type, tok->cont_len);
    Var *var = new_gvar (new_label (), ty);
    var->contents = tok->contents;
    var->cont_len = tok->cont_len;
    return new_var_node (var, tok);
  } else if (token->kind == TK_NUM) {
    return new_num (expect_number (), token);
  } else {
    error_tok (token, "expected expression");
  }
}

