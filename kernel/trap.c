#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "kalloc.h"
#include "pa_track.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void access_trap_handler(void);

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap(void)
{
        int which_dev = 0;

        if ((r_sstatus() & SSTATUS_SPP) != 0)
                panic("usertrap: not from user mode");
        
        struct proc *p = myproc();

        // send interrupts and exceptions to kerneltrap(),
        // since we're now in the kernel.
        w_stvec((uint64)kernelvec);

        // Verify this is a user address and of adequate size
        uint64 va = r_stval();
        if (va >= MAXVA)
        {
                setkilled(p);
                goto done;              // go to the end immediately, skip walking when x >= MAXVA (as GROUNDDOWN(x) >= MAXVA and breaks the operation)
        }

        // check killed status to exit immediately
        if (killed(p))
                exit(-1);
        
        // save user program counter.
        p->trapframe->epc = r_sepc();
        
        if (r_scause() == 8)
        {
                // system call
                if (killed(p))
                        exit(-1);

                // sepc points to the ecall instruction,
                // but we want to return to the next instruction.
                p->trapframe->epc += 4;

                // an interrupt will change sepc, scause, and sstatus,
                // so enable only now that we're done with those registers.
                intr_on();

                syscall();
        }
        else if ((which_dev = devintr()) != 0)
        {
                // ok
        }
        else if (r_scause() == 12 || r_scause() == 15 || r_scause() == 13) // Page fault cases
        {
                // Update statistics
                acquire(&memory_statistics.lock);
                memory_statistics.user_faults++;
                release(&memory_statistics.lock);
                access_trap_handler();
        }
        else
        {
                pte_t *pte = walk(p->pagetable, va, 0);
                printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
                printf("            sepc=0x%lx stval=0x%lx\n pte=%ld\n", r_sepc(), r_stval(), *pte);
                setkilled(p);
        }

        done:
        if (killed(p = myproc()))
        {
                exit(-1);
        }

        // give up the CPU if this is a timer interrupt.
        if (which_dev == 2)
                yield();

        usertrapret();
}

void
access_trap_handler(void)
{

        // Reading an invalid page or writing an invalid page
        uint64 va = r_stval();
        struct proc *p = myproc();

        // Page align the faulting address
        va = PGROUNDDOWN(va);
        
        // Get the PTE if it exists
        pte_t *pte = walk(p->pagetable, va, 0);
        
        if(pte == 0) {
                // No PTE exists - this is an invalid access
                // printf("usertrap(): page not mapped pid=%d va=%p\n", p->pid, (void*)va);
                setkilled(p);
                return;
        }

        // Check if COW case
        if ((*pte & PTE_V) && !(*pte & PTE_W) && (*pte & PTE_C))
        {
                // Update statistics
                acquire(&memory_statistics.lock);
                memory_statistics.cow_page_faults++;
                release(&memory_statistics.lock);

                if(intr_get()) {
                        // printf("usertrap(): page fault in interrupt context\n");
                        setkilled(p);
                        return;
                }

                void *pa = (void *) PTE2PA(*pte);

                acquire(&cow_ref_lock);
                uint64 refs = cow_refcount[(uint64) pa / PGSIZE];
                release(&cow_ref_lock);

                if (refs > 1)
                {
                        void *mem = kalloc();                           // Also sets cow_refcount[mem]= 1
                        if (mem == 0)
                        {
                                // printf("usertrap(): kalloc failed pid=%d va=%p\n", p->pid, (void*)va);
                                setkilled(p);
                                return;
                        }
                        
                        void *prev = (void *) PTE2PA(*pte);
                        memmove(mem, prev, PGSIZE);                     // Copy page content into new page
                        *pte = PA2PTE(mem) | PTE_FLAGS(*pte);
                        *pte = (*pte | PTE_W) & ~PTE_C;

                        // Record cow copies made (User trap side)
                        acquire(&memory_statistics.lock);
                        memory_statistics.cow_copies_made++;
                        release(&memory_statistics.lock);

                        acquire(&cow_ref_lock);
                        cow_refcount[(uint64) prev / PGSIZE] -= 1;
                        release(&cow_ref_lock);
                }
                else            // refs = 1 (no need to allocate for another phys address)
                {
                        *pte = (*pte | PTE_W) & ~PTE_C;
                }
                sfence_vma();
                return;
        }
        // Check if this is actually a demand paging case
        else if((*pte & PTE_U) && (*pte & PTE_D) && !(*pte & PTE_V))
        {
                // Update statistics
                acquire(&memory_statistics.lock);
                memory_statistics.demand_page_faults++;
                release(&memory_statistics.lock);

                // Ensure we're not in an interrupt context
                if(intr_get()) {
                        // printf("usertrap(): page fault in interrupt context\n");
                        setkilled(p);
                        return;
                }

                // Allocate and map the page
                void *mem = kalloc_and_map(p->pagetable, pte);
                if(mem == 0) {
                        // printf("usertrap(): kalloc failed pid=%d va=%p\n", p->pid, (void*)va);
                        setkilled(p);
                        return;
                }
        } 
        else
        {
                // Not a demand paging case - invalid access
                // printf("usertrap(): invalid page access scause=%ld pid=%d va=%p pte=%p\n", r_scause(), p->pid, (void*)va, (void*)*pte);
                setkilled(p);
                return;
        }
}

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to userret in trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    // interrupt or trap from an unknown source
    printf("scause=0x%lx sepc=0x%lx stval=0x%lx status=0x%lx\n", scause, r_sepc(), r_stval(), sstatus);
    panic("kerneltrap");
  }
  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  if(cpuid() == 0){
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
  }

  // ask for the next timer interrupt. this also clears
  // the interrupt request. 1000000 is about a tenth
  // of a second.
  w_stimecmp(r_time() + 1000000);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if(scause == 0x8000000000000009L){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000005L){
    // timer interrupt.
    clockintr();
    return 2;
  } else {
    return 0;
  }
}

