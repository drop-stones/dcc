#include "dcc.h"

static char *argreg1 [] = { "dil", "sil", "dl", "cl", "r8b", "r9b" };
static char *argreg8 [] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };

static int labelseq = 0;
static char *funcname;

static void gen (Node *node);

// Pushes the given node's address to the stack.
static void gen_addr (Node *node) {
  switch (node->kind) {
  case ND_VAR: {
    Var *var = node->var;
    if (var->is_local) {
      printf ("  lea rax, [rbp-%d]\n", node->var->offset);
      printf ("  push rax\n");
    } else {
      printf ("  push offset %s\n", var->name);
    }
    return;
  }
  case ND_DEREF:
    gen (node->lhs);
    return;
  }

  error_tok (node->tok, "not an lvalue\n");
}

static void gen_lval (Node *node) {
  if (node->ty->kind == TY_ARRAY)
    error_tok (node->tok, "not an lvalue");
  gen_addr (node);
}

static void load (Type *ty) {
  printf ("  pop rax\n");
  if (ty->size == 1)
    printf ("  movsx rax, BYTE PTR [rax]\n");
  else
    printf ("  mov rax, [rax]\n");
  printf ("  push rax\n");
}

static void store (Type *ty) {
  printf ("  pop rdi\n");
  printf ("  pop rax\n");
  if (ty->size == 1)
    printf ("  mov [rax], dil\n");
  else
    printf ("  mov [rax], rdi\n");
  printf ("  push rdi\n");
}

static void gen (Node *node) {
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
    if (node->ty->kind != TY_ARRAY)
      load (node->ty);
    return;
  case ND_ASSIGN:
    gen_lval (node->lhs);
    gen (node->rhs);
    store (node->ty);
    return;
  case ND_ADDR:
    gen_addr (node->lhs);
    return;
  case ND_DEREF:
    gen (node->lhs);
    if (node->ty->kind != TY_ARRAY)
      load (node->ty);
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
  case ND_STMT_EXPR:
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
  case ND_PTR_ADD:
    printf ("  imul rdi, %d\n", node->ty->base->size);
    printf ("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf ("  sub rax, rdi\n");
    break;
  case ND_PTR_SUB:
    printf ("  imul rdi, %d\n", node->ty->base->size);
    printf ("  sub rax, rdi\n");
    break;
  case ND_PTR_DIFF:
    printf ("  sub rax, rdi\n");
    printf ("  cqo\n");
    printf ("  mov rdi, %d\n", node->lhs->ty->base->size);
    printf ("  idiv rdi\n");
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

static void load_arg (Var *var, int idx) {
  int sz = var->ty->size;
  if (sz == 1) {
    printf ("  mov [rbp-%d], %s\n", var->offset, argreg1 [idx]);
  } else {
    assert (sz == 8);
    printf ("  mov [rbp-%d], %s\n", var->offset, argreg8 [idx]);
  }
}

static void emit_data (Program *prog) {
  printf ("  .data\n");

  for (VarList *vl = prog->globals; vl; vl = vl->next) {
    Var *var = vl->var;
    printf ("%s:\n", var->name);
    if (var->contents)
      for (int i = 0; i < var->cont_len; i++)
        printf ("  .byte 0x%x\n", var->contents [i]);
    else if (var->val)
      printf ("  .long %d\n", var->val);
    else if (var->int_arr)
      for (int i = 0; i < var->ty->size; i++)
        //printf ("  .long %d\n", var->int_arr [i]);
        printf ("  .quad %d\n", var->int_arr [i]);
    else
      printf ("  .zero %d\n", var->ty->size);
  }
}

static void emit_text (Program *prog) {
  printf ("  .text\n");

  for (Function *fn = prog->fns; fn; fn = fn->next) {
    printf (".global %s\n", fn->name);
    printf ("%s:\n", fn->name);
    funcname = fn->name;

    // Prologue
    printf ("  push rbp\n");
    printf ("  mov rbp, rsp\n");
    printf ("  sub rsp, %d\n", fn->stack_size);

    // Push arguments to the stack
    int i = 0;
    for (VarList *vl = fn->params; vl; vl = vl->next)
      load_arg (vl->var, i++);

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

void codegen (Program *prog) {
  printf (".intel_syntax noprefix\n");
  emit_data (prog);
  emit_text (prog);
}

