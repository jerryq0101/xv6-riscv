#include "kernel/types.h"
#include "user/user.h"

#define NPAGES 100
#define MODIFY_PATTERN 10  // Modify every Nth page

int main() {
    struct memstat stats_before, stats_fork, stats_after;
    
    // Get initial statistics
    getmemstat(&stats_before);
    
    // Allocate and initialize memory
    printf("Allocating and initializing %d pages...\n", NPAGES);
    char *mem = malloc(NPAGES * 4096);
    
    // Initialize all pages 
    for (int i = 0; i < NPAGES; i++) {
        mem[i * 4096] = i;
    }
    
    printf("Forking to test COW efficiency...\n");
    int pid = fork();
    
    if (pid < 0) {
        printf("Fork failed\n");
        exit(1);
    }
    
    // Get post-fork statistics
    getmemstat(&stats_fork);
    
    if (pid == 0) {
        // Child: modify every MODIFY_PATTERN'th page
        printf("Child modifying pages...\n");
        for (int i = 0; i < NPAGES; i += MODIFY_PATTERN) {
            mem[i * 4096] = 99;  // Trigger COW
        }
        
        // Get final statistics
        getmemstat(&stats_after);
        
        printf("\nChild statistics:\n");
        printf("  Pages shared through COW: %ld\n", stats_fork.cow_pages_shared - stats_before.cow_pages_shared);
        printf("  COW page faults: %ld\n", stats_after.cow_page_faults - stats_fork.cow_page_faults);
        printf("  COW pages copied: %ld\n", stats_after.cow_copies_made - stats_fork.cow_copies_made);
        
        exit(0);
    } else {
        // Parent: wait for child
        wait(0);
        
        // Parent modifies different pages
        printf("Parent modifying pages...\n");
        for (int i = 1; i < NPAGES; i += MODIFY_PATTERN) {
            mem[i * 4096] = 88;  // Trigger COW
        }
        
        // Get final statistics
        getmemstat(&stats_after);
        
        printf("\nParent statistics:\n");
        printf("  Pages shared through COW: %ld\n", stats_fork.cow_pages_shared - stats_before.cow_pages_shared);
        printf("  COW page faults: %ld\n", stats_after.cow_page_faults - stats_fork.cow_page_faults);
        printf("  COW pages copied: %ld\n", stats_after.cow_copies_made - stats_fork.cow_copies_made);
        
        printf("\nMemory efficiency:\n");
        printf("  Without COW (theoretical): %d pages\n", NPAGES * 2);
        printf("  With COW (actual): %ld pages\n", 
               NPAGES + (stats_after.cow_copies_made - stats_fork.cow_copies_made));
        printf("  Memory saved: %ld pages\n", 
               NPAGES - (stats_after.cow_copies_made - stats_fork.cow_copies_made));
        
        free(mem);
        exit(0);
    }
}
