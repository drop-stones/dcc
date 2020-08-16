#include "dcc.h"

char *TokenKindStr [] = {
  "RESERVED", "IDENT", "NUM", "RETURN", "IF", "ELSE",
  "WHILE", "FOR", "EOF",
};

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
      strncmp (token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

Token *consume_keyword () {
  if (token->kind != TK_RETURN &&
      token->kind != TK_IF     &&
      token->kind != TK_ELSE   &&
      token->kind != TK_WHILE  &&
      token->kind != TK_FOR)
    return NULL;
  Token *tok_kw = token;
  token = token->next;
  return tok_kw;
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

// Ensure that the currect token is TK_IDENT.
// Return the ident name.
char *expect_ident (void) {
  if (token->kind != TK_IDENT)
    error_at (token->str, "expected an identifier");
  char *s = strndup (token->str, token->len);
  token = token->next;
  return s;
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
    } else if (strchr ("+-*/&()<>=;{},", *p)) {
      cur = new_token (TK_RESERVED, cur, p++);
      cur->len = 1;
    } else if (strncmp (p, "return", 6) == 0 && !isalnum (p[6])) {
      cur = new_token (TK_RETURN, cur, p);
      cur->len = 6;
      p += 6;
    } else if (strncmp (p, "if", 2) == 0 && !isalnum (p[2])) {
      cur = new_token (TK_IF, cur, p);
      cur->len = 2;
      p += 2;
    } else if (strncmp (p, "else", 4) == 0 && !isalnum (p[4])) {
      cur = new_token (TK_ELSE, cur, p);
      cur->len = 4;
      p += 4;
    } else if (strncmp (p, "while", 5) == 0 && !isalnum (p[5])) {
      cur = new_token (TK_WHILE, cur, p);
      cur->len = 5;
      p += 5;
    } else if (strncmp (p, "for", 3) == 0 && !isalnum (p[3])) {
      cur = new_token (TK_FOR, cur, p);
      cur->len = 3;
      p += 3;
    } else if (isdigit (*p)) {
      cur = new_token (TK_NUM, cur, p);
      cur->val = strtol (p, &p, 10);
    } else if (isalpha (*p)) {
      // ident
      char *q = p++;
      while (isalnum (*p))
        p++;
      cur = new_token (TK_IDENT, cur, q);
      cur->len = p - q;
    } else {
      error_at (p, "invalid token\n");
    }
  }

  new_token (TK_EOF, cur, p);
  return head.next;
}


void print_tokens (Token *head) {
  if (head == NULL)
    return;

  printf ("%s", TokenKindStr [head->kind]);
  Token *cur = head->next;
  while (cur != NULL) {
    printf (" -> %s", TokenKindStr [cur->kind]);
    cur = cur->next;
  }
  printf ("\n");
}

