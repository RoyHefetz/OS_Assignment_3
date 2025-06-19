// user/log_test.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/riscv.h"   // for PGSIZE

#define NCHILD 7

void child_log(int index, char *shared) {
    char msg[64];
    int msg_len = 0;

    if (index >= 10) {
        msg[msg_len++] = '0' + (index / 10);
        msg[msg_len++] = '0' + (index % 10);
    } else {
        msg[msg_len++] = '0' + index;
    }

    memmove(msg + 6, msg, msg_len); // 6 beacuse of "child " message length
    memcpy(msg, "child ", 6);
    msg_len += 6;

    const char *suffix = ": hello from log!";
    int suf_len = strlen(suffix);
    memcpy(msg + msg_len, suffix, suf_len);
    msg_len += suf_len;

    // scan the shared buffer for a free slot
    char *ptr = shared;
    char *end = shared + PGSIZE;
    while (ptr + 4 + msg_len < end) {
        unsigned int *hdr = (unsigned int *)ptr;
        if (*hdr == 0) {
            unsigned int new_hdr = (index << 16) | msg_len;
            // child try to claim the shared buffer
            if (__sync_val_compare_and_swap(hdr, 0, new_hdr) == 0) {
                memcpy(ptr + 4, msg, msg_len);
                break;
            }
        } 
        // else: skip to next entry
        unsigned short existing = *hdr & 0xFFFF;
        ptr += 4 + existing;
        ptr = (char *)(((uint64)ptr + 3) & ~3); // 4-byte alignment
    }
}

void read_log_buffer(char *shared) {
    char *ptr = shared;
    char *end = shared + PGSIZE;

    while (ptr + 4 < end) { // reads until end of buffer
        unsigned int *hdr = (unsigned int *)ptr;
        unsigned int h = *hdr;
        if (h == 0) break;
        int child = (h >> 16) & 0xFFFF; // extract child index
        int len = h & 0xFFFF; // extract message length
        if (ptr + 4 + len > end) {
            printf("Corrupt entry stopping\n");
            break;
        }

        char buf[128];
        memcpy(buf, ptr + 4, len);
        buf[len] = '\0';
        printf("[child %d] %s\n", child, buf);

        ptr += 4 + len;
        ptr = (char *)(((uint64)ptr + 3) & ~3); // b-byte alignment
    }
}
int main(void) {
  int parent = getpid();
  char *shared = malloc(PGSIZE);
  memset(shared, 0, PGSIZE);

  for (int i = 0; i < NCHILD; i++) {
    if (fork() == 0) {
      char *mapped = map_shared_pages(shared, PGSIZE, parent); 
      if (!mapped) { printf("Child %d: map failed\n", i);
         exit(1); 
      }
      child_log(i, mapped);
      exit(0);
    }
  }

  // Waits for all children processes to finish write into the shared buffer
  for (int i = 0; i < NCHILD; i++)
    wait(0);

  printf("\n--- Parent reading logs ---\n");
  read_log_buffer(shared);
  printf("--- End of log ---\n");

  exit(0);
}
