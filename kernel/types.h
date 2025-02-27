typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;

#ifndef MEMSTAT
#define MEMSTAT
struct memstat {
        uint64 user_faults;             // Total usertrap faults
        uint64 total_allocations;       // Total page allocations 
        uint64 total_allocated_pages;   // Current allocated pages
        uint64 demand_page_faults;      // Count of demand paging faults
        uint64 cow_page_faults;         // Count of COW faults
        uint64 cow_pages_shared;        // Pages initially shared through COW
        uint64 cow_copies_made;         // Pages copied due to writes
        uint64 peak_allocated_pages;    // Maximum memory usage observed
};
#endif
