struct mem_stats {
        struct spinlock lock;
        uint64 total_allocated_pages;             // Tracks how many allocated pages right now
        uint64 total_allocations;                 // Tracks how many alloc operations has been made
};
extern struct mem_stats memory_statistics;

void* kalloc_and_map(pagetable_t pagetable, pte_t *pte);
