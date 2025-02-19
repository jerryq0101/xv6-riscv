// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "kalloc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct mem_stats memory_statistics;

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
        initlock(&kmem.lock, "kmem");
        freerange(end, (void*)PHYSTOP);
        initlock(&kmem.lock, "kmem");
        initlock(&memory_statistics.lock, "mem_stats");
        // Calculate number of pages between end and PHYSTOP
        uint64 num_pages = ((uint64)PHYSTOP - PGROUNDUP((uint64)end)) / PGSIZE;
        memory_statistics.total_allocated_pages = num_pages;

        freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  // Decrease current allocated pages
  acquire(&memory_statistics.lock);
  memory_statistics.total_allocated_pages-=1;
  release(&memory_statistics.lock);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
  {
    memset((char*)r, 5, PGSIZE); // fill with junk

    // Update Memory Statistics
    acquire(&memory_statistics.lock);
    memory_statistics.total_allocated_pages+=1;
    memory_statistics.total_allocations+=1;
    release(&memory_statistics.lock);
}
  return (void*)r;
}
