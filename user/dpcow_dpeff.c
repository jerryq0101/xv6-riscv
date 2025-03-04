// dptest.c - Measures demand paging effectiveness
#include "kernel/types.h"
#include "user/user.h"
#pragma GCC diagnostic ignored "-Wunused-variable"

#define LARGE_SIZE (50 * 4096) // 50 pages (200KB)

int main(int argc, char* argv[])
{
        if (argc < 3)
        {
                printf("Usage: dpcow_dpeff X_pages_per_access 1(faults)/0(pages_saved)\n");
                exit(1);
        }
        // Assuming input is an integer > 0
        int ACCESS_INTERVAL = atoi(argv[1]); // Only access every X page
        int faults = atoi(argv[2]);

        struct memstat before, after;
        getmemstat(&before);

        // Allocate large buffer with malloc (increases heap size)
        char *buf = malloc(LARGE_SIZE);
        if (!buf)
        {
                printf("Failed to allocate memory\n");
                exit(1);
        }

        // Only access every Nth page - this shows demand paging efficiency
        for (int i = 0; i < LARGE_SIZE; i += ACCESS_INTERVAL * 4096)
        {
                buf[i] = i; // Touch this page
                
        }

        // Wait a moment to ensure stats are updated
        sleep(1);

        getmemstat(&after);

        uint64 actual_allocated = after.total_allocations - before.total_allocations;
        if (faults == 1)
        {
                uint64 user_faults = after.user_faults - before.user_faults;
                printf("User faults: %ld | alloc_ops: %ld\n", user_faults, actual_allocated);
        }
        else if (faults == 0)
        {
                // Calculate metrics
                uint64 total_virtual_pages = LARGE_SIZE / 4096;
                uint64 touched_pages = total_virtual_pages / ACCESS_INTERVAL;
                
                printf("Memory saved: %ld pages (%ld KB)\n",
                        total_virtual_pages - actual_allocated,
                        (total_virtual_pages - actual_allocated) * 4);
        }
        else
        {
                
        }

        free(buf);
        exit(0);
}
