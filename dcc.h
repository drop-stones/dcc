#define _GNU_SOURCE
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;

//
// tokenize.c
//

// Token
typedef enum {
  TK_RESERVED,	// Keywords or punctuators
  TK_IDENT,	// Identifiers
  TK_STR,	// String literals
  TK_NUM, 	// Number literals
  TK_EOF,	// End of File
} TokenKind;


// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // Token kind
  Token *next;	  // Next token
  int val;	  // If kind is TK_NUM, its value
  char *str;	  // Token string
  int len;	  // Token length

  char *contents; // String literal contents including terminating '\0'
  int cont_len;	  // String literal length
};

void error (char *fmt, ...);
void error_at (char *loc, char *fmt, ...);
void error_tok (Token *tok, char *fmt, ...);
Token *peek (char *s);
Token *consume (char *op);
Token *consume_ident (void);
Token *consume_sizeof (void);
void expect (char *op);
int expect_number (void);
char *expect_ident (void);
bool at_eof (void);
Token *tokenize (void);
void print_tokens (Token *head);

extern char *filename;
extern char *user_input;
extern Token *token;
extern char *TokenKindStr [];


//
//  parse.c
//

// Variable
typedef struct Var Var;
struct Var {
  char *name;
  Type *ty;
  bool is_local;

  // Local variable
  int offset;	// Offset from rbp

  // Global variable

  // Integer
  int val;

  // Pointer
  //void *ptr;
  int *int_ptr;

  // Array
  //void *arr;
  int *int_arr;

  // String literal
  char *contents;
  int cont_len;
};

typedef struct VarList VarList;
struct VarList {
  VarList *next;
  Var *var;
};

// AST node
typedef enum {
  ND_NULL,	// nop
  ND_ADD,	// num + num
  ND_PTR_ADD,	// ptr + num or num + ptr
  ND_SUB,	// num - num
  ND_PTR_SUB,	// ptr - num
  ND_PTR_DIFF,	// ptr - ptr
  ND_MUL,	// *
  ND_DIV,	// /
  ND_EQ,	// ==
  ND_NE,	// !=
  ND_LT,	// <
  ND_LE,	// <=
  ND_ADDR,	// unary &
  ND_DEREF,	// unary *
  ND_ASSIGN,	// =
  ND_VAR,	// Local variable
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
  ND_EXPR_STMT,	// expression statement
  ND_STMT_EXPR,	// statement expression
} NodeKind;


// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node *next;
  Type *ty;	// Type, e.g. int or pointer to int
  Token *tok;	// Representative token

  Node *lhs;	// left-hand side
  Node *rhs;	// right-hand side

  // "if", "while", or "for" statement
  Node *cond;
  Node *then;
  Node *els;
  Node *init;
  Node *inc;

  // Block or Statement-expression
  Node *body;

  // Function call
  char *funcname;
  Node *args;

  int val;	// used if kind == ND_NUM
  Var *var;	// used if kind == ND_LVAR
};


typedef struct Function Function;
struct Function {
  Function *next;
  char *name;
  Type *ty;
  VarList *params;

  Node *node;
  VarList *locals;
  int stack_size;
};


typedef struct Program Program;
struct Program {
  VarList  *globals;
  Function *fns;
};

Program *program (void);

//
// type.c
//

typedef enum {
  TY_CHAR,
  TY_INT,
  TY_PTR,
  TY_ARRAY,
} TypeKind;

struct Type {
  TypeKind kind;
  int size;	// sizeof () value
  Type *base;
  int array_len;
};

extern Type *char_type;
extern Type *int_type;

bool is_integer (Type *ty);
Type *pointer_to (Type *base);
Type *array_of (Type *base, int len);
void add_type (Node *node);

/*
 *  codegen.c
 */

void codegen (Program *prog);
