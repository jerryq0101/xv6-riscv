// dpcow_forkch.c - fixed version
#include "kernel/types.h"
#include "user/user.h"

#define CHAIN_LENGTH 50
#define NPAGES 20

int main() {
    struct memstat initial_stats, final_stats;
    
    // Get initial stats
    getmemstat(&initial_stats);
    
    // Allocate memory
    char *mem = malloc(NPAGES * 4096);
    for (int i = 0; i < NPAGES; i++) {
        mem[i * 4096] = i;  // Initialize all pages
    }
    
    printf("Creating fork chain of length %d...\n", CHAIN_LENGTH);
    
    // Create a simple linear chain
    int i;
    for(i = 0; i < CHAIN_LENGTH; i++) {
        int pid = fork();
        if(pid < 0) {
            printf("Fork failed\n");
            exit(1);
        }
        
        if(pid > 0) {
            // Parent waits for child then exits
            wait(0);
            break;
        }
        
        // Only the child continues the loop
        printf("Forked process %d in chain\n", i+1);
    }
    
    // The last child in the chain will have i == CHAIN_LENGTH
    if(i == CHAIN_LENGTH) {
        printf("Last child modifying memory...\n");
        for (int j = 0; j < NPAGES; j++) {
            mem[j * 4096] = 99;  // This will trigger COW
        }
        
        getmemstat(&final_stats);
        
        printf("\nFork chain results:\n");
        printf("  Original pages: %d\n", NPAGES);
        printf("  Total processes: %d\n", CHAIN_LENGTH + 1);
        printf("  Pages shared through COW: %ld\n", final_stats.cow_pages_shared - initial_stats.cow_pages_shared);
        printf("  COW page faults: %ld\n", final_stats.cow_page_faults - initial_stats.cow_page_faults);
        printf("  COW pages copied: %ld\n", final_stats.cow_copies_made - initial_stats.cow_copies_made);
    }
    
    exit(0);
}
