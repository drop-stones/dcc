#include "dcc.h"


int
main (int argc, char *argv [])
{
  if (argc != 2) {
    fprintf (stderr, "usage: %s <number>\n", argv [0]);
    return 1;
  }

  user_input = argv [1];
  token = tokenize (user_input);
  program ();

  printf (".intel_syntax noprefix\n");
  printf (".globl main\n");
  printf ("main:\n");

  // prologue
  printf ("  push rbp\n");
  printf ("  mov rbp, rsp\n");
  printf ("  sub rsp, %d\n", locals->offset);

  for (int i = 0; code [i] != NULL; i++) {
    gen (code [i]);

    // pop result of statement
    printf ("  pop rax\n");
  }

  // epilogue
  printf ("  mov rsp, rbp\n");
  printf ("  pop rbx\n");
  printf ("  ret\n");

  return 0;
}

