#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
  int p2c[2];
  int c2p[2];
  pipe(p2c);
  pipe(c2p);
  // p[0] for reading
  // p[1] for writing 

  if (fork() == 0) {
    // child process
    char buf[10];
    read(p2c[0], buf, 1);   // read from parent process through read end.
    fprintf(1, "%d: received ping\n", getpid());
    write(c2p[1], buf, 1);  // write to parent process through write end.
  } else {
    // parent process
    char buf[10];
    write(p2c[1], buf, 1);  // write to child process through write end.
    read(c2p[0], buf, 1);   // read from child process through read end.
    fprintf(1, "%d: received pong\n", getpid());
  }
  close(p2c[0]);
  close(p2c[0]);
  close(c2p[0]);
  close(c2p[1]);
  exit(0);
}