#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


#define MAP_SIZE 4096


int
main(int argc, char *argv[])
{

  int unmap_flag = 1;
  unmap_flag = atoi(argv[1]);
  char *shared = malloc(MAP_SIZE);
  int parent_pid = getpid();
  
  int pid = fork();

  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // ---------------------------
    // CHILD PROCESS
    // ---------------------------
    char* sz_before = sbrk(0);
    printf("shared in child: %p\n", shared);
    printf("child: sz before mapping: %p\n", sz_before);
    sleep(20);
    char* mapped = map_shared_pages(shared, MAP_SIZE, parent_pid);
    printf("mapped in child is: %p\n", mapped);
    
    if (!mapped) {
      printf("parent: map_shared_pages failed\n");
      kill(pid);
      wait(0);
      exit(1);
    }

    char* sz_after_mapping = sbrk(0);
    printf("child: sz after mapping: %p\n", sz_after_mapping);

    // Use the shared pointer (valid after mapping)
    strcpy(mapped, "Hello daddy");

    sleep(50);

    if (unmap_flag) {
      if (unmap_shared_pages(mapped, MAP_SIZE) < 0)
        printf("child: unmap_shared_pages failed\n");
      else
        printf("child: unmapped shared memory\n");

    } 
    else {
      printf("child: not unmapping memory (per flag)\n");
    }

    char* sz_after_unmap = sbrk(0);
    void *tmp = malloc(MAP_SIZE*25);
    char* sz_after_malloc = sbrk(0);
    printf("child: sz after unmap: %p, after malloc: %p\n", sz_after_unmap, sz_after_malloc);

    if (tmp) {
      free(tmp);
      printf("temp exists\n");
    }

    exit(0);
  } 
  
  else {
    sleep(10);
    char* parent_sz_before = sbrk(0);
    printf("parent size before: %p\n", parent_sz_before);

    // ---------------------------
    // PARENT PROCESS
    // ---------------------------
    // Allocate memory to share
    if (!shared) {
      printf("parent: malloc failed\n");
      kill(pid);
      wait(0);
      exit(1);
    }

    sleep(40);
    // Read message
    printf("parent: read from shared memory: %s\n", shared);
    printf("shared in parent is: %p\n", shared);


    wait(0);
    free(shared);
  }

  exit(0);
}