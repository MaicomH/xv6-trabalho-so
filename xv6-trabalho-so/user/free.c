#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int
main(void)
{
  struct sysinfo si;

  if(sysinfo(&si) < 0){
    fprintf(2, "free: sysinfo falhou\n");
    exit(1);
  }
  printf("total: %d KB\nlivre: %d KB\nusada: %d KB\nprocs: %d\n",
         (int)(si.totalmem / 1024),
         (int)(si.freemem / 1024),
         (int)((si.totalmem - si.freemem) / 1024),
         (int)si.nproc);
  exit(0);
}