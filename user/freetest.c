#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

static int falhas = 0;

static void
check(int cond, char *msg)
{
  printf("%s: %s\n", cond ? "OK   " : "FALHA", msg);
  if(!cond)
    falhas++;
}

static uint64
livre(void)
{
  struct sysinfo si;
  if(sysinfo(&si) < 0){
    printf("sysinfo falhou\n");
    exit(1);
  }
  return si.freemem;
}

static uint64
nprocs(void)
{
  struct sysinfo si;
  sysinfo(&si);
  return si.nproc;
}

int
main(void)
{
  struct sysinfo si;
  uint64 antes, depois, final, n1, n2;
  char *p;
  int i, pid, fd[2];

  // 1. sucesso e coerência
  check(sysinfo(&si) == 0, "sysinfo com ponteiro valido");
  check(si.freemem <= si.totalmem, "livre <= total");
  check(si.nproc >= 3, "nproc >= 3 (init, sh, freetest)");

  // 2. alocar 1 MB: a memória livre tem que cair, e voltar ao liberar
  antes = livre();
  p = sbrk(1 << 20);
  check(p != (char*)-1, "sbrk de 1 MB");
  for(i = 0; i < (1 << 20); i += 4096)
    p[i] = 1;                              // toca cada página (a alocação pode ser preguiçosa)
  depois = livre();
  check(antes - depois >= (1 << 20), "livre caiu pelo menos 1 MB");
  check(antes - depois <= (1 << 20) + 4 * 4096, "queda de ~1 MB (margem p/ tabelas de pagina)");
  sbrk(-(1 << 20));
  final = livre();
  check(final - depois >= (1 << 20), "livre recuperou 1 MB apos liberar");

  // 3. processos: um filho vivo aumenta nproc em 1
  n1 = nprocs();
  pipe(fd);
  pid = fork();
  if(pid == 0){
    char c;
    close(fd[1]);
    read(fd[0], &c, 1);                    // bloqueia até o pai fechar o pipe
    exit(0);
  }
  n2 = nprocs();
  check(n2 == n1 + 1, "filho vivo aumenta nproc em 1");
  close(fd[0]);
  close(fd[1]);                            // libera o filho
  wait(0);
  check(nprocs() == n1, "nproc volta ao normal apos wait");

  // 4. ponteiros inválidos: não pode dar panic
  check(sysinfo((struct sysinfo*)0xFFFFFFFF) < 0, "endereco nao mapeado retorna -1");
  check(sysinfo((struct sysinfo*)0) < 0, "endereco 0 (texto, somente leitura) retorna -1");

  printf(falhas ? "RESULTADO: %d falha(s)\n" : "RESULTADO: tudo certo\n", falhas);
  exit(falhas != 0);
}