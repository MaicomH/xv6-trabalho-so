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

static int
rodar(char *prog, char *argv[])
{
  int pid, st;

  pid = fork();
  if(pid < 0)
    return -1;
  if(pid == 0){
    exec(prog, argv);
    exit(127);
  }
  wait(&st);
  return st;
}

int
main(void)
{
  int pid, st, i, ok, fd[2];
  char c;

  // 1. contrato da syscall
  check(trace(0) == 0, "trace(0) desliga o rastreamento");
  check(trace(-1) < 0, "mascara negativa retorna -1");
  check(trace(0x7fffffff) == 0 && trace(0) == 0, "mascara maxima valida (31 bits) aceita");

  // 2. rastrear não muda o resultado das chamadas
  trace(1 << 11);                         // só getpid (11)
  pid = getpid();
  trace(0);
  check(pid > 0, "getpid continua correto com trace ligado");

  // 3. herança no fork: o filho mantém a máscara mesmo se o pai desligar
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

  // 4. programa strace: sucesso (sobrevivência da máscara após exec) e argumentos inválidos
  {
    char *ok_argv[] = { "strace", "8388608", "free", 0 };  // 1 << 23 (SYS_sysinfo)
    char *sem_args[] = { "strace", 0 };
    char *mask_zero[] = { "strace", "0", "free", 0 };
    char *mask_neg[] = { "strace", "-1", "free", 0 };
    char *mask_txt[] = { "strace", "abc", "free", 0 };
    char *mask_mista[] = { "strace", "12abc", "free", 0 };
    char *cmd_inv[] = { "strace", "2048", "nao_existe_cmd", 0 };

    check(rodar("strace", ok_argv) == 0, "strace com mascara valida executa comando com sucesso");
    check(rodar("strace", sem_args) != 0, "strace sem argumentos retorna erro");
    check(rodar("strace", mask_zero) != 0, "strace com mascara 0 retorna erro");
    check(rodar("strace", mask_neg) != 0, "strace com mascara negativa retorna erro");
    check(rodar("strace", mask_txt) != 0, "strace com mascara nao numerica retorna erro");
    check(rodar("strace", mask_mista) != 0, "strace com mascara parcialmente numerica retorna erro");
    check(rodar("strace", cmd_inv) != 0, "strace com comando inexistente retorna erro");
  }

  // 5. concorrência: múltiplos filhos rastreados simultaneamente
  for(i = 0; i < 4; i++){
    if(fork() == 0){
      if(trace(1 << 11) < 0)              // só getpid (11)
        exit(1);
      if(getpid() <= 0)
        exit(2);
      exit(0);
    }
  }
  ok = 1;
  for(i = 0; i < 4; i++){
    wait(&st);
    if(st != 0)
      ok = 0;
  }
  check(ok, "concorrencia: 4 filhos rastreados simultaneamente");

  printf(falhas ? "RESULTADO: %d falha(s)\n" : "RESULTADO: tudo certo\n", falhas);
  exit(falhas != 0);
}