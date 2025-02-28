struct mem_stats {
        struct spinlock lock;
        uint64 total_allocated_pages;             // Tracks how many allocated pages right now
        uint64 total_allocations;                 // Tracks how many kalloc operations has been made

        // New metrics for benchmarking
        uint64 user_faults;                // Count of total usertraps
        uint64 demand_page_faults;         // Count of demand paging faults
        uint64 cow_page_faults;            // Count of COW faults
        uint64 cow_pages_shared;           // Pages initially shared through COW
        uint64 cow_copies_made;            // Pages copied due to writes to COW pages
        uint64 exec_allocations;           // Pages allocated during exec
        uint64 sbrk_allocations;           // Pages allocated during heap growth
        uint64 peak_allocated_pages;       // Maximum memory usage observed
};
extern struct mem_stats memory_statistics;
