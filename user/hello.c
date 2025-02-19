#include "kernel/types.h"
#include "user/user.h"

#define PGSIZE 4096

void print_memory_statistics(struct memstat *s, int i, int start_time);

int main(int argc, char *argv[])
{
        // Get starting time in ticks
        int start_time = uptime();
        
        // Lazy allocating user memory (In this implementation)
        int *arr = malloc(PGSIZE * 1000);
        struct memstat *s = malloc(sizeof(struct memstat));
        
        for (int i = 0; i < PGSIZE * 1000; i+=PGSIZE)
        {
                arr[i] += i;
                print_memory_statistics(s, i, start_time);
        }

        int m = fork();
        if (m == 0)
        {
                malloc(PGSIZE * 8);
                getmemstat(s);
                printf("Child at time %d: pages_curr_allocated: %ld, total_allocations: %ld\n", 
                       uptime() - start_time, 
                       s->total_allocated_pages, 
                       s->total_allocations);
                exit(0);
        }
        int thing;
        wait(&thing);
        getmemstat(s);
        printf("AFTER RUN at time %d: pages_curr_allocated: %ld, total_allocations: %ld\n", 
               uptime() - start_time,
               s->total_allocated_pages, 
               s->total_allocations);

        exit(0);
}

void print_memory_statistics(struct memstat *s, int i, int start_time)
{
        getmemstat(s);
        printf("time %d, index %d: pages_curr_allocated: %ld, total_allocations: %ld\n", 
               uptime() - start_time,
               i, 
               s->total_allocated_pages, 
               s->total_allocations);
}

