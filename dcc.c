#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Token Token;
typedef struct Node Node;

// current Token
Token *token;
char *user_input;

/*
 *  Tokenizer
 */

typedef enum {
  TK_RESERVED,	// operation
  TK_NUM, 	// number
  TK_EOF,	// End of File
} TokenKind;


struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *str;
  int len;
};


// error function
// Take same arguments as printf.
void error_at (char *loc, char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);

  int pos = loc - user_input;
  fprintf (stderr, "%s\n", user_input);
  fprintf (stderr, "%*s", pos, "");
  fprintf (stderr, "^ ");
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  exit (1);
}

// consume one Token from Token sequence
bool consume (char *op) {
  if (token->kind != TK_RESERVED ||
      strlen (op) != token->len  ||
      memcmp (token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

// move to next Token from Token sequence
void expect (char *op) {
  if (token->kind != TK_RESERVED ||
      strlen (op) != token->len  ||
      memcmp (token->str, op, token->len))
    error_at (token->str, "'%c' is wrong.", op);
  token = token->next;
}

// return num from sequence
int expect_number () {
  if (token->kind != TK_NUM)
    error_at (token->str, "Not number.");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof () {
  return token->kind == TK_EOF;
}

// create new Token and append it to cur
Token *new_token (TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc (1, sizeof (Token));
  tok->kind = kind;
  tok->str  = str;
  cur->next = tok;
  return tok;
}

// Tokenize string
Token *tokenize (char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    if (isspace (*p)) {
      // skip white space
      p++;
    } else if (!strncmp (p, "<=", 2) ||
               !strncmp (p, ">=", 2) ||
               !strncmp (p, "==", 2) ||
               !strncmp (p, "!=", 2)) {
      cur = new_token (TK_RESERVED, cur, p);
      cur->len = 2;
      p += 2;
    } else if (strchr ("+-*/()<>", *p)) {
      cur = new_token (TK_RESERVED, cur, p++);
      cur->len = 1;
    } else if (isdigit (*p)) {
      cur = new_token (TK_NUM, cur, p);
      cur->val = strtol (p, &p, 10);
    } else {
      error_at (p, "invalid token\n");
    }
  }

  new_token (TK_EOF, cur, p);
  return head.next;
}

char *getTokenKind (Token *token) {
  if (token == NULL)
    return "NULL";

  switch (token->kind) {
  case TK_RESERVED: return "TK_RESERVED";
  case TK_NUM     : return "TK_NUM";
  case TK_EOF     : return "TK_EOF";
  }
}

void print_tokens (Token *head) {
  if (head == NULL)
    return;

  printf ("%s", getTokenKind (head));
  Token *cur = head->next;
  while (cur != NULL) {
    printf (" -> %s", getTokenKind (cur));
    cur = cur->next;
  }
  printf ("\n");
}


/*
 *  Parser
 */

// AST node type
typedef enum {
  ND_ADD,	// +
  ND_SUB,	// -
  ND_MUL,	// *
  ND_DIV,	// /
  ND_EQ,	// ==
  ND_NE,	// !=
  ND_LT,	// <
  ND_LE,	// <=
  ND_NUM,	// Integer
} NodeKind;


// AST node
struct Node {
  NodeKind kind;
  Node *lhs;	// left-hand side
  Node *rhs;	// right-hand side
  int val;	// used if kind == ND_NUM
};

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

Node *expr ();
Node *equality ();
Node *relational ();
Node *add ();
Node *mul ();
Node *unary ();
Node *primary ();

Node *expr () {
  return equality ();
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
  if (consume ("(")) {
    Node *node = expr ();
    expect (")");
    return node;
  }

  return new_node_num (expect_number ());
}


void gen (Node *node) {
  if (node == NULL)
    exit (1);

  if (node->kind == ND_NUM) {
    printf ("  push %d\n", node->val);
    return;
  }

  gen (node->lhs);
  gen (node->rhs);

  printf ("  pop rdi\n");
  printf ("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf ("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf ("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf ("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf ("  cqo\n");
    printf ("  idiv rdi\n");
    break;
  case ND_EQ:
    printf ("  cmp rax, rdi\n");
    printf ("  sete al\n");
    printf ("  movzb rax, al\n");
    break;
  case ND_NE:
    printf ("  cmp rax, rdi\n");
    printf ("  setne al\n");
    printf ("  movzb rax, al\n");
    break;
  case ND_LT:
    printf ("  cmp rax, rdi\n");
    printf ("  setl al\n");
    printf ("  movzb rax, al\n");
    break;
  case ND_LE:
    printf ("  cmp rax, rdi\n");
    printf ("  setle al\n");
    printf ("  movzb rax, al\n");
    break;
  }

  printf ("  push rax\n");
}


int
main (int argc, char *argv [])
{
  if (argc != 2) {
    fprintf (stderr, "usage: %s <number>\n", argv [0]);
    return 1;
  }

  user_input = argv [1];
  token = tokenize (user_input);
  Node *node = expr ();

  printf (".intel_syntax noprefix\n");
  printf (".globl main\n");
  printf ("main:\n");

  gen (node);

  printf ("  pop rax\n");
  printf ("  ret\n");

  return 0;
}
