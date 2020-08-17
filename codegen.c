#include "dcc.h"

static char *argreg [] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };

static int labelseq = 0;
static char *funcname;

static void gen (Node *node);

// Pushes the given node's address to the stack.
static void gen_addr (Node *node) {
  switch (node->kind) {
  case ND_VAR:
    printf ("  lea rax, [rbp-%d]\n", node->var->offset);
    printf ("  push rax\n");
    return;
  case ND_DEREF:
    gen (node->lhs);
    return;
  }

  error ("not an lvalue\n");
}

static void load (void) {
  printf ("  pop rax\n");
  printf ("  mov rax, [rax]\n");
  printf ("  push rax\n");
}

static void store (void) {
  printf ("  pop rdi\n");
  printf ("  pop rax\n");
  printf ("  mov [rax], rdi\n");
  printf ("  push rdi\n");
}

//void gen_lval (Node *node) {
//  if (node->kind != ND_VAR)
//    error ("Left value is not variable in assinment\n");
//
//  printf ("  mov rax, rbp\n");
//  printf ("  sub rax, %d\n", node->var->offset);
//  printf ("  push rax\n");
//}

static void gen (Node *node) {
  if (node == NULL)
    exit (1);

  switch (node->kind) {
  case ND_NULL:
    return;
  case ND_NUM:
    printf ("  push %d\n", node->val);
    return;
  case ND_EXPR_STMT:
    gen (node->lhs);
    printf ("  add rsp, 8\n"); // pop the stack top
    return;
  case ND_VAR:
    gen_addr (node);
    load ();
    return;
  case ND_ASSIGN:
    gen_addr (node->lhs);
    gen (node->rhs);
    store ();
    return;
  case ND_ADDR:
    gen_addr (node->lhs);
    return;
  case ND_DEREF:
    gen (node->lhs);
    load ();
    return;
  case ND_IF: {
    int seq = labelseq++;
    if (node->els == NULL) {
      gen (node->cond);
      printf ("  pop rax\n");
      printf ("  cmp rax, 0\n");
      printf ("  je .Lend%03d\n", seq);
      gen (node->then);
      printf (".Lend%03d:\n", seq);
    } else {
      gen (node->cond);
      printf ("  pop rax\n");
      printf ("  cmp rax, 0\n");
      printf ("  je .Lelse%03d\n", seq);
      gen (node->then);
      printf ("  jmp .Lend%03d\n", seq);
      printf (".Lelse%03d:\n", seq);
      gen (node->els);
      printf (".Lend%03d:\n", seq);
    }
    return;
  }
  case ND_WHILE: {
    int seq = labelseq++;
    printf (".Lbegin%03d:\n", seq);
    gen (node->cond);
    printf ("  pop rax\n");
    printf ("  cmp rax, 0\n");
    printf ("  je .Lend%03d\n", seq);
    gen (node->then);
    printf ("  jmp .Lbegin%03d\n", seq);
    printf (".Lend%03d:\n", seq);
    return;
  }
  case ND_FOR: {
    int seq = labelseq++;
    if (node->init != NULL)
      gen (node->init);
    printf (".Lbegin%03d:\n", seq);
    if (node->cond != NULL) {
      gen (node->cond);
      printf ("  pop rax\n");
      printf ("  cmp rax, 0\n");
      printf ("  je .Lend%03d\n", seq);
    }
    gen (node->then);
    if (node->inc != NULL)
      gen (node->inc);
    printf ("  jmp .Lbegin%03d\n", seq);
    printf (".Lend%03d:\n", seq);
    return;
  }
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen (n);
    return;
  case ND_FUNCALL: {
    // argument
    int nargs = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
      gen (arg);
      nargs++;
    }

    // Assume: args <= 6
    for (int i = nargs - 1; i >= 0; i--)
      printf ("  pop %s\n", argreg [i]);

    // We need to align rsp to a 16 byte boundary before
    // calling a function because of an ABI requirement.
    int seq = labelseq++;
    printf ("  mov rax, rsp\n");
    printf ("  and rax, 15\n");
    printf ("  jnz .L.call.%d\n", seq);
    printf ("  mov rax, 0\n");
    printf ("  call %s\n", node->funcname);
    printf ("  jmp .L.end.%d\n", seq);
    printf (".L.call.%d:\n", seq);
    printf ("  sub rsp, 8\n");
    printf ("  mov rax, 0\n");
    printf ("  call %s\n", node->funcname);
    printf ("  add rsp, 8\n");
    printf (".L.end.%d:\n", seq);
    printf ("  push rax\n");

    return;
  }
  case ND_RETURN:
    gen (node->lhs);
    printf ("  pop rax\n");
    printf ("  jmp .L.return.%s\n", funcname);
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

void codegen (Function *prog) {
  printf (".intel_syntax noprefix\n");

  for (Function *fn = prog; fn; fn = fn->next) {
    printf (".global %s\n", fn->name);
    printf ("%s:\n", fn->name);
    funcname = fn->name;

    // Prologue
    printf ("  push rbp\n");
    printf ("  mov rbp, rsp\n");
    printf ("  sub rsp, %d\n", fn->stack_size);

    // Push arguments to the stack
    int i = 0;
    for (VarList *vl = fn->params; vl; vl = vl->next) {
      Var *var = vl->var;
      printf ("  mov [rbp-%d], %s\n", var->offset, argreg [i++]);
    }

    // Emit code
    for (Node *node = fn->node; node; node = node->next)
      gen (node);

    // Epilogue
    printf (".L.return.%s:\n", funcname);
    printf ("  mov rsp, rbp\n");
    printf ("  pop rbp\n");
    printf ("  ret\n");
  }
}

