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
  Function *prog = program ();

  for (Function *fn = prog; fn; fn = fn->next) {
    int offset = 0;
    for (LVar *lvar = prog->locals; lvar; lvar = lvar->next) {
      offset += 8;
      lvar->offset = offset;
    }
    fn->stack_size = offset;
  }

  codegen (prog);

  return 0;
}

