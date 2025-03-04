// enhanced_cowtest.c - Tests COW with different write patterns
#include "kernel/types.h"
#include "user/user.h"
#pragma GCC diagnostic ignored "-Wunused-variable"

#define MEMORY_SIZE (100 * 4096) // 100 pages

int main(int argc, char *argv[])
{
        if (argc != 4)
        {
                printf("Usage: dpcow_encow child_count write_percentage_per_child 1(faults)/0(pages_saved)\n");
                exit(1);
        }
        // Preconditions:
        // - CHILD_COUNT < NPROC (xv6 system limitations)
        // - WRITE_PERCENT <= 100
        int CHILD_COUNT = atoi(argv[1]);
        int WRITE_PERCENT = atoi(argv[2]);
        int faults = atoi(argv[3]);
        
        struct memstat before, after;
        getmemstat(&before);

        // Allocate and initialize memory
        char *memory = malloc(MEMORY_SIZE);
        for (int i = 0; i < MEMORY_SIZE; i++)
        {
                memory[i] = i & 0xFF; // Initialize all memory
        }

        // Create multiple children
        int pids[CHILD_COUNT];
        for (int c = 0; c < CHILD_COUNT; c++)
        {
                if ((pids[c] = fork()) == 0)
                {
                        // Child process - modify section of shared memory
                        int write_size = MEMORY_SIZE * WRITE_PERCENT / 100;

                        // Write to portion - this triggers COW only for these pages
                        for (int i = 0; i < write_size; i++)
                        {
                                memory[i] = 0xFF +i;
                        }

                        // Sleep to give parent time to collect stats
                        sleep(c + 1);
                        exit(0);
                }
        }

        // Wait for all children
        for (int c = 0; c < CHILD_COUNT; c++)
        {
                wait(0);
        }

        getmemstat(&after);

        // Calculate metrics
        if (faults)
        {
                uint64 actual_allocated = after.total_allocations - before.total_allocations;
                uint64 user_faults = after.user_faults - before.user_faults;
                
                printf("User faults: %ld | alloc_ops: %ld\n", user_faults, actual_allocated);
        }
        else
        {
                // Note: Since we are only modifying the heap memory at max to 100%, there will always exist
                //      some percentage that is still shared for readonly and code data in the child.
                uint64 pages_shared = after.cow_pages_shared - before.cow_pages_shared;
                uint64 pages_copied = after.cow_copies_made - before.cow_copies_made;        
                printf("Pages saved by COW: %ld (%ld KB)\n",
                       pages_shared - pages_copied,
                       (pages_shared - pages_copied) * 4);
        }

        free(memory);
        exit(0);
}
