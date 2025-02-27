// exectest.c
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    struct memstat start_stats, end_stats;
    
    if (argc > 1 && strcmp(argv[1], "child") == 0) {
        // This is the child process after exec
        getmemstat(&start_stats);
        
        // Allocate and touch memory
        printf("Child allocating memory...\n");
        char *mem = malloc(20 * 4096);
        for (int i = 0; i < 20; i++) {
            mem[i * 4096] = i;  // Touch each page
        }
        
        getmemstat(&end_stats);
        
        printf("Child memory statistics:\n");
        printf("  Pages allocated: %ld\n", end_stats.total_allocated_pages - start_stats.total_allocated_pages);
        printf("  Demand page faults: %ld\n", end_stats.demand_page_faults - start_stats.demand_page_faults);
        
        exit(0);
    } else {
        // Parent process
        getmemstat(&start_stats);
        
        printf("Testing exec memory behavior...\n");
        
        // Fork and exec
        int pid = fork();
        if (pid < 0) {
            printf("Fork failed\n");
            exit(1);
        }
        
        if (pid == 0) {
            // Child
            char *args[] = {"dpcow_exec", "child", 0};
            exec("dpcow_exec", args);
            printf("Exec failed\n");
            exit(1);
        }
        
        // Wait for child
        wait(0);
        
        // Get final stats
        getmemstat(&end_stats);
        
        printf("\nExec statistics:\n");
        printf("  Peak memory usage: %ld pages\n", end_stats.peak_allocated_pages);
        printf("  Net change in allocated pages: %ld\n", 
               end_stats.total_allocated_pages - start_stats.total_allocated_pages);
        
        exit(0);
    }
}
