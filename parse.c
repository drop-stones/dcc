#include "dcc.h"

void program ();
Node *stmt ();
Node *expr ();
Node *assign ();
Node *equality ();
Node *relational ();
Node *add ();
Node *mul ();
Node *unary ();
Node *primary ();

Node *code [100];
LVar *locals;

// find local variable
LVar *find_lvar (Token *tok) {
  for (LVar *var = locals; var != NULL; var = var->next)
    if (var->len == tok->len && !strncmp (tok->str, var->name, var->len))
      return var;
  return NULL;
}

Node *new_node (NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc (1, sizeof (Node));
  node->kind = kind;
  node->lhs  = lhs;
  node->rhs  = rhs;
  return node;
}

Node *new_node_num (int val) {
  Node *node = calloc (1, sizeof (Node));
  node->kind = ND_NUM;
  node->val  = val;
  return node;
}


void program () {
  int i = 0;

  if (locals == NULL)
    locals = calloc (1, sizeof (Node));

  while (!at_eof ())
    code [i++] = stmt ();
  code [i] = NULL;
}

Node *stmt () {
  Node *node;
  Token *tok;

  if ((tok = consume_keyword ()) != NULL) {
    switch (tok->kind) {
    case TK_RETURN:
      node = calloc (1, sizeof (Node));
      node->kind = ND_RETURN;
      node->lhs = expr ();
      expect (";");
      break;
    case TK_IF:
      node = calloc (1, sizeof (Node));
      node->kind = ND_IF;
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
      node = calloc (1, sizeof (Node));
      node->kind = ND_WHILE;
      expect ("(");
      node->cond = expr ();
      expect (")");
      node->body = stmt ();
      break;
    case TK_FOR:
      node = calloc (1, sizeof (Node));
      node->kind = ND_FOR;
      expect ("(");
      if (!consume (";")) {
        node->init = expr ();
        expect (";");
      }
      if (!consume (";")) {
        node->cond = expr ();
        expect (";");
      }
      if (!consume (")")) {
        node->inc = expr ();
        expect (")");
      }
      node->body = stmt ();
    }
  } else if (consume ("{")) {
    Node head = {};
    Node *cur = &head;

    while (!consume ("}")) {
      cur->next = stmt ();
      cur = cur->next;
    }
    node = calloc (1, sizeof (Node));
    node->kind = ND_BLOCK;
    node->body = head.next;
  } else {
    node = expr ();
    expect (";");
  }
  return node;
}

Node *expr () {
  return assign ();
}

Node *assign () {
  Node *node = equality ();

  if (consume ("="))
    node = new_node (ND_ASSIGN, node, assign ());
  return node;
}

Node *equality () {
  Node *node = relational ();

  for (;;) {
    if (consume ("=="))
      node = new_node (ND_EQ, node, relational ());
    else if (consume ("!="))
      node = new_node (ND_NE, node, relational ());
    else
      return node;
  }
}

Node *relational () {
  Node *node = add ();

  for (;;) {
    if (consume ("<"))
      node = new_node (ND_LT, node, add ());
    else if (consume ("<="))
      node = new_node (ND_LE, node, add ());
    else if (consume (">"))
      node = new_node (ND_LT, add (), node);
    else if (consume (">="))
      node = new_node (ND_LE, add (), node);
    else
      return node;
  }
}

Node *add () {
  Node *node = mul ();

  for (;;) {
    if (consume ("+"))
      node = new_node (ND_ADD, node, mul ());
    else if (consume ("-"))
      node = new_node (ND_SUB, node, mul ());
    else
      return node;
  }
}

Node *mul () {
  Node *node = unary ();

  for (;;) {
    if (consume ("*"))
      node = new_node (ND_MUL, node, unary ());
    else if (consume ("/"))
      node = new_node (ND_DIV, node, unary ());
    else
      return node;
  }
}

Node *unary () {
  if (consume ("+"))
    return primary ();
  if (consume ("-"))
    return new_node (ND_SUB, new_node_num (0), primary ());
  return primary ();
}

Node *primary () {
  Node *node;
  Token *tok;

  if (consume ("(")) {
    node = expr ();
    expect (")");
    return node;
  } else if ((tok = consume_ident ()) != NULL) {
    node = calloc (1, sizeof (Node));
    node->kind = ND_LVAR;

    LVar *lvar = find_lvar (tok);
    if (lvar != NULL) {
      // the lvar exists
      node->offset = lvar->offset;
    } else {
      // the lvar doesn't exist
      lvar = calloc (1, sizeof (LVar));
      lvar->next = locals;
      lvar->name = tok->str;
      lvar->len  = tok->len;
      lvar->offset = locals->offset + 8;
      node->offset = lvar->offset;
      locals = lvar;
    }
    return node;
  } else {
    return new_node_num (expect_number ());
  }
}

