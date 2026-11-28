#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/procinfo.h"
#include "user/user.h"

// índices iguais ao enum procstate de kernel/proc.h
static char *estados[] = { "unused", "used", "sleep", "runble", "run", "zombie" };

static struct procinfo lista[NPROC];   // global: não ocupa a pilha do usuário

int
main(void)
{
  int n, i;

  n = getprocs(lista, NPROC);
  if(n < 0){
    fprintf(2, "ps: getprocs falhou\n");
    exit(1);
  }
  printf("PID\tPPID\tESTADO\tMEM(KB)\tNOME\n");
  for(i = 0; i < n; i++){
    char *est = (lista[i].state >= 0 && lista[i].state <= 5) ? estados[lista[i].state] : "?";
    printf("%d\t%d\t%s\t%d\t%s\n", lista[i].pid, lista[i].ppid, est,
           (int)(lista[i].sz / 1024), lista[i].name);
  }
  exit(0);
}