#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "sysinfo.h"
#include "procinfo.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}
//seção do free
uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
uint64
sys_sysinfo(void)
{
  uint64 addr;                    // ponteiro do USUÁRIO: o kernel não pode desreferenciá-lo
  struct sysinfo si;
  struct proc *p = myproc();

  argaddr(0, &addr);              // 1º argumento, vindo de a0 no trapframe
  si.totalmem = ktotalmem();
  si.freemem  = kfreemem();
  si.nproc    = knproc();
  if(copyout(p->pagetable, p->sz, addr, (char*)&si, sizeof(si)) < 0)
    return -1;                    // endereço inválido vira erro, não panic
  return 0;
}
//fim da seção do free

//seção do hostname

#define HOSTNAME_MAX 64               // inclui o '\0': nome com no máximo 63 caracteres

static char hostname[HOSTNAME_MAX] = "xv6";
static struct spinlock hostname_lock;

void
hostnameinit(void)
{
  initlock(&hostname_lock, "hostname");
}

uint64
sys_gethostname(void)
{
  uint64 addr;                        // buffer do USUÁRIO
  int n, len;
  char tmp[HOSTNAME_MAX];             // cópia local, na pilha do kernel
  struct proc *p = myproc();

  argaddr(0, &addr);
  argint(1, &n);                      // tamanho do buffer do usuário

  acquire(&hostname_lock);
  len = strlen(hostname);
  memmove(tmp, hostname, len + 1);    // copia o '\0' também
  release(&hostname_lock);            // solta o lock ANTES do copyout

  if(n < len + 1)                     // buffer pequeno demais
    return -1;
  if(copyout(p->pagetable, p->sz, addr, tmp, len + 1) < 0)
    return -1;                        // endereço inválido
  return len;
}

uint64
sys_sethostname(void)
{
  char buf[HOSTNAME_MAX];
  int len;

  len = argstr(0, buf, HOSTNAME_MAX); // -1 se o ponteiro for inválido ou a string não caber
  if(len <= 0)                        // erro ou string vazia
    return -1;

  acquire(&hostname_lock);
  memmove(hostname, buf, len + 1);    // só altera o estado compartilhado com o lock
  release(&hostname_lock);
  return 0;
}

//fim da seção do hostname

//seção ps

uint64
sys_getprocs(void)
{
  uint64 addr;                      // vetor do USUÁRIO
  int max, n = 0, i;
  struct procinfo pi;               // uma entrada por vez, na pilha do kernel
  struct proc *p = myproc();

  argaddr(0, &addr);
  argint(1, &max);
  if(max <= 0)
    return -1;

  for(i = 0; i < NPROC && n < max; i++){
    if(kprocinfo(i, &pi) == 0)      // slot UNUSED: pula
      continue;
    // copyout FORA de qualquer lock
    if(copyout(p->pagetable, p->sz, addr + n * sizeof(pi), (char*)&pi, sizeof(pi)) < 0)
      return -1;
    n++;
  }
  return n;
}

// fim da seção ps