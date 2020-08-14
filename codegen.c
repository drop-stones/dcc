#include "dcc.h"

char *argreg8 [] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
int labelseq = 0;


void gen_lval (Node *node) {
  if (node->kind != ND_LVAR)
    error ("Left value is not variable in assinment\n");

  printf ("  mov rax, rbp\n");
  printf ("  sub rax, %d\n", node->offset);
  printf ("  push rax\n");
}

void gen (Node *node) {
  if (node == NULL)
    exit (1);

  switch (node->kind) {
  case ND_NUM:
    printf ("  push %d\n", node->val);
    return;
  case ND_LVAR:
    gen_lval (node);
    printf ("  pop rax\n");
    printf ("  mov rax, [rax]\n");
    printf ("  push rax\n");
    return;
  case ND_ASSIGN:
    gen_lval (node->lhs);
    gen (node->rhs);

    printf ("  pop rdi\n");
    printf ("  pop rax\n");
    printf ("  mov [rax], rdi\n");
    printf ("  push rdi\n");
    return;
  case ND_RETURN:
    gen (node->lhs);

    printf ("  pop rax\n");
    printf ("  mov rsp, rbp\n");
    printf ("  pop rbp\n");
    printf ("  ret\n");
    return;
  case ND_IF: {
    int seq = labelseq++;
    gen (node->cond);
    printf ("  pop rax\n");
    printf ("  cmp rax, 0\n");
    if (node->els == NULL) {
      printf ("  je .Lend%03d\n", seq);
      gen (node->then);
      printf (".Lend%03d:\n", seq);
    } else {
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
    gen (node->body);
    printf ("  jmp .Lbegin%03d\n", seq);
    printf (".Lend%03d:\n", seq);
    return;
  }
  case ND_FOR: {
    int seq = labelseq++;
    if (node->init != NULL)
      gen (node->init);
    printf (".Lbegin%03d:\n", seq);
    //if (node->cond != NULL)
    gen (node->cond);
    printf ("  pop rax\n");
    printf ("  cmp rax, 0\n");
    printf ("  je .Lend%03d\n", seq);
    gen (node->body);
    gen (node->inc);
    printf ("  jmp .Lbegin%03d\n", seq);
    printf (".Lend%03d:\n", seq);
    return;
  }
  case ND_BLOCK:
    while (node->body != NULL) {
      gen (node->body);
      printf ("  pop rax\n");
      node->body = node->body->next;
    }
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
      printf ("  pop %s\n", argreg8 [i]);

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



