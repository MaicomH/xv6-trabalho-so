# Declaração de Uso de Modelos de Linguagem (LLMs)

Conforme a Seção 4 do enunciado (*Declaração de uso de LLM*), este documento registra de forma transparente como ferramentas de Inteligência Artificial foram utilizadas durante o desenvolvimento, revisão e documentação deste trabalho.

> **Nota para o grupo:** as versões exatas do Claude Code e do Gemini não foram informadas; o enunciado só exige isso "quando disponível", então deixem assim se não souberem precisar. Se quiserem mais detalhe nos itens 1 e 4 da Seção 3 (prompts específicos, como já há nos itens 2 e 3), é só complementar — não é obrigatório para a entrega.

---

## 1. Ferramentas, Modelos e Integrantes

| Ferramenta / Plataforma | Modelo / Versão | Integrante(s) | Finalidade principal |
| :--- | :--- | :--- | :--- |
| Claude Code | (versão não registrada pelo grupo) | Todos os integrantes (Andreza Ercílio, Eduardo Gabriel, Maicom Howie) | Geração inicial dos 4 comandos e das 5 syscalls — commits `21b38d8` (free/sysinfo), `caebe6b` (hostname/gethostname/sethostname), `78c34a3` (ps/getprocs) e `b5e3cb2` (strace/trace) —, incluindo o estudo de traps, despacho de syscalls e ordem de locks necessário para a implementação. |
| Google Antigravity (CLI) | Gemini 4 Neon (Medium) | Todos os integrantes (Andreza Ercílio, Eduardo Gabriel, Maicom Howie) | Aperfeiçoamento do código gerado: revisão frente ao enunciado, identificação de pontos de atenção, correções, ampliação de testes e estruturação inicial da documentação (commit `2eb6ed4`). |
| Gemini | (versão não registrada pelo grupo) | Todos os integrantes (Andreza Ercílio, Eduardo Gabriel, Maicom Howie) | Revisão de texto da documentação já estruturada, principalmente deste arquivo (`USO-LLM.md`) e do `README-TRABALHO.md`. |
| Claude.ai (chat) | Claude Sonnet 5 | Todos os integrantes (Andreza Ercílio, Eduardo Gabriel, Maicom Howie) | Nova conferência do `README-TRABALHO.md`/`USO-LLM.md` contra o código já alterado, após a geração inicial destes documentos, e preenchimento da identificação do grupo (ver Seção 3, item 5). |

---

## 2. Onde foi utilizada

- **Estudo e planejamento:** compreensão do mecanismo de traps, despacho de syscalls em `kernel/syscall.c`, ordem de travamento `wait_lock -> p->lock` em `kernel/proc.c` e uso seguro de `copyout`/`argstr` — com apoio do Claude Code, durante a implementação inicial dos 4 comandos.
- **Implementação de código:**
  - Comandos e syscalls afetados: `free` (`sysinfo`), `hostname` (`gethostname`/`sethostname`), `ps` (`getprocs`) e `strace` (`trace`).
  - Geração inicial com Claude Code (commits `21b38d8`, `caebe6b`, `78c34a3`, `b5e3cb2`).
  - Arquivos revisados/corrigidos depois, com apoio do Google Antigravity (Gemini 4 Neon):
    - `kernel/proc.c` (`allocproc`)
    - `kernel/kalloc.c` (remoção de protótipos duplicados)
    - `kernel/syscall.c` (`syscall` — guarda de shift `num < 31` e formato `%ld`)
    - `user/user.h` (limpeza de declarações duplicadas)
    - `user/strace.c` (`parse_mask` para validação estrita da máscara numérica)
- **Testes:**
  - `user/hostnametest.c` — adição de casos-limite (tamanho negativo, endereço `0` somente leitura) e restauração do hostname padrão `"xv6"` ao final para garantir idempotência.
  - `user/stracetest.c` — adição de testes para máscara máxima de 31 bits, execução do binário `strace` com argumentos válidos e inválidos (sobrevivência da máscara após `exec`) e concorrência com múltiplos filhos rastreados.
  - (Ambos ampliados na etapa de aperfeiçoamento, com o Google Antigravity.)
- **Documentação:**
  - `README-TRABALHO.md` e `USO-LLM.md` — estruturação inicial com apoio do Google Antigravity, a partir do código-base e dos requisitos do enunciado; revisão de texto com o Gemini, principalmente deste `USO-LLM.md`; revisada e completada pelo grupo.
  - Nova revisão (Claude, nesta sessão, depois de alterações no código): conferência de cada afirmação técnica do `README-TRABALHO.md` contra o código atual e preenchimento da identificação do grupo; nenhuma divergência foi encontrada, então nenhum conteúdo técnico foi reescrito (ver Seção 3, item 5).

---

## 3. Como foi utilizada

### Consultas relevantes e decisões do grupo

1. **Geração inicial dos 4 comandos (Claude Code):**
   - **Objetivo:** Implementar as syscalls `sysinfo`, `gethostname`/`sethostname`, `getprocs` e `trace`, os quatro programas de usuário e os testes iniciais, a partir das interfaces sugeridas no Anexo A do enunciado.
   - **Resultado:** Código inicial funcional para os quatro comandos, integrado em `kernel/syscall.h`, `kernel/syscall.c`, `kernel/sysproc.c`, `kernel/proc.c`, `user/user.h`, `user/usys.pl` e no `Makefile` (commits `21b38d8`, `caebe6b`, `78c34a3`, `b5e3cb2`).
   - **Decisão do grupo:** Aceito como base e revisado/corrigido nas etapas seguintes (itens 2 e 3 abaixo).

2. **Auditoria do projeto contra o enunciado (`enunciado_trabalho_xv6_llm.pdf`):**
   - **Objetivo:** Verificar se os 4 comandos implementados atendiam aos requisitos de comportamento real no kernel, proteção de memória, sincronização, testes e entregáveis obrigatórios.
   - **Resultado:** Identificou-se que os 4 comandos e 5 syscalls já estavam implementados, mas faltavam os entregáveis de documentação (`README-TRABALHO.md`, `USO-LLM.md`, relatório em PDF), havia correções pendentes não commitadas e lacunas nos testes de `strace` e `hostname`.
   - **Decisão do grupo:** Aceito integralmente.

3. **Revisão de validação de argumentos e casos-limite:**
   - **Objetivo:** Melhorar a robustez de `user/strace.c`, `user/stracetest.c`, `user/hostnametest.c` e `kernel/proc.c`.
   - **Sugestões aceitas/adaptadas:**
     - Substituir `atoi()` em `user/strace.c` por `parse_mask()`, rejeitando entradas parcialmente numéricas (ex.: `"12abc"`) e overflow acima de `0x7fffffff`.
     - Inicializar explicitamente `p->tracemask = 0` em `allocproc()` (`kernel/proc.c`), além da limpeza em `freeproc()`.
     - Adicionar testes de `gethostname(buf, -1)`, `gethostname((char*)0, MAX)` e restauração de `"xv6"` em `user/hostnametest.c`.
     - Expandir `user/stracetest.c` para testar o comando `strace` via `fork` + `exec` e concorrência de processos rastreados.
   - **Sugestões rejeitadas/ajustadas:**
     - Não adicionar teste esperando erro em `sethostname((char*)0)`, pois no xv6-RISC-V o endereço virtual `0` pertence ao segmento `.text` mapeado com permissão de leitura (`PTE_R | PTE_U`), logo `copyinstr` consegue lê-lo (diferente de `copyout`, que exige `PTE_W` e falha corretamente).

4. **Revisão de texto da documentação (Gemini):**
   - **Objetivo:** Revisar a redação do `README-TRABALHO.md` e, principalmente, deste `USO-LLM.md`, verificando clareza e aderência aos itens exigidos na Seção 4 do enunciado.
   - **Resultado:** Ajustes de redação e organização nos dois documentos.
   - **Decisão do grupo:** Aceito.

5. **Nova conferência do projeto após alterações posteriores no código (esta sessão, com o Claude no chat):**
   - **Objetivo:** Depois que o código foi alterado após a geração inicial do `README-TRABALHO.md` e do `USO-LLM.md`, verificar se as duas entregas ainda descreviam corretamente o comportamento do kernel e preencher a identificação do grupo.
   - **Método:** releitura de `kernel/syscall.h`, `kernel/syscall.c`, `kernel/sysproc.c`, `kernel/proc.c`, `kernel/proc.h`, `kernel/kalloc.c`, `kernel/defs.h`, `kernel/main.c`, `user/user.h`, `user/usys.pl`, `Makefile` e dos quatro programas de usuário com seus testes; comparação de cada afirmação do `README-TRABALHO.md` com o trecho de código correspondente; conferência de `git log` e `git diff base-original HEAD` para validar a lista de arquivos modificados e a ausência dos comandos/syscalls no commit-base.
   - **Resultado:** nenhuma divergência encontrada entre a documentação e o código (detalhes na Seção 4). O conteúdo técnico alterado foi a identificação dos integrantes (antes em branco) e a estrutura deste documento, para refletir as ferramentas de LLM usadas em cada etapa.
   - **Decisão do grupo:** Aceito.

---

## 4. Verificação das Sugestões no Código-Base e nos Testes

Todas as sugestões aceitas foram verificadas diretamente contra o código-base do xv6-RISC-V e os requisitos do enunciado:

- **Assinaturas e comportamento interno do kernel:**
  - Conferido em `kernel/vm.c` que `copyout` nesta versão do xv6 recebe 5 argumentos (`pagetable, psz, dstva, src, len`).
  - Conferido em `kernel/printk.c` que o especificador `%ld` é suportado nativamente para impressão de `long`/`uint64` em `kernel/syscall.c`.
  - Conferida a ordem de travamento em `kernel/proc.c`: `kprocinfo()` adquire `wait_lock` antes de `p->lock` (mesma ordem usada em `kwait()`/`reparent()`), e `sys_getprocs()` executa `copyout` fora de qualquer lock.
- **Erros identificados e corrigidos durante a revisão:**
  - Protótipos duplicados no fim de `kernel/kalloc.c` e em `user/user.h` removidos.
  - Possível *undefined behavior* no deslocamento `p->tracemask >> num` em `kernel/syscall.c` corrigido com a guarda `(num < 31)`.
  - Aceitação indevida de máscaras malformadas (como `"12abc"`) pelo `atoi()` em `user/strace.c` corrigida com `parse_mask()`.
  - Falta de idempotência em `user/hostnametest.c` (que deixava o hostname modificado ao final) corrigida com restauração para `"xv6"`.
- **Validação em execução:**
  - Compilação limpa executada com `make clean && make kernel/kernel fs.img` sem erros nem warnings (`-Wall -Werror`).
  - Executados no QEMU (`QEMU 8.2.2`, `riscv64-linux-gnu-gcc 13.3.0`, `Ubuntu 24.04.4 LTS`): `free`, `hostname`, `ps`, `strace 8388608 free`, `freetest`, `hostnametest`, `pstest`, `stracetest` (todos retornando `RESULTADO: tudo certo`) e `usertests -q` (`ALL TESTS PASSED`), confirmando ausência de regressões no xv6 original.
- **Nova conferência estática feita nesta sessão (Claude, sem recompilar nem reexecutar o xv6):**
  - Conferido em `kernel/vm.c` que `copyout` recusa a escrita quando `(*pte & PTE_W) == 0`, e que `copyin`/`copyinstr` não exigem essa permissão — confirma por que o endereço `0` (início do segmento `.text`, mapeado a partir de `0x0` em `user/user.ld` com permissão de leitura/execução) é aceito por `copyinstr` em `sethostname` mas rejeitado por `copyout` em `gethostname`, sustentando a justificativa do item 3 acima.
  - Conferido com `git diff --stat base-original HEAD` que a lista de "Arquivos Modificados e Criados" do `README-TRABALHO.md` corresponde exatamente aos 23 arquivos alterados (21 de código/build, mais os dois documentos).
  - Conferido com `git show base-original:kernel/syscall.h` e `git ls-tree base-original -- user/` que nenhum dos quatro comandos nem as cinco syscalls existia no commit-base, validando o item 1 dos requisitos de implementação.
  - Esta conferência foi estática (leitura do código e do histórico do git); não substitui a recompilação limpa (`make clean && make qemu`) que o grupo deve rodar antes da entrega.
