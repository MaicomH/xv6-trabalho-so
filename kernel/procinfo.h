struct procinfo {
  int pid;
  int ppid;          // pid do pai (0 se não tem)
  int state;         // valor do enum procstate
  uint64 sz;         // tamanho do espaço de endereçamento, em bytes
  char name[16];
};