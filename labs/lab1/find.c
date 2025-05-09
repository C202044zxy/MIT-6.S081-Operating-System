#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int match(char* str, char* pattern) {
  int len1 = strlen(str), len2 = strlen(pattern);
  for (int i = 0; i + len2 <= len1; i++) {
    int flag = 1;
    for (int j = 0; j < len2; j++) {
      if (str[i+j] != pattern[j]) {
        // failed
        flag = 0;
        break;
      }
    }
    if (flag == 1) {
      return 1;
    }
  }
  return 0;
}

void find(char* path, char* pattern) {
  char buf[512];
  char* p;
  int fd;
  struct dirent de;
  struct stat st;
  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "find: can't open %s\n", path);
    return ;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: can't stat %s\n", path);
    close(fd);
    return ;
  }
  if (st.type == T_FILE) {
    if (match(path, pattern)) {
      printf("%s\n", path);
      return ;
    }
  } else if (st.type == T_DIR) {
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
      printf("The path is too long\n");
      return ;
    }
    strcpy(buf, path);
    p = buf + strlen(path);
    *p++ = '/';
    while (read(fd, &de, sizeof de) > 0) {
      if (de.inum == 0) {
        continue;
      }
      if (de.name[0] == '.' && de.name[1] == 0) {
        continue;
      }
      if (de.name[0] == '.' && de.name[1] == '.' && de.name[2] == 0) {
        continue;
      }
      memcpy(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, pattern);
    }
  }
  close(fd);
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(2, "usage: find [path] [pattern]\n");
    exit(0);
  }
  find(argv[1], argv[2]);
  exit(0);
}