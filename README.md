# Demand Paging and Copy-on-Write in xv6

This project implements two critical memory optimization techniques in the xv6 operating system: demand paging and copy-on-write (COW). These techniques are fundamental to modern operating systems, allowing for conservative memory allocations.

## Project Overview

Memory management is one of the core responsibilities of an operating system kernel. This project enhances xv6's memory management in two significant ways:

1. **Demand Paging**: Instead of allocating physical memory pages at the time of mapping, allocation is deferred until the page is actually accessed, reducing memory waste.

2. **Copy-on-Write (COW)**: When a process forks, instead of duplicating all pages immediately, pages are shared with read-only permissions until one process attempts to modify a page, at which point a copy is made.

## Implementation Details

Sounds simple until now right? AAHAHHAHA. no.

### Demand Paging

Demand paging was implemented through modifications to several key components:

1. **Page Table Entry Modifications**:
   
   <strong>Defined a custom `PTE_D` (Demand) flag to mark pages that are mapped but not yet allocated.</strong>
   - This mainly altered the allocation sequence in `uvmalloc`, where the original procedure is to force allocation of all physical memory referred to. Now, in `uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz, int xperm, int force_alloc)` in `vm.c` there is an option either to `force_alloc`ate, or, to set PTE_D = 1 along with other permissions and leave the PFN initially blank. Note that the original option is still needed because program data in `exec(char *path, char **argv)` that needs to be immediately allocated.
   - We'd also need to mark PTE_V = 0 allowing invalid access page faults to be triggered to put us into `usertrap`.

2. **Trap Handler Modifications**: 
   
   Now, demand pages are "labelled", we need to handle them now. 
   
   <strong>Extended `usertrap` in `trap.c` to have a specific case for page faults to catch the demand paging signal</strong>
   - When `PTE_D` = 1 and `PTE_V` = 0 we'd know that this page is actually a demand paging case we marked from `uvmalloc`
   - Additionally, the other bit we needed to check was `PTE_U`. Since a usertrap is from user space, a user is accessing some memory. If the user doesn't even have permission for this virtual memory, then its actually a permission fault.

   <strong>Implemented allocation and mapping logic for demand-paged addresses</strong>
   - Ok, once we made sure these labels work, the rest of the work is somewhat straightforward: allocate physical memory for the PTE, and modify the bits on this PTE s.t. `PTE_V` = 1 and `PTE_D` = 0 and appending the new physical memory address as the PFN. Now this page is a normal page!
   - Note that we should also flush the TLB since the original PTE would be outdated (This step maybe deleted, but I am unclear to the TLB operations in xv6)

3. **Memory Operations**:

   Ok, we now have to consider kernel space writes. Since the kernel can handle a syscall and also write to user space pages, it may also write to demand pages. The kernel space will not trigger a usertrap, but a kerneltrap panic (which we don't want). Therefore, we have to do necessary demand page handling in the functions that carries out the syscall operations in kernel space.

   </strong>Modified `copyout(...)`, `copyin(...)`, and `copyinstr(...)` in `vm.c` to handle demand-paged memory</strong>
   - These parts required a similar process of the page fault handler. Since involving each of these functions count as a "touch" to the page, by definition of demand paging, we would be allocating it. Therefore, in a helper function `walkaddr_demand_paged`, what we do is to first find the pte, check if its demand paged => allocate physical memory for it, and check validity of permissions. This helper allows the top level function `copyout` to treat the page as an already allocated page, which we can then copy kernel syscall data into.

   <strong>Added safeguards to properly handle invalid accesses</strong>
   - Safeguards are more-so needed in this situation due to being in kernel space. Therefore we have to check bit validity for the syscall to write to after `kalloc_and_map` (e.g. PTE_U and PTE_R), write overflow from user virtual address, and the validity of the allocated memory, all to ensure that we don't let the kernel trap handle a user memory related issue. This also entails that syscalls in user programs will additionally fail with -1 for user memory issues.

4. **Process Creation and Management**:
   
   <strong>On forking, ensure that a demand page stays a demand page.</strong>
   - This was enforced in `uvmcopy` in `vm.c` where it copies all of the parent's pages into the child.

   <strong>On deletion, ensure that a demand page is not free'd</strong>
   - This is enforced in `uvmunmap` in `vm.c` where it handles kfree calls to free physical memory, since all the other clean up functions relies on `uvmunmap` (`uvmdealloc`, `uvmfree`)


### Copy-on-Write

Copy-on-Write was implemented through the following changes:

1. **Page Table Entry Modifications**:
   
   <strong>Added a custom `PTE_C` (COW) flag to mark pages shared through copy-on-write</strong>
   - Similar to the demand paging signifier PTE_D, we needed one for this process when pages are shared at `uvmcopy` in `vm.c`.

2. **Fork Modifications**:
   
   <strong>Modified `uvmcopy()` to share pages instead of duplicating them in the fork process</strong>
   - On calling fork, which calls `uvmcopy`, we turn PTE_W = 0 and turn on PTE_C. Note that when doing this, one should consider whether if the page was originally writable. If it is not originally writable `PTE_W`, the pte shouldn't even be turned into a COW page as it could simply be shared - code/readonly data. (I almost augmented a memory allocator and with a hashtable and AVL tree to track this information LMAO).
   - On fork, the other case that we should preserve is demand paging, since demand pages don't even have physical pages yet, therefore don't have any sharing, the PTE data should simply be copied directly.

3. **Fault Handling**:

   After creating these COW pages, we need to copy and allocate, and then write. Similar to demand paging, there are two sides of the write issue: user space writes, kernel space syscall writes. We first think about a user space write.

   <strong>Extended the page fault handler to detect write attempts to COW pages</strong>
   - Write attempts to PTE_C = 1, PTE_W = 0 pages should trigger an usertrap due to a permission error (r_scause=15) (Notice that for COW usertrap-signalling, we didn't need to set `PTE_V` = 0 to trigger this)

   <strong>Implemented the page duplication mechanism when writes occur</strong>
   - This is the copy on write part!
   - `handle_cow_fault` handles the allocation of a new physical page, copying the previous contents to the new PA, and also decreasing the reference count of the original PA (See 5). The result of the allocation for COW is similar to demand paging, the "detached" page is now a "normal" page without PTE_C

4. **Kernel Writing to User Space**:

   When a kernel executes operations like `write`, writing to user memory in kernel space will envoke `copyout`. If a page fault (due to bad access) occurs during the kernel's operation, we won't go to the usertrap, we'd go to the kerneltrap instead, which panics. This is not good, we actually want the system to allocate and carry through the operation! (perhaps it is possible to implement a solution in `kerneltrap` in `trap.c`)

   <strong>Modified `copyout()` to handle COW pages correctly when the kernel writes to user space (similar to the process of usertrap)</strong>
   - Copyout now handles both demand paging and copy on write cases.
   - Notice that in `copyout`, we do have to help the kernel carry out the copy operation by using memmove on the newly allocated user page from COW. This is because in a trap, we'd retry the original instruction when returning to the program. For a kernel syscall, since we aren't in a trap and are simply carrying out a syscall, we do have to help the syscall take place as it goes to the next instruction. 


5. **Reference Counting**:

   Ok, now we have the process the creation of COW pages and the writing process, how do we clean up? What if many processes refer to the same physical address? - we gotta know if we should free the physical address (PA) or not. References are also needed for writing - if there is only one reference to this physical page, then we can write to it, no need to allocate another one. Therefore, aside from adding a `PTE_C` bit to the PTE, we also needed to track how many references a single PA has: To not write over other process memory, and at termination, leak memory. 

   <strong>Implemented a global kernel reference counter to track how many processes share each physical page</strong>
   - This is in `pa_track.h` which hosts an array of `uint cow_refcount[PHYS]` to track the number of cow references for a PA.
   - This is a global overhead in kernel memory space and it is also an overhead to the individual operations that need to update and check `cow_refcount[pa]`
   - Note that a lock is needed for this since the array is shared for all processes.

   <strong>Ensured proper memory cleanup when the last reference to a page is removed</strong>
   - Ok now we have the references, we are able to clean up correctly in `kfree`. For when a kfree is called for a COW page, we decrement the number of refences for cow_refcount[this page]. And if only there was a single reference, we then free the physical memory.
   - It is thus super important to ensure that the cow_refcount is updated accordingly after kfree and allocation (`usertrap` in `trap.c`, `copyout` in `vm.c`, and `uvmcopy` in `vm.c`) to not accidently free a PA still in use, or leak the physical page when terminating the program. 




## Performance Measurements

Despite the behaviour being predictable, I've implemented statistics to measure the behaviour of demand paging and copy on write. I was not able to measure time due to the precision of `uptime()` in xv6, which only measures ticks and is only accurate to about ~10ms. I am planning to measure CPU cycles in a future implementation.

### For a demand paging workload (in `dpcow_dpeff.c`) we can evaluate its strengths and weaknesses in these three graphs.

![Demand Paging Workload: Pages Saved vs Percentage of Heap Touched](./graphs/dp_pages_heaptouch_vs_pagesave.png)
(Figure 1)

This demonstrates the space efficiency that is possible from sparse read/write workloads using demand paging.

![Demand Paging: Page faults vs Percentage of Heap Touched](./graphs/dp_heaptouch_vs_pagefaults.png)
(Figure 2)

This demonstrates time efficiency overhead of demand paging, the amount of additional page faults to allocate pages increases (Note: only `usertrap` is counted here because a kernel syscall will context switch anyway).

![Demand Paging: Page faults vs Percentage of Heap Touched](./graphs/dp_heaptouch_vs_allocationops.png)
(Figure 3)

The dynamic number of allocation operations can be viewed as a pro and a con. Pro-the OS has less work <strong>in total</strong> since it allocates one by one. Con-the OS will have an extra step for each memory touch.

Whether this is a pro or a con depends on the specific operation being carried out (E.g. HFT may care more about speed of a single trade vs speed of a program sequence, E.g. Training a model would be better if the time taken is overall less). In general, we could care more about total runtime in total for the program. Therefore, more of a pro than a con.

### For a Copy on Write workload (in `dpcow_encow.c`) we can evaluate its strengths and weaknesses in these three graphs.

![Copy on Write: Percentage Children's Heap Written to vs KB Saved from Allocation](./graphs/cow_heapwrite_vs_pagesaved.png)
(Figure 4)

This demonstrates the space efficiency that is possible from sparse read/write in children workloads.

![Copy on Write: Percentage Children's Heap Written to vs Page Faults](./graphs/cow_heapwrite_vs_pagefault.png)
(Figure 5)

This demonstrates time efficiency overhead of copy on write, similar to demand paging. Say you have a 50% utilization of the heap, you'd have 15X more page faults than immediate allocation. 

However, this impact on my riscv-xv6 qemu simulation is not noticable quantitatively when measured with ticks. Ticks are accurate to about ~10ms. Therefore, the cost of page faulting has <= 10ms of impact.

![Copy on Write: Percentage Children's Heap Written to vs Page Faults](./graphs/cow_heapwrite_vs_allocationops.png)
(Figure 6)

[Similar discussion to Demand Paging above]


## Challenges Overcome

Several significant technical challenges were addressed during development:

1. **TLB Coherence**: Ensuring TLB was properly flushed after page table modifications to prevent stale entries from causing incorrect behavior.

   This is relevant when updating PTE's for DP and COW. At situations when we first touch a DP page (`trap.c`'s `usertrap`, `vm.c`'s `copyout`), we allocate physical memory for it and also we alter the PTE to change the DP page into a normal allocated page, therefore should update the TLB.

    When we fork for a COW page, we modify the PTE_C and PTE_W bits. On write, when we allocate for a PTE, we are modifying the PTE_C and PTE_W bits to change this into a normal page. Therefore we should update the TLB here as well.

2. **Race Conditions**: Carefully managing reference counts and page allocations to prevent races between processes sharing COW pages.

    `pa_track.h` implements the existing xv6 locking mechanisms to ensure that reference counts stay accurate for all processes, making sure that we make the correct COW decision for allocation in `access_trap_handler()` in `trap.c` and `kfree(...)` in `kalloc.c`.

    Locks are also implemented for tracking `memory_statistics`

3. **Chain Forking**: Handling scenarios where COW pages are further shared through subsequent forks.

    This was one of the most difficult oversights I faced during development. I wondered why on fork, the PTE_C prividges were disappearing (This was `usertests copyout`). Initially, I thought it was a memory corruption caused by some bad memset. It was not. If I was writing to a readonly page of a child, it meant that I was doing something wrong for pages when spawning the child process.
    
    I realized that I had to preserve the original writability of each page when doing COW, since inside of each process space, there existed readonly (e.g. textdata and code) content, along side the user modifiable content. 
    
    So, I went to claude. The LLM told me to create a global hashtable storing the information, and due to each virtual address can have different permissions to each physical address I may have to create a separate AVL tree for each process to store `writability`.

    After a bit of structuring, I thought, ok, lets implement this kernel add-on. But I asked myself: "woah woah woah, why the frick am I doing all this?" I realized that I could just initialize the readonly pages as PTE_C = 0, and not have them participate, since they can just be always shared (since non other operation will modify the pages).

    This resulted in the finalized logic branch of `uvmcopy` in `vm.c`

4. **Edge Cases**: Managing special cases like zero-length allocations, page boundary operations, and ensuring proper cleanup during process termination.

   Write checks are implemented throughout the data moving functions `copyout`, `copyinstr`, and `copyin`.

5. **Usertests Compatibility**: Resolving subtle interactions with existing xv6 usertests, particularly with memory-intensive operations.

   One of the tests, `sbrkfail`, is technically still not resolved. This revealed an interesting timing-related issue. Without any modifications, the test would fail despite correct memory management logic.
   
   GDB analysis revealed a race condition between parent and child processes during memory allocation failure handling. When a child process fails to allocate memory, it attempts to exit, but the parent's `wait()` call sometimes executes before the child can fully transition to the zombie state.
      
   Curiously, adding a printf statement before the `wait()` call resolves the issue. Other delay mechanisms did not have the same effect, suggesting this isn't merely a timing issue.Notably, this behavior occurs in vanilla xv6 as well.

   Further investigation revealed this is likely due to xv6's simplified scheduler and process state transition mechanisms. The printf syscall forces a context switch that allows the child process to complete its state transition to zombie before the parent's `wait()` call checks for it.

## Technical Design Decisions

### Memory Allocation Strategy
I chose to implement a lazy allocation approach where PTEs are initially marked with the `PTE_D` flag but no physical memory is allocated. This significantly reduces memory consumption for large allocations that may never be fully utilized.

### Reference Counting Implementation
For COW pages, I implemented a global reference counting mechanism rather than per-process tracking, which provides a more efficient use of kernel memory while still properly tracking shared pages.

### COW Permissions Handling
I carefully preserved original page permissions during fork() to ensure that read-only pages in the parent remain read-only in the child, while still applying COW semantics to writable pages.

### Fault Handler Design
The fault handler was designed to differentiate between:
- Demand paging faults (first access to mapped but unallocated memory)
- COW faults (write attempts to shared read-only pages)
- Invalid memory accesses (segmentation faults)

## Testing and Validation

The implementation was thoroughly tested using:

1. **Custom Test Programs**: Purpose-built programs to stress-test specific aspects of demand paging and COW. (`dpcow_cowtest.c`, `dpcow_basicmem.c`, `dpcow_exec.c`, `dpcow_forkch.c`, `dpcow_irl.c`)

2. **Modified xv6 Usertests**: The standard xv6 test suite was used to ensure compatibility with existing code. (`usertests.c`)

3. **Performance Benchmarks**: Comparative tests measuring memory usage and runtime performance. (`dpcow_dpeff.c`, `dpcow_encow.c`) 

## Future Work

Potential extensions to this project include:

1. **Page Replacement Algorithms**: Implementing FIFO, LRU, or Clock algorithms to manage memory under pressure.

2. **Swap Space Support**: Adding the ability to swap pages to disk when physical memory is scarce.

3. **Memory Compression**: Implementing page compression for infrequently accessed memory.

## Credits and References

This implementation was built on and inspired by:
- The xv6 operating system (MIT)
- "Operating Systems: Three Easy Pieces" by Remzi H. Arpaci-Dusseau and Andrea C. Arpaci-Dusseau
- Linux kernel's memory management subsystem

## Author

Ji (Jerry) Qi - University of Toronto

## License

This project is licensed under the MIT License - see the LICENSE file for details.
