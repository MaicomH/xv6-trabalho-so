struct sysinfo {
  uint64 totalmem;  // bytes de RAM gerenciada pelo alocador
  uint64 freemem;   // bytes livres agora
  uint64 nproc;     // processos que não estão UNUSED
};