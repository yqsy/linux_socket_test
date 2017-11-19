#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


int main() {
  pid_t pid;
  int status;

  if ((pid = fork()) < 0) {
    perror("fork");
    exit(1);
  }

  // child
  if (pid == 0) {
    exit(0);
  }

  sleep(5);

  pid = wait(&status);
  if (WIFEXITED(status)) {
    fprintf(stderr, "\n\t[%d]\tProcess %d exited with status %d.\n",
            (int)getpid(), pid, WEXITSTATUS(status));
  }

  return 0;
}