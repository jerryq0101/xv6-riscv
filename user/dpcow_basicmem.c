#include "kernel/types.h"
#include "user/user.h"

int main() {
    struct memstat stats_before, stats_after;
    
    // Get initial statistics
    getmemstat(&stats_before);
    
    printf("Initial memory usage: %ld pages\n", stats_before.total_allocated_pages);
    
    // Allocate a large chunk of memory
    printf("Allocating 100 pages...\n");
    char *mem = malloc(100 * 4096);
    
    // Access every 4th page to trigger demand paging
    for (int i = 0; i < 100; i += 4) {
        mem[i * 4096] = i;
    }
    
    // Get updated statistics
    getmemstat(&stats_after);
    
    printf("Post-allocation statistics:\n");
    printf("  Pages allocated: %ld\n", stats_after.total_allocated_pages - stats_before.total_allocated_pages);
    printf("  Demand page faults: %ld\n", stats_after.demand_page_faults - stats_before.demand_page_faults);
    
    free(mem);
    exit(0);
}
