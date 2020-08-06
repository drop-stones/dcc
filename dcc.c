#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef enum {
  TK_RESERVED,	// operation
  TK_NUM, 	// number
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

// error function
// Take same arguments as printf.
void error (char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
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
    error ("'%c' is wrong.", op);
  token = token->next;
}

// return num from sequence
int expect_number () {
  if (token->kind != TK_NUM)
    error ("Not number.");
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
    // skip white space
    if (isspace (*p)) {
      p++;
    } else if (*p == '+' || *p == '-') {
      cur = new_token (TK_RESERVED, cur, p++);
    } else if (isdigit (*p)) {
      cur = new_token (TK_NUM, cur, p);
      cur->val = strtol (p, &p, 10);
    } else {
      error ("Cannot tokeninze.\n");
    }
  }

  new_token (TK_EOF, cur, p);
  return head.next;
}

int
main (int argc, char *argv [])
{
  if (argc != 2) {
    error ("usage: %s <number>\n", argv [0]);
    return 1;
  }

  token = tokenize (argv [1]);

  printf (".intel_syntax noprefix\n");
  printf (".globl main\n");
  printf ("main:\n");

  printf ("  mov rax, %d\n", expect_number ());

  while (!at_eof ()) {
    if (consume ('+'))
      printf ("  add rax, %d\n", expect_number ());
    else if (consume ('-'))
      printf ("  sub rax, %d\n", expect_number ());
    else
      error ("operation is wrong");
  }

  printf ("  ret\n");

  return 0;
}
