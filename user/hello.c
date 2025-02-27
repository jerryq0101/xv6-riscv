#include "kernel/types.h"
#include "user/user.h"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define PGSIZE 4096

// void print_memory_statistics(struct memstat *s, int i, int start_time);

int main(int argc, char *argv[])
{

        
}

// void print_memory_statistics(struct memstat *s, int i, int start_time)
// {
//         getmemstat(s);
//         printf("time %d, index %d: pages_curr_allocated: %ld, total_allocations: %ld\n", 
//                uptime() - start_time,
//                i, 
//                s->total_allocated_pages, 
//                s->total_allocations);
// }

