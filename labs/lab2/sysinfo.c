// sysproc.c

uint64 sys_sysinfo(void) {
  uint64 addr;
  if (argaddr(0, &addr) < 0) {
    return -1;
  }
  struct sysinfo info;
  info.freemem = freemem();
  info.nproc = nproc();
  if (copyout(myproc()->pagetable, addr, (char *)(&info), sizeof(info)) < 0) {
    return -1;
  }
  return 0;
}

// kalloc.c

uint64 freemem(void) {
  struct run *r = kmem.freelist;
  uint64 cnt = 0;
  while (r != NULL) {
    cnt++;
    r = r->next;
  }
  return cnt * PGSIZE;
}

// proc.c

uint64 nproc(void) {
  uint64 cnt = 0;
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&(p->lock));
    if (p->state == UNUSED) {
      cnt++;
    }
    release(&(p->lock));
  }
  return cnt;
}
