# Trabalho Prático de Sistemas Operacionais — Extensão do xv6-RISC-V

Este documento descreve a extensão do sistema operacional **xv6-RISC-V** com quatro novos comandos de usuário (**`free`**, **`hostname`**, **`ps`** e **`strace`**) e cinco novas chamadas ao sistema (**`sysinfo`**, **`gethostname`**, **`sethostname`**, **`getprocs`** e **`trace`**).

---

## 1. Identificação do Grupo e Commit-Base

- **Integrantes:**
  - Andreza Ercílio — matrícula 20220022773
  - Eduardo Gabriel — matrícula 20210037825
  - Maicom Howie — matrícula 20220011949
- **Repositório base:** `https://github.com/mit-pdos/xv6-riscv`
- **Commit-base original:** `fa2ddcb` (tag `base-original` — *"usertests: bound the zero-length pipe read regression test"*)
- **Verificação de ausência na versão base:** Confirmado no commit `fa2ddcb` que nenhum dos quatro comandos (`free`, `hostname`, `ps`, `strace`) nem as cinco syscalls (`sysinfo`, `gethostname`, `sethostname`, `getprocs`, `trace`) existiam no xv6-RISC-V original.

---

## 2. Comandos, Chamadas ao Sistema e Interfaces

| Comando | Syscall(s) | Número | Classificação | Resumo da funcionalidade |
| :--- | :--- | :---: | :--- | :--- |
| `free` | `sysinfo` | 23 | Inspirada no Linux (adaptada) | Exibe memória física total, livre e usada (em KB) e número de processos ativos a partir do alocador `kalloc` e da tabela `proc`. |
| `hostname` | `gethostname`, `sethostname` | 24, 25 | Inspiradas no Linux (simplificadas) | Consulta e altera o nome do sistema armazenado no kernel, com proteção por spinlock (`hostname_lock`). |
| `ps` | `getprocs` | 26 | **Interface didática** | Lista PID, PPID, estado, tamanho em memória (KB) e nome dos processos ativos. No Linux, `ps` lê `/proc`; não há syscall `getprocs`. |
| `strace` | `trace` | 27 | **Interface didática** | Rastreia chamadas ao sistema de um processo e seus filhos por máscara de bits no dispatcher. No Linux, `strace` usa `ptrace`. |

### 2.1. `free` — `int sysinfo(struct sysinfo *info)`

- **Estrutura compartilhada** (`kernel/sysinfo.h`):
  ```c
  struct sysinfo {
    uint64 totalmem;  // bytes de RAM gerenciados pelo alocador kalloc (PHYSTOP - PGROUNDUP(end))
    uint64 freemem;   // bytes atualmente livres na freelist do kalloc
    uint64 nproc;     // quantidade de slots em proc[] com state != UNUSED
  };
  ```
- **Unidades:** A syscall retorna memória em **bytes**; o programa `free` converte para **KB** (`valor / 1024`).
- **Retorno e erros:** Retorna `0` em sucesso e `-1` se `copyout` falhar (ponteiro não mapeado, fora do espaço do usuário ou em página somente leitura como endereço `0`).
- **Sincronização e proteção de memória:** `kfreemem()` adquire `kmem.lock` para percorrer `kmem.freelist`; `knproc()` adquire `p->lock` por entrada da tabela `proc`; `copyout` é chamado fora de qualquer lock.
- **Simplificação frente ao Linux:** No Linux, `free` consulta `/proc/meminfo` (incluindo buffers, cache e swap) e `sysinfo(2)` expõe uptime, loads, swap etc. No xv6 não há swap nem page cache separado, então `struct sysinfo` expõe apenas os dados reais mantidos pelo kernel (`totalmem`, `freemem`, `nproc`).

### 2.2. `hostname` — `int gethostname(char *buf, int n)` e `int sethostname(const char *name)`

- **Estado no kernel** (`kernel/sysproc.c`): `static char hostname[64] = "xv6";` protegido por `hostname_lock` (inicializado em `main()` via `hostnameinit()`).
- **`gethostname(buf, n)`:**
  - Copia o hostname atual (incluindo `'\0'`) para um buffer local na pilha do kernel sob `hostname_lock`, libera o lock e então chama `copyout`.
  - **Retorno e erros:** Retorna o comprimento da string (`strlen(hostname)`) em sucesso; retorna `-1` se `n < len + 1` (incluindo `n <= 0`) ou se `copyout` falhar.
- **`sethostname(name)`:**
  - Lê a string do usuário via `argstr(0, buf, HOSTNAME_MAX)` (usa `copyinstr`).
  - **Limites:** Aceita nomes de **1 a 63 caracteres** (`HOSTNAME_MAX = 64` contando `'\0'`).
  - **Retorno e erros:** Retorna `0` em sucesso; retorna `-1` se o ponteiro for inválido, a string for vazia (`len == 0`) ou exceder 63 caracteres. Em caso de erro, o nome anterior é preservado.
- **Simplificações frente ao Linux:**
  - No Linux, `sethostname(const char *name, size_t len)` recebe tamanho explícito e exige privilégio `CAP_SYS_ADMIN`; no xv6 não há modelo de usuários/permissões, e `sethostname` recebe uma string terminada em `'\0'`.
  - O hostname reside apenas em memória volátil do kernel (não persiste após reinicialização do xv6).

### 2.3. `ps` — `int getprocs(struct procinfo *buf, int max)` *(interface didática)*

- **Estrutura compartilhada** (`kernel/procinfo.h`):
  ```c
  struct procinfo {
    int pid;
    int ppid;          // PID do pai (0 se não houver pai)
    int state;         // valor do enum procstate (kernel/proc.h)
    uint64 sz;         // tamanho do espaço de endereçamento em bytes
    char name[16];
  };
  ```
- **Argumentos e retorno:** Preenche até `max` entradas em `buf` e retorna o número de processos efetivamente copiados (`>= 0`). Retorna `-1` se `max <= 0` ou se `copyout` falhar em qualquer posição.
- **Sincronização e proteção de memória:** `kprocinfo(i, &pi)` adquire `wait_lock` **antes** de `p->lock` (respeitando a ordem global de travamento do xv6 para evitar deadlock) ao ler `p->parent->pid`, `p->state`, `p->pid`, `p->sz` e `p->name`. O `copyout` é executado fora de qualquer lock.
- **Diferença frente ao Linux:** No Linux, `ps` lê o sistema de arquivos virtual `/proc`. No xv6, foi criada a syscall didática `getprocs` que exporta apenas os campos seguros em `struct procinfo`, sem expor ponteiros internos do kernel.

### 2.4. `strace` — `int trace(int mask)` *(interface didática)*

- **Atributo no processo** (`kernel/proc.h`): campo `int tracemask` em `struct proc`.
  - Inicializado com `0` em `allocproc()` e limpo em `freeproc()`.
  - **Herança:** Copiado do pai para o filho em `kfork()` (`np->tracemask = p->tracemask`).
  - **Sobrevivência ao `exec`:** Como `exec` substitui apenas a imagem de usuário mantendo o mesmo `struct proc`, a máscara configurada por `strace` permanece ativa durante a execução do comando alvo.
- **Argumentos e retorno:** `mask` é uma máscara de bits onde o bit `(1 << SYS_xxx)` ativa o rastreamento da syscall `SYS_xxx`. `trace(0)` desliga o rastreamento. Retorna `0` em sucesso e `-1` se `mask < 0` (bit 31 ligado).
- **Comportamento no dispatcher** (`kernel/syscall.c`):
  - Verifica `(num < 31) && ((p->tracemask >> num) & 1)`.
  - Como `SYS_exit` não retorna, seu registro é impresso antes da execução (`<pid> <nome>: syscall exit`).
  - Para as demais syscalls, imprime após a execução: `<pid> <nome>: syscall <nome_syscall> -> <retorno>`.
- **Programa `strace`:** `strace <mascara> <comando> [args...]`. Valida rigorosamente `<mascara>` (apenas dígitos decimais, positiva, dentro de 31 bits), chama `trace(mask)` e executa `<comando>` via `exec`.
- **Diferença frente ao Linux:** No Linux, `strace` é implementado com `ptrace(2)`, interceptando um processo externo. A syscall didática `trace` é autoaplicada via máscara de bits e herdada por `fork`/`exec`.

---

## 3. Arquivos Modificados e Criados

### Kernel
- `kernel/syscall.h` — numeração das syscalls 23 a 27 (`SYS_sysinfo`, `SYS_gethostname`, `SYS_sethostname`, `SYS_getprocs`, `SYS_trace`).
- `kernel/syscall.c` — tabela `syscalls[]`, tabela `syscall_names[]` e lógica de rastreamento no dispatcher `syscall()`.
- `kernel/defs.h` — protótipos de `ktotalmem`, `kfreemem`, `knproc`, `kprocinfo` e `hostnameinit`.
- `kernel/sysinfo.h` *(novo)* — definição de `struct sysinfo`.
- `kernel/procinfo.h` *(novo)* — definição de `struct procinfo`.
- `kernel/kalloc.c` — implementação de `ktotalmem()` e `kfreemem()`.
- `kernel/main.c` — chamada a `hostnameinit()` na inicialização do kernel.
- `kernel/proc.h` — campo `int tracemask` em `struct proc`.
- `kernel/proc.c` — inicialização/limpeza/herança de `tracemask` (`allocproc`, `freeproc`, `kfork`) e implementação de `knproc()` e `kprocinfo()`.
- `kernel/sysproc.c` — implementação de `sys_sysinfo`, `hostnameinit`, `sys_gethostname`, `sys_sethostname`, `sys_getprocs` e `sys_trace`.

### Espaço de Usuário e Build
- `user/user.h` — declarações das 5 novas syscalls e forward declarations de `struct sysinfo` e `struct procinfo`.
- `user/usys.pl` — geração dos stubs RISC-V (`li a7, SYS_xxx; ecall; ret`) para as 5 novas syscalls.
- `Makefile` — inclusão dos programas e testes em `UPROGS`.
- `user/free.c`, `user/hostname.c`, `user/ps.c`, `user/strace.c` *(novos)* — programas de usuário.
- `user/freetest.c`, `user/hostnametest.c`, `user/pstest.c`, `user/stracetest.c` *(novos)* — suítes de testes automatizados.

---

## 4. Instruções de Compilação e Execução

### Versões das ferramentas utilizadas
- **GCC RISC-V:** `riscv64-linux-gnu-gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`
- **QEMU:** `QEMU emulator version 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18)`
- **Sistema Operacional:** `Ubuntu 24.04.4 LTS`

### Compilação limpa e execução
```bash
make clean
make qemu
```
Para testar em configuração de CPU única (útil para analisar determinismo/disputa):
```bash
make qemu CPUS=1
```
Para sair do QEMU: `Ctrl-A` seguido de `X`.

---

## 5. Exemplos de Uso e Resultados Esperados

No shell do xv6 (`$`):

### `free`
```text
$ free
total: 130956 KB
livre: 129660 KB
usada: 1296 KB
procs: 3
```
*(Os valores exatos variam conforme o tamanho do kernel e processos em execução.)*

### `hostname`
```text
$ hostname
xv6
$ hostname meu-xv6
$ hostname
meu-xv6
```

### `ps`
```text
$ ps
PID     PPID    ESTADO  MEM(KB) NOME
1       0       sleep   16      init
2       1       sleep   20      sh
3       2       run     16      ps
```

### `strace`
Rastreando `sysinfo` (syscall 23 -> `1 << 23 = 8388608`) ao executar `free`:
```text
$ strace 8388608 free
4 free: syscall sysinfo -> 0
total: 130956 KB
livre: 129660 KB
usada: 1296 KB
procs: 3
```
Rastreando todas as syscalls (`2147483647` = `0x7fffffff`) em `hostname`:
```text
$ strace 2147483647 hostname
```

### Execução dos testes automatizados
```text
$ freetest
...
RESULTADO: tudo certo

$ hostnametest
...
RESULTADO: tudo certo

$ pstest
...
RESULTADO: tudo certo

$ stracetest
...
RESULTADO: tudo certo
```

### Verificação de regressão do xv6 original
```text
$ usertests
...
ALL TESTS PASSED
```

---

## 6. Limitações Conhecidas

1. **Hostname volátil:** O nome definido por `sethostname` fica apenas em memória do kernel e retorna para `"xv6"` ao reiniciar o emulador.
2. **Sem controle de permissões em `sethostname`:** Como o xv6 não possui usuários/credenciais, qualquer processo pode alterar o hostname.
3. **Snapshot não atômico em `getprocs`:** Cada entrada da tabela `proc[]` é lida com locks consistentes (`wait_lock` e `p->lock`), mas a tabela inteira não fica travada simultaneamente durante a varredura; processos criados/encerrados durante a chamada podem ou não aparecer naquela leitura.
4. **Máscara de `trace` limitada a 31 bits:** `tracemask` é um `int` com sinal (bits 0..30), suficiente para as 27 syscalls atuais, mas exigiria `uint64` caso o kernel ultrapasse 30 syscalls.
5. **Opções de linha de comando simplificadas:** `free` e `ps` não implementam flags do Linux (como `free -m` ou `ps aux`), e `strace` recebe máscara numérica inteira em vez de nomes de syscalls (`-e trace=...`).
