// dptest.c - Measures demand paging effectiveness
#include "kernel/types.h"
#include "user/user.h"

#define LARGE_SIZE (50 * 4096) // 50 pages (200KB)
#define ACCESS_INTERVAL 10     // Only access every 10th page

int main()
{
        struct memstat before, after;
        getmemstat(&before);

        printf("Allocating %d bytes...\n", LARGE_SIZE);

        // Allocate large buffer with malloc (increases heap size)
        char *buf = malloc(LARGE_SIZE);
        if (!buf)
        {
                printf("Failed to allocate memory\n");
                exit(1);
        }

        // Only access every Nth page - this shows demand paging efficiency
        printf("Touching every %dth page...\n", ACCESS_INTERVAL);
        for (int i = 0; i < LARGE_SIZE; i += ACCESS_INTERVAL * 4096)
        {
                buf[i] = 1; // Touch this page
        }

        // Wait a moment to ensure stats are updated
        sleep(1);

        getmemstat(&after);

        // Calculate metrics
        uint64 total_virtual_pages = LARGE_SIZE / 4096;
        uint64 touched_pages = total_virtual_pages / ACCESS_INTERVAL;
        uint64 actual_allocated = after.total_allocations - before.total_allocations;

        printf("\nDEMAND PAGING EFFECTIVENESS:\n");
        printf("Total virtual pages: %ld\n", total_virtual_pages);
        printf("Pages actually touched: %ld\n", touched_pages);
        printf("Pages physically allocated: %ld\n", actual_allocated);
        printf("Memory saved: %ld pages (%ld bytes)\n",
               total_virtual_pages - actual_allocated,
               (total_virtual_pages - actual_allocated) * 4096);

        // Use integer arithmetic instead of floating point
        uint64 efficiency_pct = (100 * (total_virtual_pages - actual_allocated)) / total_virtual_pages;
        printf("Memory efficiency: %ld%%\n", efficiency_pct);
        free(buf);
        exit(0);
}
