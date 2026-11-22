#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int mask;

  if(argc < 3){
    fprintf(2, "uso: strace mascara comando [args...]\n");
    exit(1);
  }
  mask = atoi(argv[1]);
  if(mask <= 0){
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