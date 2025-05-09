#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void child(int rd) {
  int p[2] = {};
  int num = 0, prime = 0, setNext = 0;
  read(rd, &prime, 4); // read the prime
  printf("prime %d\n", prime);
  // pass the next numbers
  while (read(rd, &num, 4) != 0) {
    if (num % prime == 0) {
      // not a prime
      continue; 
    }
    if (setNext == 0) {
      setNext = 1;
      // create the pipe.
      // p[0] for reading, p[1] for writing.
      pipe(p);
      if (fork() == 0) {
        // close the writing end in child process.
        // this does not interfere file descriptor table of parent process.
        close(p[1]);
        child(p[0]);
        close(p[0]);
        return ;
      } else {
        // colse the reading end in parent process.
        // this does not interfere file descriptor table of child process. 
        close(p[0]);
      }
    }
    // write to the pipe
    write(p[1], &num, 4);
  }
  if (p[1]) {
    close(p[1]);
  }
  wait(0); // wait for child to complete
}

int main(int agrc, char* argv[]) {
  int p[2] = {};
  pipe(p);
  if (fork() == 0) {
    close(p[1]);
    child(p[0]);
    close(p[0]);
  } else {
    close(p[0]);
    for (int i = 2; i <= 35; i++) {
      write(p[1], &i, 4);
    }
    close(p[1]);
    wait(0); // wait for child to complete
  }
  exit(0);
}