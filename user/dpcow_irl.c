// realworld.c - Simulates server workload with comprehensive memory metrics
#include "kernel/types.h"
#include "user/user.h"

#define CONFIG_SIZE (2 * 4096) // Configuration data (2 pages)
#define CODE_SIZE (10 * 4096)  // Shared code/libraries (10 pages)
#define HEAP_SIZE (20 * 4096)  // Dynamic heap allocation (20 pages)
#define CLIENTS 5              // Number of client requests to handle

int main()
{
        struct memstat start, end;
        getmemstat(&start);

        // Server initialization phase
        printf("Server starting...\n");

        // Shared read-only configuration (benefits from COW)
        char *config = malloc(CONFIG_SIZE);
        for (int i = 0; i < CONFIG_SIZE; i++)
        {
                config[i] = 'C'; // Fill config with data
        }

        // Shared code/libraries (benefits from COW)
        char *code = malloc(CODE_SIZE);
        for (int i = 0; i < CODE_SIZE; i++)
        {
                code[i] = 'X'; // Initialize code memory
        }

        // Pre-allocate a pool of memory but don't touch most of it
        // (benefits from demand paging)
        char *memory_pool = malloc(HEAP_SIZE);

        // Only touch a small portion to initialize critical structures
        for (int i = 0; i < HEAP_SIZE / 10; i++)
        {
                memory_pool[i] = 'M';
        }

        struct memstat after_init;
        getmemstat(&after_init);

        printf("Server initialized\n");
        printf("Initial setup complete.\n");

        // Handle client requests
        for (int client = 0; client < CLIENTS; client++)
        {
                int pid = fork();
                if (pid == 0)
                {
                        struct memstat client_start, client_end;
                        getmemstat(&client_start);

                        // Child process - client request handler

                        // Every client reads config (shared through COW)
                        int config_sum = 0;
                        for (int i = 0; i < CONFIG_SIZE; i++)
                        {
                                config_sum += config[i];
                        }

                        // Every client uses code (shared through COW)
                        int code_sum = 0;
                        for (int i = 0; i < CODE_SIZE; i++)
                        {
                                code_sum += code[i];
                        }

                        // Each client uses different portions of the heap
                        // (sparse access pattern benefits from demand paging)
                        int offset = (client * HEAP_SIZE / CLIENTS);
                        int length = HEAP_SIZE / CLIENTS;

                        // Most clients only touch a small percentage of their memory
                        // (like a real program that allocates memory it might need)
                        int access_percent = 10 + (client * 10); // 10% to 50%
                        for (int i = 0; i < length * access_percent / 100; i++)
                        {
                                memory_pool[offset + i] = client;
                        }

                        // Some clients modify small parts of shared data (triggers COW)
                        if (client % 3 == 0)
                        {
                                for (int i = 0; i < CONFIG_SIZE / 10; i++)
                                {
                                        config[i] = 'D'; // Modify config (forces COW)
                                }
                        }

                        getmemstat(&client_end);

                        printf("\n--- CLIENT %d METRICS ---\n", client);
                        printf("Page faults:       %ld\n", client_end.user_faults - client_start.user_faults);
                        printf("  Demand paging:   %ld\n", client_end.demand_page_faults - client_start.demand_page_faults);
                        printf("  COW faults:      %ld\n", client_end.cow_page_faults - client_start.cow_page_faults);
                        printf("Pages allocated:   %ld\n", client_end.total_allocations - client_start.total_allocations);
                        printf("Current memory:    %ld pages (%ld KB)\n",
                               client_end.total_allocated_pages,
                               client_end.total_allocated_pages * 4);
                        printf("COW pages shared:  %ld\n", client_end.cow_pages_shared - client_start.cow_pages_shared);
                        printf("COW copies made:   %ld\n", client_end.cow_copies_made - client_start.cow_copies_made);
                        printf("Access percentage: %d%%\n", access_percent);
                        printf("Modified config:   %s\n", (client % 3 == 0) ? "Yes" : "No");

                        exit(0);
                }
        }

        // Wait for all client handlers to complete
        for (int i = 0; i < CLIENTS; i++)
        {
                wait(0);
        }

        // After waiting for all clients to complete
        getmemstat(&end);

        // Calculate pages allocated during the test
        uint64 pages_allocated_during_test = end.total_allocations - start.total_allocations;

        // Print comprehensive metrics
        printf("\n===== SERVER OVERALL METRICS =====\n");
        printf("Server execution summary:\n");
        printf("Initial memory allocation state:       %ld pages (%ld KB)\n",
               after_init.total_allocated_pages,
               after_init.total_allocated_pages * 4);
        printf("Additional pages allocated during test: %ld pages (%ld KB)\n",
               pages_allocated_during_test,
               pages_allocated_during_test * 4);
        printf("Ending memory allocation state:         %ld pages (%ld KB)\n",
               end.total_allocated_pages,
               end.total_allocated_pages * 4);

        printf("\nMemory efficiency metrics:\n");
        printf("Total page faults:          %ld\n", end.user_faults - start.user_faults);
        printf("  Demand paging faults:     %ld\n", end.demand_page_faults - start.demand_page_faults);
        printf("  Copy-on-write faults:     %ld\n", end.cow_page_faults - start.cow_page_faults);
        printf("Total allocations:          %ld\n", end.total_allocations - start.total_allocations);

        printf("\nMemory sharing effectiveness:\n");
        printf("COW pages initially shared: %ld\n", end.cow_pages_shared - start.cow_pages_shared);
        printf("COW pages eventually copied:%ld\n", end.cow_copies_made - start.cow_copies_made);
        printf("COW sharing efficiency:     %ld%%\n",
               (end.cow_pages_shared - start.cow_pages_shared) > 0 ? (100 - (end.cow_copies_made - start.cow_copies_made) * 100 /
                                                                                (end.cow_pages_shared - start.cow_pages_shared))
                                                                   : 0);

        // Calculate theoretical memory without DP/COW
        uint64 theoretical_pages = (CONFIG_SIZE + CODE_SIZE + HEAP_SIZE) * CLIENTS / 4096;
        printf("\nTheoretical memory without DP/COW: %ld pages (%ld KB)\n",
               theoretical_pages,
               theoretical_pages * 4);

        // Compare with actual allocations
        printf("Actual memory with DP/COW:        %ld pages (%ld KB)\n",
               pages_allocated_during_test,
               pages_allocated_during_test * 4);

        // Calculate savings
        uint64 memory_savings = theoretical_pages - pages_allocated_during_test;
        printf("Total memory savings:             %ld pages (%ld KB)\n",
               memory_savings,
               memory_savings * 4);

        // Calculate efficiency as a percentage
        if (theoretical_pages > 0)
        {
                printf("Only %ld%% of theoretical mem was needed allocated\n",
                       100 - (pages_allocated_during_test * 100) / theoretical_pages);
        }
        else
        {
                printf("Memory efficiency improvement:    N/A (theoretical pages is zero)\n");
        }

        free(config);
        free(code);
        free(memory_pool);
        exit(0);
}
