#include "dcc.h"

//Node *code [100];

// All local variable instances created during parsing are
// accumulated to this list.
LVar *locals;

// find local variable by name.
static LVar *find_lvar (Token *tok) {
  for (LVar *var = locals; var != NULL; var = var->next)
    if (strlen (var->name) == tok->len && !strncmp (tok->str, var->name, tok->len))
      return var;
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

static Node *new_var_node (LVar *lvar) {
  Node *node = new_node (ND_LVAR);
  node->lvar = lvar;
  return node;
}

static LVar *new_lvar (char *name) {
  LVar *lvar = calloc (1, sizeof (LVar));
  lvar->next = locals;
  lvar->name = name;
  //lvar->offset = locals->offset + 8;
  locals = lvar;
  return lvar;
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

static Function *function (void) {
  locals = NULL;

  char *name = expect_ident ();
  expect ("(");
  expect (")");
  expect ("{");

  Node head = {};
  Node *cur = &head;

  while (!consume ("}")) {
    cur->next = stmt ();
    cur = cur->next;
  }

  Function *fn = calloc (1, sizeof (Function));
  fn->name = name;
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
//      | expr ";"
Node *stmt () {
  Node *node;
  Token *tok;

  if ((tok = consume_keyword ()) != NULL) {
    switch (tok->kind) {
    case TK_RETURN:
      node = new_unary (ND_RETURN, expr ());
      expect (";");
      break;
    case TK_IF:
      node = new_node (ND_IF);
      expect ("(");
      node->cond = expr ();
      expect (")");
      node->then = stmt ();
      if (token->kind == TK_ELSE) {
        consume_keyword ();
        node->els = stmt ();
      }
      break;
    case TK_WHILE:
      node = new_node (ND_WHILE);
      expect ("(");
      node->cond = expr ();
      expect (")");
      node->then = stmt ();
      break;
    case TK_FOR:
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
    }
  } else if (consume ("{")) {
    Node head = {};
    Node *cur = &head;

    while (!consume ("}")) {
      cur->next = stmt ();
      cur = cur->next;
    }
    node = new_node (ND_BLOCK);
    node->body = head.next;
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

// unary = ("+" | "-")? unary
Node *unary () {
  if (consume ("+"))
    return primary ();
  if (consume ("-"))
    return new_binary (ND_SUB, new_num (0), primary ());
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
    LVar *lvar = find_lvar (tok);
    if (!lvar)
      lvar = new_lvar (strndup (tok->str, tok->len));
    return new_var_node (lvar);
  } else {
    return new_num (expect_number ());
  }
}

