#include "kernel/types.h"
#include "user/user.h"

#define PGSIZE 4096

void print_memory_statistics(struct memstat *s, int i);

int main(int argc, char *argv[])
{
  printf("Hello my name is %s\n", argv[0]);

        // Lazy allocating user memory (In this implementation)
        int *arr = malloc(PGSIZE * 1000 * sizeof(int));
        struct memstat *s = malloc(sizeof(struct memstat));
        
        for (int i = 0; i < PGSIZE * 1000; i+=PGSIZE)
        {
                arr[i] += i;
                print_memory_statistics(s, i);
        }

        int m = fork();
        if (m == 0)
        {
                malloc(PGSIZE * 8);
                getmemstat(s);
                printf("index (in child process): pages_curr_allocated: %ld, total_allocations: %ld\n", s->total_allocated_pages, s->total_allocations);
                exit(0);
        }
        int thing;
        wait(&thing);
        getmemstat(s);
        printf("index (AFTER RUN): pages_curr_allocated: %ld, total_allocations: %ld\n", s->total_allocated_pages, s->total_allocations);

        exit(0);
}


void print_memory_statistics(struct memstat *s, int i)
{
        getmemstat(s);
        printf("index %d: pages_curr_allocated: %ld, total_allocations: %ld\n", i, s->total_allocated_pages, s->total_allocations);
}
