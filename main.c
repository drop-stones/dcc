#include "dcc.h"

char *read_file (char *path) {
  // open file
  FILE *fp = fopen (path, "r");
  if (!fp)
    error ("cannot open %s: %s", path, strerror (errno));

  // file size
  if (fseek (fp, 0, SEEK_END) == -1)
    error ("%s: fseek: %s", path, strerror (errno));
  size_t size = ftell (fp);
  if (fseek (fp, 0, SEEK_SET) == -1)
    error ("%s: fseek: %s", path, strerror (errno));

  //size_t size = 10 * 1024 * 1024;

  // read file contents
  char *buf = calloc (1, size + 2);
  fread (buf, size, 1, fp);

  if (size == 0 || buf [size - 1] != '\n')
    buf [size++] = '\n';
  buf [size] = '\0';
  fclose (fp);
  return buf;
}

int align_to (int n, int align) {
  return (n + align - 1) & ~(align - 1);
}

int
main (int argc, char *argv [])
{
  if (argc != 2) {
    fprintf (stderr, "usage: %s <number>\n", argv [0]);
    return 1;
  }

  filename = argv [1];
  user_input = read_file (filename);
  token = tokenize ();
  Program *prog = program ();

  for (Function *fn = prog->fns; fn; fn = fn->next) {
    int offset = 0;
    for (VarList *vl = fn->locals; vl; vl = vl->next) {
      Var *var = vl->var;
      offset += var->ty->size;
      var->offset = offset;
    }
    fn->stack_size = align_to (offset, 8);
  }

  codegen (prog);

  return 0;
}

