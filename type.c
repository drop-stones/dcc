#include "dcc.h"

bool is_integer (Type *ty) {
  return ty->kind == TY_INT;
}

Type *pointer_to (Type *base) {
  Type *ty = calloc (1, sizeof (Type));
  ty->kind = TY_PTR;
  ty->base = base;
  return ty;
}