#include "kernel/types.h"
#include "user/user.h"

#define MAX 64

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
  char buf[MAX], longo[100];
  int i, j, pid, st, ok;

  // 1. nome inicial
  check(gethostname(buf, MAX) == 3 && strcmp(buf, "xv6") == 0, "nome padrao xv6");

  // 2. sucesso: set + get
  check(sethostname("lumie") == 0, "sethostname valido");
  check(gethostname(buf, MAX) == 5 && strcmp(buf, "lumie") == 0, "gethostname le o novo nome");

  // 3. estado fica no kernel: a alteração de um filho é vista pelo pai
  pid = fork();
  if(pid == 0){
    sethostname("filho");
    exit(0);
  }
  wait(0);
  gethostname(buf, MAX);
  check(strcmp(buf, "filho") == 0, "alteracao do filho visivel no pai");

  // 4. limites
  for(i = 0; i < 63; i++) longo[i] = 'a';
  longo[63] = 0;
  check(sethostname(longo) == 0, "63 caracteres aceito");
  for(i = 0; i < 64; i++) longo[i] = 'a';
  longo[64] = 0;
  check(sethostname(longo) < 0, "64 caracteres rejeitado");
  check(sethostname("") < 0, "string vazia rejeitada");
  gethostname(buf, MAX);
  check(strlen(buf) == 63, "nome anterior preservado apos erro");

  // 5. buffer pequeno ou negativo
  sethostname("abcdef");
  check(gethostname(buf, 3) < 0, "buffer pequeno retorna -1");
  check(gethostname(buf, 0) < 0, "buffer de tamanho 0 retorna -1");
  check(gethostname(buf, -1) < 0, "buffer de tamanho negativo retorna -1");

  // 6. ponteiros inválidos: o kernel não pode dar panic
  check(gethostname((char*)0xFFFFFFFF, MAX) < 0, "gethostname com ponteiro nao mapeado");
  check(gethostname((char*)0, MAX) < 0, "gethostname com endereco 0 (texto somente leitura)");
  check(sethostname((char*)0xFFFFFFFF) < 0, "sethostname com ponteiro nao mapeado");

  // 7. concorrência: 4 filhos gravam nomes de letras iguais e conferem que
  //    nunca leem um nome "misturado" (leitura rasgada)
  for(i = 0; i < 4; i++){
    if(fork() == 0){
      char nome[MAX], lido[MAX];
      int k;
      for(j = 0; j < 62; j++) nome[j] = 'a' + i;
      nome[62] = 0;
      for(j = 0; j < 2000; j++){
        sethostname(nome);
        gethostname(lido, MAX);
        for(k = 1; lido[k]; k++){
          if(lido[k] != lido[0]){
            printf("FALHA: leitura misturada: %s\n", lido);
            exit(1);
          }
        }
      }
      exit(0);
    }
  }
  ok = 1;
  for(i = 0; i < 4; i++){
    wait(&st);
    if(st != 0)
      ok = 0;
  }
  check(ok, "concorrencia: sem leituras misturadas");

  // 8. restaura o nome padrão para manter o teste idempotente
  check(sethostname("xv6") == 0, "restaura hostname padrao xv6");

  printf(falhas ? "RESULTADO: %d falha(s)\n" : "RESULTADO: tudo certo\n", falhas);
  exit(falhas != 0);
}