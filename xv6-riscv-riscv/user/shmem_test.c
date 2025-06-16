#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "fcntl.h"
#include <string.h>
#include <stdint.h> // Add this

#define PGSIZE 4096

extern void* map_shared_pages(void *src_va, int src_pid, int length);
extern int   unmap_shared_pages(void *addr,    int length);

int
main(int argc, char *argv[])
{
    int skip_unmap = 0;
    if (argc == 2 && strcmp(argv[1], "-skip") == 0)
        skip_unmap = 1;

    uintptr_t before_alloc = (uintptr_t)sbrk(0);
    printf("Parent: initial break = %p\n", (void*)before_alloc);

    char *buf = malloc(PGSIZE);
    if (!buf) {
        printf("Parent: malloc failed\n");
        exit(1);
    }
    uintptr_t after_alloc = (uintptr_t)sbrk(0);
    printf("Parent: after malloc(1 page) break = %p\n", (void*)after_alloc);

    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }

    if (pid == 0) {
        uintptr_t child_before = (uintptr_t)sbrk(0);
        printf("Child[%d]: before map break = %p\n", getpid(), (void*)child_before);

        void *shared = map_shared_pages(buf, getppid(), PGSIZE);
        if (!shared) {
            printf("Child: map_shared_pages failed\n");
            exit(1);
        }
        uintptr_t child_after_map = (uintptr_t)sbrk(0);
        printf("Child[%d]: after map break = %p\n", getpid(), (void*)child_after_map);

        strcpy((char*)shared, "Hello daddy");

        if (!skip_unmap) {
            int ret = unmap_shared_pages(shared, PGSIZE);
            printf("Child[%d]: unmap_shared_pages returned %d\n", getpid(), ret);
            printf("Child[%d]: after unmap break = %p\n", getpid(), sbrk(0));
        } else {
            printf("Child[%d]: skipping unmap step\n", getpid());
        }

        char *tmp = malloc(16);
        if (!tmp) {
            printf("Child[%d]: second malloc failed\n", getpid());
            exit(1);
        }
        printf("Child[%d]: after second malloc break = %p\n", getpid(), sbrk(0));

        exit(0);
    } else {
        wait(0);
        printf("Parent: read from buffer = \"%s\"\n", buf);
        exit(0);
    }

    return 0;
}