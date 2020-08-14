#include "dcc.h"

char *func_arg_reg [] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
int L_count = 0;


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

  int Label, i;
  Node *cur;
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
  case ND_IF:
    Label = L_count++;
    gen (node->cond);
    printf ("  pop rax\n");
    printf ("  cmp rax, 0\n");
    if (node->els == NULL) {
      printf ("  je .Lend%03d\n", Label);
      gen (node->then);
      printf (".Lend%03d:\n", Label);
    } else {
      printf ("  je .Lelse%03d\n", Label);
      gen (node->then);
      printf ("  jmp .Lend%03d\n", Label);
      printf (".Lelse%03d:\n", Label);
      gen (node->els);
      printf (".Lend%03d:\n", Label);
    }
    return;
  case ND_WHILE:
    Label = L_count++;
    printf (".Lbegin%03d:\n", Label);
    gen (node->cond);
    printf ("  pop rax\n");
    printf ("  cmp rax, 0\n");
    printf ("  je .Lend%03d\n", Label);
    gen (node->body);
    printf ("  jmp .Lbegin%03d\n", Label);
    printf (".Lend%03d:\n", Label);
    return;
  case ND_FOR:
    Label = L_count++;
    if (node->init != NULL)
      gen (node->init);
    printf (".Lbegin%03d:\n", Label);
    //if (node->cond != NULL)
    gen (node->cond);
    printf ("  pop rax\n");
    printf ("  cmp rax, 0\n");
    printf ("  je .Lend%03d\n", Label);
    gen (node->body);
    gen (node->inc);
    printf ("  jmp .Lbegin%03d\n", Label);
    printf (".Lend%03d:\n", Label);
    return;
  case ND_BLOCK:
    while (node->body != NULL) {
      gen (node->body);
      printf ("  pop rax\n");
      node->body = node->body->next;
    }
    return;
  case ND_FUNCALL:
    // argument
    cur = node->args;
    for (i = 0; cur; i++) {
      printf ("  mov %s, %d\n", func_arg_reg [i], cur->val);
      cur = cur->next;
    }
    printf ("  call %s\n", node->funcname);
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



