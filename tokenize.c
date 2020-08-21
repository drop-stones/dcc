#include "dcc.h"

char *TokenKindStr [] = {
  "RESERVED", "IDENT", "NUM", "EOF",
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

// Reports ana error location and exit.
static void verror_at (char *loc, char *fmt, va_list ap) {
  int pos = loc - user_input;
  fprintf (stderr, "%s\n", user_input);
  fprintf (stderr, "%*s", pos, "");
  fprintf (stderr, "^ ");
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
  verror_at (loc, fmt, ap);
}

// Reports an error location and exit.
void error_tok (Token *tok, char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
  verror_at (tok->str, fmt, ap);
}

// Check whether the currect token matches a given string.
Token *peek (char *s) {
  if (token->kind != TK_RESERVED ||
      strlen (s)  != token->len  ||
      strncmp (token->str, s, token->len))
    return NULL;
  return token;
}

// Consume one Token from Token sequence
Token *consume (char *op) {
  if (token->kind != TK_RESERVED ||
      strlen (op) != token->len  ||
      strncmp (token->str, op, token->len))
    return NULL;
  Token *tok = token;
  token = token->next;
  return tok;
}


Token *consume_ident (void) {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *tok_ident = token;
  token = token->next;
  return tok_ident;
}

//Token *consume_sizeof (void) {
//  if (token->kind != TK_SIZEOF)
//    return NULL;
//  Token *tok_sizeof = token;
//  token = token->next;
//  return tok_sizeof;
//}

// move to next Token from Token sequence
void expect (char *s) {
  if (!peek (s))
    error_tok (token, "expected i\"%s\"", s);
  token = token->next;
}

// return num from sequence
int expect_number (void) {
  if (token->kind != TK_NUM)
    error_tok (token, "expected a number");
  int val = token->val;
  token = token->next;
  return val;
}

// Ensure that the currect token is TK_IDENT.
// Return the ident name.
char *expect_ident (void) {
  if (token->kind != TK_IDENT)
    error_tok (token, "expected an identifier");
  char *ident_name = strndup (token->str, token->len);
  token = token->next;
  return ident_name;
}

bool at_eof () {
  return token->kind == TK_EOF;
}

// create new Token and append it to cur
static Token *new_token (TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc (1, sizeof (Token));
  tok->kind = kind;
  tok->str  = str;
  tok->len  = len;
  cur->next = tok;
  return tok;
}

static bool startswith (char *p, char *q) {
  return strncmp (p, q, strlen (q)) == 0;
}

static bool is_alpha (char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alnum (char c) {
  return is_alpha (c) || ('0' <= c && c <= '9');
}

static char *starts_with_reserved (char *p) {
  // Keyword
  static char *kw [] = { "return", "if", "else", "while", "for", "int", "sizeof" };

  for (int i = 0; i < sizeof (kw) / sizeof (*kw); i++) {
    int len = strlen (kw [i]);
    if (startswith (p, kw [i]) && !is_alnum (p [len]))
      return kw [i];
  }

  // Multi-letter punctuator
  static char *ops [] = { "==", "!=", "<=", ">=" };

  for (int i = 0; i < sizeof (ops) / sizeof (*ops); i++)
    if (startswith (p, ops [i]))
      return ops [i];

  return NULL;
}

// Tokenize string
Token *tokenize (void) {
  char *p = user_input;
  Token head = {};
  Token *cur = &head;

  char *kw;
  while (*p) {
    if (isspace (*p)) {
      // skip white space
      p++;
    } else if ((kw = starts_with_reserved (p)) != NULL) {
      // Keywords or multi-letter punctuators
      int len = strlen (kw);
      cur = new_token (TK_RESERVED, cur, p, len);
      p += len;
    //} else if (!strncmp (p, "sizeof", 6)) {
    //  cur = new_token (TK_SIZEOF, cur, p, 6);
    //  p += 6;
    } else if (ispunct (*p)) {
      // Single-letter punctuators
      cur = new_token (TK_RESERVED, cur, p++, 1);
    } else if (is_alpha (*p)) {
      // identifier
      char *q = p++;
      while (is_alnum (*p))
        p++;
      cur = new_token (TK_IDENT, cur, q, p - q);
    } else if (isdigit (*p)) {
      // Integer literal
      cur = new_token (TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol (p, &p, 10);
      cur->len = p - q;
    } else {
      error_at (p, "invalid token\n");
    }
  }

  // EOF
  new_token (TK_EOF, cur, p, 0);
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

