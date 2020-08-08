#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 *  Tokenizer
 */

typedef enum {
  TK_RESERVED,	// operation
  TK_NUM, 	// number
  TK_PAR,	// parentheses
  TK_EOF,	// End of File
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *str;
};

// current Token
Token *token;
char *user_input;

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
bool consume (char op) {
  if (token->kind != TK_RESERVED || token->str [0] != op)
    return false;
  token = token->next;
  return true;
}

// move to next Token from Token sequence
void expect (char op) {
  if (token->kind != TK_RESERVED || token->str [0] != op)
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
    //} else if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
    } else if (strchr ("+-*/()", *p)) {
      cur = new_token (TK_RESERVED, cur, p++);
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


/*
 *  Parser
 */

// AST node type
typedef enum {
  ND_ADD,	// +
  ND_SUB,	// -
  ND_MUL,	// *
  ND_DIV,	// /
  ND_NUM,	// number
} NodeKind;

typedef struct Node Node;

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

Node *mul ();
Node *unary ();
Node *primary ();

Node *expr () {
  Node *node = mul ();

  for (;;) {
    if (consume ('+'))
      node = new_node (ND_ADD, node, mul ());
    else if (consume ('-'))
      node = new_node (ND_SUB, node, mul ());
    else
      return node;
  }
}

Node *mul () {
  Node *node = unary ();

  for (;;) {
    if (consume ('*'))
      node = new_node (ND_MUL, node, unary ());
    else if (consume ('/'))
      node = new_node (ND_DIV, node, unary ());
    else
      return node;
  }
}

Node *unary () {
  if (consume ('+'))
    return primary ();
  if (consume ('-'))
    return new_node (ND_SUB, new_node_num (0), primary ());
  return primary ();
}

Node *primary () {
  if (consume ('(')) {
    Node *node = expr ();
    expect (')');
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
