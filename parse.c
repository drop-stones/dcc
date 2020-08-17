#include "dcc.h"


// All local variable instances created during parsing are
// accumulated to this list.
static VarList *locals;

// find a local variable by name.
static Var *find_var (Token *tok) {
  for (VarList *vl = locals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (strlen (var->name) == tok->len && !strncmp (tok->str, var->name, tok->len))
      return var;
  }
  return NULL;
}

static Node *new_node (NodeKind kind) {
  Node *node = calloc (1, sizeof (Node));
  node->kind = kind;
  return node;
}

static Node *new_binary (NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc (1, sizeof (Node));
  node->kind = kind;
  node->lhs  = lhs;
  node->rhs  = rhs;
  return node;
}

static Node *new_unary (NodeKind kind, Node *expr) {
  Node *node = calloc (1, sizeof (Node));
  node->kind = kind;
  node->lhs  = expr;
  return node;
}

static Node *new_num (int val) {
  Node *node = calloc (1, sizeof (Node));
  node->kind = ND_NUM;
  node->val  = val;
  return node;
}

static Node *new_var_node (Var *var) {
  Node *node = new_node (ND_VAR);
  node->var = var;
  return node;
}

static Var *new_lvar (char *name) {
  Var *var = calloc (1, sizeof (Var));
  var->name = name;

  VarList *vl = calloc (1, sizeof (VarList));
  vl->var = var;
  vl->next = locals;
  locals = vl;
  return var;
}

static Function *function (void);
static Node *stmt (void);
static Node *expr (void);
static Node *assign (void);
static Node *equality (void);
static Node *relational (void);
static Node *add (void);
static Node *mul (void);
static Node *unary (void);
static Node *primary (void);


// program = function*
Function *program (void) {
  Function head = {};
  Function *cur = &head;

  while (!at_eof ()) {
    cur->next = function ();
    cur = cur->next;
  }

  return head.next;
}


// declaration = basetype ident ("=" expr) ";"
static Node *declaration (void) {
  //Token *tok = token;
  expect ("int");
  Var *var = new_lvar (expect_ident ());

  expect (";");
  return new_node (ND_NULL);
}

static VarList *read_func_params (void) {
  if (consume (")"))
    return NULL;

  VarList *head = calloc (1, sizeof (VarList));

  expect ("int");

  head->var = new_lvar (expect_ident ());
  VarList *cur = head;

  while (!consume (")")) {
    expect (",");
    expect ("int");

    cur->next = calloc (1, sizeof (VarList));
    cur->next->var = new_lvar (expect_ident ());
    cur = cur->next;
  }

  return head;
}

// function = ident "(" params? ")" "{" stmt* "}"
// params   = ident ("," ident)*
static Function *function (void) {
  locals = NULL;

  Function *fn = calloc (1, sizeof (Function));

  expect ("int");

  fn->name = expect_ident ();
  expect ("(");
  fn->params = read_func_params ();
  expect ("{");

  Node head = {};
  Node *cur = &head;

  while (!consume ("}")) {
    cur->next = stmt ();
    cur = cur->next;
  }

  fn->node = head.next;
  fn->locals = locals;
  return fn;
}

static Node *read_expr_stmt (void) {
  return new_unary (ND_EXPR_STMT, expr ());
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | declaration
//      | expr ";"
Node *stmt () {
  Node *node;
  //Token *tok;

  if (consume ("return")) {
    node = new_unary (ND_RETURN, expr ());
    expect (";");
  } else if (consume ("if")) {
    node = new_node (ND_IF);
    expect ("(");
    node->cond = expr ();
    expect (")");
    node->then = stmt ();
    if (consume ("else")) {
      node->els = stmt ();
    }
  } else if (consume ("while")) {
    node = new_node (ND_WHILE);
    expect ("(");
    node->cond = expr ();
    expect (")");
    node->then = stmt ();
  } else if (consume ("for")) {
    node = new_node (ND_FOR);
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
  } else if (consume ("{")) {
    Node head = {};
    Node *cur = &head;

    while (!consume ("}")) {
      cur->next = stmt ();
      cur = cur->next;
    }
    node = new_node (ND_BLOCK);
    node->body = head.next;
  } else if (peek ("int")) {
    node = declaration ();
  } else {
    node = read_expr_stmt ();
    expect (";");
  }

  return node;
}

// expr = assign
Node *expr () {
  return assign ();
}

// assign = equality ("=" assign)?
Node *assign () {
  Node *node = equality ();

  if (consume ("="))
    node = new_binary (ND_ASSIGN, node, assign ());
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality () {
  Node *node = relational ();

  for (;;) {
    if (consume ("=="))
      node = new_binary (ND_EQ, node, relational ());
    else if (consume ("!="))
      node = new_binary (ND_NE, node, relational ());
    else
      return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational () {
  Node *node = add ();

  for (;;) {
    if (consume ("<"))
      node = new_binary (ND_LT, node, add ());
    else if (consume ("<="))
      node = new_binary (ND_LE, node, add ());
    else if (consume (">"))
      node = new_binary (ND_LT, add (), node);
    else if (consume (">="))
      node = new_binary (ND_LE, add (), node);
    else
      return node;
  }
}

// add = mul ("+" mul | "-" mul)*
Node *add () {
  Node *node = mul ();

  for (;;) {
    if (consume ("+"))
      node = new_binary (ND_ADD, node, mul ());
    else if (consume ("-"))
      node = new_binary (ND_SUB, node, mul ());
    else
      return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul () {
  Node *node = unary ();

  for (;;) {
    if (consume ("*"))
      node = new_binary (ND_MUL, node, unary ());
    else if (consume ("/"))
      node = new_binary (ND_DIV, node, unary ());
    else
      return node;
  }
}

// unary = ("+" | "-" | "*" | "&")? unary
Node *unary () {
  if (consume ("+"))
    return primary ();
  if (consume ("-"))
    return new_binary (ND_SUB, new_num (0), primary ());
  if (consume ("*"))
    return new_unary (ND_DEREF, unary ());
  if (consume ("&"))
    return new_unary (ND_ADDR, unary ());
  return primary ();
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

// primary = "(" expr ")" | ident func-args? | num
Node *primary () {
  Node *node;
  Token *tok;

  if (consume ("(")) {
    node = expr ();
    expect (")");
    return node;
  } else if ((tok = consume_ident ()) != NULL) {
    // Function call
    if (consume ("(")) {
      node = new_node (ND_FUNCALL);
      node->funcname = strndup (tok->str, tok->len);
      node->args = func_args ();
      return node;
    }

    // Variable
    Var *var = find_var (tok);
    if (!var)
      //var = new_lvar (strndup (tok->str, tok->len));
      error_at (token->str, "undeclared identifier\n");
    return new_var_node (var);
  } else {
    return new_num (expect_number ());
  }
}

