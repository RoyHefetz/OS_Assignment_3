#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAP_SIZE 4096

int main(int argc, char *argv[]) {
  int should_unmap = atoi(argv[1]); // Shared memory unmapping flag
  char *shared_mem = malloc(MAP_SIZE);
  int parent_pid = getpid();
  int pid = fork();

  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // Child process
    char* brk_before = sbrk(0); // Get the current heap value before mapping shared pages
    printf("shared in child: %p\n", shared_mem);
    printf("child: sbrk before mapping: %p\n", brk_before);

    sleep(20); // Allow parent to allocate memory before child maps shared pages
    char* mapped = map_shared_pages(shared_mem, MAP_SIZE, parent_pid);
    printf("mapped in child is: %p\n", mapped);

    if (!mapped) {
      printf("child: map_shared_pages failed\n");
      kill(pid);
      wait(0);
      exit(1);
    }

    char* brk_after_map = sbrk(0);
    printf("child: sbrk after mapping: %p\n", brk_after_map);

    strcpy(mapped, "Hello daddy");

    sleep(50);

    if (should_unmap) {
      if (unmap_shared_pages(mapped, MAP_SIZE) < 0)
        printf("child: unmap_shared_pages failed\n");
      else
        printf("child: unmapped shared memory\n");
    } else {
      printf("child: not unmapping memory\n");
    }

    char* brk_after_unmap = sbrk(0);
    void *tmp_alloc = malloc(MAP_SIZE * 25);
    char* brk_after_alloc = sbrk(0);

    printf("child: sbrk after unmap: %p, after malloc: %p\n", brk_after_unmap, brk_after_alloc);

    if (tmp_alloc) {
      free(tmp_alloc);
      printf("child: temp malloc exists and was freed\n");
    }

    exit(0);
  } else {
    // Parent process
    sleep(10);
    char* parent_brk_before = sbrk(0);
    printf("parent: sbrk before: %p\n", parent_brk_before);

    if (!shared_mem) {
      printf("parent: malloc failed\n");
      kill(pid);
      wait(0);
      exit(1);
    }

    sleep(40);
    printf("parent: read from shared memory: %s\n", shared_mem);
    printf("shared in parent is: %p\n", shared_mem);

    wait(0);
    free(shared_mem);
  }

  exit(0);
}