#include "kernel/types.h"
#include "user/user.h"

static int falhas = 0;

static void
check(int cond, char *msg)
{
  printf("%s: %s\n", cond ? "OK   " : "FALHA", msg);
  if(!cond)
    falhas++;
}

int
main(void)
{
  int pid, st, fd[2];
  char c;

  // 1. contrato da syscall
  check(trace(0) == 0, "trace(0) desliga o rastreamento");
  check(trace(-1) < 0, "mascara negativa retorna -1");

  // 2. rastrear não muda o resultado das chamadas
  trace(1 << 11);                         // só getpid (11)
  pid = getpid();
  trace(0);
  check(pid > 0, "getpid continua correto com trace ligado");

  // 3. herança: o filho rastreado funciona e termina normalmente
  pipe(fd);
  trace(1 << 5);                          // só read (5)
  pid = fork();
  if(pid == 0){
    close(fd[1]);
    read(fd[0], &c, 1);                   // deve aparecer como "read" do filho
    exit(7);
  }
  trace(0);                               // desliga no pai; o filho mantém a máscara
  close(fd[0]);
  write(fd[1], "x", 1);
  close(fd[1]);
  wait(&st);
  check(st == 7, "filho rastreado termina com o status esperado");

  printf(falhas ? "RESULTADO: %d falha(s)\n" : "RESULTADO: tudo certo\n", falhas);
  exit(falhas != 0);
}