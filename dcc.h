#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 *  tokenize.c
 */

// Token
typedef enum {
  TK_RESERVED,	// operation
  TK_IDENT,	// identifier
  TK_NUM, 	// number
  TK_RETURN,	// return
  TK_IF,	// if
  TK_ELSE,	// else
  TK_WHILE,	// while
  TK_FOR,	// for
  TK_EOF,	// End of File
} TokenKind;


// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *str;
  int len;
};

void error (char *fmt, ...);
void error_at (char *loc, char *fmt, ...);
bool consume (char *op);
Token *consume_keyword();
Token *consume_ident ();
void expect (char *op);
int expect_number ();
bool at_eof ();
Token *tokenize (char *p);
void print_tokens (Token *head);

extern char *user_input;
extern Token *token;
extern char *TokenKindStr [];


/*
 *  parse.c
 */

// Local variable type
typedef struct LVar LVar;
struct LVar {
  LVar *next;	// next variable
  char *name;
  int  len;
  int offset;	// offset from rbp
};

// AST node
typedef enum {
  ND_ADD,	// +
  ND_SUB,	// -
  ND_MUL,	// *
  ND_DIV,	// /
  ND_EQ,	// ==
  ND_NE,	// !=
  ND_LT,	// <
  ND_LE,	// <=
  ND_ASSIGN,	// =
  ND_LVAR,	// Local variable
  ND_FUNCALL,	// function call
  ND_NUM,	// Integer
  ND_RETURN,	// return
  ND_IF,	// if
  ND_ELSE,	// else
  ND_WHILE,	// while
  ND_FOR,	// for
  ND_DO,	// do
  ND_SWITCH,	// switch
  ND_CASE,	// case
  ND_BLOCK,	// {}
} NodeKind;


// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node *next;

  Node *lhs;	// left-hand side
  Node *rhs;	// right-hand side

  // "if", "while", or "for" statement
  Node *cond;
  Node *then;
  Node *els;
  Node *init;
  Node *inc;

  Node *body;

  // used if kind == FUNCALL
  char *funcname;
  Node *args;

  int val;	// used if kind == ND_NUM
  int offset;	// used if kind == ND_LVAR
};

//Node *expr ();
void program ();

extern Node *code [100];
extern LVar *locals;

/*
 *  codegen.c
 */

void gen (Node *node);
