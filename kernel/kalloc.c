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
#include "pa_track.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct mem_stats memory_statistics;
uint64 cow_refcount[NPHYS];
struct spinlock cow_ref_lock;

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
        initlock(&memory_statistics.lock, "mem_stats");
        initlock(&cow_ref_lock, "cow_refcount");
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
        
        acquire(&cow_ref_lock);
        uint64 refs = cow_refcount[(uint64) pa / PGSIZE];
        release(&cow_ref_lock);
        if (refs <= 1)          // one reference left, free the physical address!
        {
                // Fill with junk to catch dangling refs.
                memset(pa, 1, PGSIZE);

                r = (struct run*)pa;

                // Update physical address tracking for COW
                acquire(&cow_ref_lock);
                cow_refcount[(uint64) pa / PGSIZE] = 0;
                release(&cow_ref_lock);

                // Update memory statistics
                acquire(&memory_statistics.lock);
                memory_statistics.total_allocated_pages-=1;
                release(&memory_statistics.lock);

                acquire(&kmem.lock);
                r->next = kmem.freelist;
                kmem.freelist = r;
                release(&kmem.lock);
        }
        else if (refs > 1)   // Case: kfreeing a pa that still has other references, so decreasing the reference count for that page
        {
                acquire(&cow_ref_lock);
                cow_refcount[(uint64) pa / PGSIZE] -= 1;
                release(&cow_ref_lock);
        }
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
        if (r)
                kmem.freelist = r->next;
        release(&kmem.lock);

        if (r)
        {
                memset((char *)r, 6, PGSIZE); // fill with junk

                // Update physical address tracking for COW
                acquire(&cow_ref_lock);
                cow_refcount[(uint64) r / PGSIZE] = 1;
                release(&cow_ref_lock);

                // Update Memory Statistics
                acquire(&memory_statistics.lock);
                memory_statistics.total_allocated_pages+=1;
                memory_statistics.total_allocations+=1;
                release(&memory_statistics.lock);
        }
        return (void *)r;
}

// Demand Paging: Allocate physical memory for the demand paged PTE and change permissions accordingly
void *
kalloc_and_map(pagetable_t pagetable, pte_t *pte)
{
        struct run *r;

        acquire(&kmem.lock);

        // PTE is demand-paged
        if ((*pte & PTE_D) == 0)
        {
                release(&kmem.lock);
                return 0;
        }

        r = kmem.freelist;
        if (r)
        {
                kmem.freelist = r->next;
                memset((char *)r, 4, PGSIZE);

                // Update physical address tracking for COW
                acquire(&cow_ref_lock);
                cow_refcount[(uint64)r / PGSIZE] = 1;
                release(&cow_ref_lock);

                // Update Memory Statistics
                acquire(&memory_statistics.lock);
                memory_statistics.total_allocated_pages += 1;
                memory_statistics.total_allocations += 1;
                release(&memory_statistics.lock);

                // Update PTE with newly got physical memory
                int perm = PTE_FLAGS(*pte);
                perm = (perm & ~PTE_D) | PTE_V;
                *pte = PA2PTE((uint64)r) | perm;
                sfence_vma();
        }

        release(&kmem.lock);
        return (void *)r;
}
