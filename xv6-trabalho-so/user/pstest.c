#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/procinfo.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

#define SLEEPING 2      // valores do enum procstate (kernel/proc.h)
#define ZOMBIE   5

static struct procinfo lista[NPROC];
static int falhas = 0;

static void
check(int cond, char *msg)
{
  printf("%s: %s\n", cond ? "OK   " : "FALHA", msg);
  if(!cond)
    falhas++;
}

static struct procinfo *
achar(int n, int pid)
{
  int i;
  for(i = 0; i < n; i++)
    if(lista[i].pid == pid)
      return &lista[i];
  return 0;
}

int
main(void)
{
  struct sysinfo si;
  struct procinfo *e;
  int n, n0, i, j, k, pid, fd[2], ok;
  char c;
  uint64 antes;

  // 1. coerência básica
  n0 = getprocs(lista, NPROC);
  check(n0 >= 3, "getprocs retorna pelo menos 3 processos");
  e = achar(n0, getpid());
  check(e != 0, "o proprio processo aparece na lista");
  check(e && strcmp(e->name, "pstest") == 0, "nome do processo correto");
  sysinfo(&si);
  check((uint64)n0 == si.nproc, "getprocs e sysinfo contam o mesmo numero de processos");

  // 2. relação pai/filho e estados
  pipe(fd);
  pid = fork();
  if(pid == 0){
    close(fd[1]);
    read(fd[0], &c, 1);             // bloqueia até o pai fechar o pipe
    exit(0);
  }
  close(fd[0]);
  pause(10);                         // dá tempo do filho bloquear no read
  n = getprocs(lista, NPROC);
  e = achar(n, pid);
  check(e != 0, "filho vivo aparece na lista");
  check(e && e->ppid == getpid(), "ppid do filho e o pid do pai");
  check(e && e->state == SLEEPING, "filho bloqueado em read esta SLEEPING");
  close(fd[1]);                      // o filho recebe EOF e sai
  pause(10);                         // o pai ainda não fez wait: o filho vira zumbi
  n = getprocs(lista, NPROC);
  e = achar(n, pid);
  check(e && e->state == ZOMBIE, "filho encerrado sem wait esta ZOMBIE");
  wait(0);
  n = getprocs(lista, NPROC);
  check(achar(n, pid) == 0, "apos wait o filho some da lista");

  // 3. memória: a coluna de memória reflete o sbrk
  n = getprocs(lista, NPROC);
  e = achar(n, getpid());
  antes = e ? e->sz : 0;
  sbrk(1 << 20);
  n = getprocs(lista, NPROC);
  e = achar(n, getpid());
  check(e && e->sz >= antes + (1 << 20), "sz cresce 1 MB apos sbrk");
  sbrk(-(1 << 20));
  n = getprocs(lista, NPROC);
  e = achar(n, getpid());
  check(e && e->sz == antes, "sz volta ao valor anterior apos liberar");

  // 4. limites e argumentos inválidos
  check(getprocs(lista, 1) == 1, "max=1 copia exatamente 1 entrada");
  check(getprocs(lista, 0) < 0, "max=0 retorna -1");
  check(getprocs(lista, -5) < 0, "max negativo retorna -1");
  check(getprocs((struct procinfo*)0xFFFFFFFF, NPROC) < 0, "endereco nao mapeado retorna -1");
  check(getprocs((struct procinfo*)0, NPROC) < 0, "endereco 0 (texto somente leitura) retorna -1");

  // 5. concorrência: 20 filhos nascendo e morrendo enquanto o pai lê a tabela
  for(i = 0; i < 20; i++){
    if(fork() == 0){
      pause(i % 3);
      exit(0);
    }
  }
  ok = 1;
  for(j = 0; j < 200; j++){
    n = getprocs(lista, NPROC);
    if(n < 0){
      ok = 0;
      break;
    }
    for(i = 0; i < n; i++){
      if(lista[i].pid <= 0 || lista[i].state < 1 || lista[i].state > 5)
        ok = 0;
      for(k = i + 1; k < n; k++)
        if(lista[k].pid == lista[i].pid)
          ok = 0;                    // pid repetido = leitura inconsistente
    }
  }
  while(wait(0) > 0)
    ;
  check(ok, "concorrencia: entradas validas e sem pid repetido");
  n = getprocs(lista, NPROC);
  check(n == n0, "sem processos residuais ao final");

  printf(falhas ? "RESULTADO: %d falha(s)\n" : "RESULTADO: tudo certo\n", falhas);
  exit(falhas != 0);
}