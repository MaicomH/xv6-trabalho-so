#include "kernel/types.h"
#include "user/user.h"

static int
parse_mask(const char *s, int *out)
{
  uint64 val = 0;

  if(s == 0 || *s == '\0')
    return -1;
  while(*s){
    if(*s < '0' || *s > '9')
      return -1;
    val = val * 10 + (*s - '0');
    if(val > 0x7fffffff)
      return -1;
    s++;
  }
  if(val == 0)
    return -1;
  *out = (int)val;
  return 0;
}

int
main(int argc, char *argv[])
{
  int mask;

  if(argc < 3){
    fprintf(2, "uso: strace mascara comando [args...]\n");
    exit(1);
  }
  if(parse_mask(argv[1], &mask) < 0){
    fprintf(2, "strace: mascara invalida: %s\n", argv[1]);
    exit(1);
  }
  if(trace(mask) < 0){
    fprintf(2, "strace: trace falhou\n");
    exit(1);
  }
  exec(argv[2], &argv[2]);          // a máscara sobrevive ao exec: é o mesmo struct proc
  fprintf(2, "strace: exec %s falhou\n", argv[2]);
  exit(1);
}