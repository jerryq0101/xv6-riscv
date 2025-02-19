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
        uint64 total_allocated_pages;             // Tracks how many allocated pages right now
        uint64 total_allocations;                 // Tracks how many alloc operations has been made
};
#endif
