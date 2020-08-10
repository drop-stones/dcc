#include "dcc.h"

Token *token;
char *user_input;

// Reports an error and exit.
void error (char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  exit (1);
}

// Reports an error message in the following format.
//
// foo.c:10: x = y + 1;
//               ^ <error message here>
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

Token *consume_ident () {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *tok_ident = token;
  token = token->next;
  return tok_ident;
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
    } else if (strchr ("+-*/()<>=;", *p)) {
      cur = new_token (TK_RESERVED, cur, p++);
      cur->len = 1;
    } else if (isdigit (*p)) {
      cur = new_token (TK_NUM, cur, p);
      cur->val = strtol (p, &p, 10);
    } else if ('a' <= *p && *p <= 'z') {
      cur = new_token (TK_IDENT, cur, p++);
      cur->len = 1;
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
  case TK_IDENT   : return "TK_IDENT";
  case TK_NUM     : return "TK_NUM";
  case TK_EOF     : return "TK_EOF";
  }
  return "error";
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

