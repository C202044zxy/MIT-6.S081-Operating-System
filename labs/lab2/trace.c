// sysproc.c

uint64 sys_trace(void) {
  int mask;
  if (argint(0, &mask) < 0) {
    return -1;
  }
  myproc()->mask = mask;
  return 0;
}

// syscall.c

void syscall(void) {
  int num;
  struct proc *p = myproc();
  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num]();
    if ((p->mask & (1 << num)) > 0) {
      // pid syscall_name return_val
      printf("%d: syscall %s -> %d\n", p->pid,
          sysCallName[num], p->trapframe->a0);
    }
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}

// proc.c

int fork(void)
{
  ...
  // copy trace mask
  np->mask = p->mask;
  ...
}