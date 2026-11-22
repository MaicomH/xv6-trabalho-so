#include "kernel/types.h"
#include "user/user.h"

#define MAX 64                          // mesmo limite do kernel (HOSTNAME_MAX)

int
main(int argc, char *argv[])
{
  char buf[MAX];

  if(argc == 1){                        // "hostname": só mostra
    if(gethostname(buf, sizeof(buf)) < 0){
      fprintf(2, "hostname: gethostname falhou\n");
      exit(1);
    }
    printf("%s\n", buf);
    exit(0);
  }

  if(argc == 2){                        // "hostname nome": altera
    if(sethostname(argv[1]) < 0){
      fprintf(2, "hostname: nome invalido (1 a %d caracteres)\n", MAX - 1);
      exit(1);
    }
    exit(0);
  }

  fprintf(2, "uso: hostname [novo_nome]\n");
  exit(1);
}