#ifndef PATRACKDEPS
#include "memlayout.h"
#include "types.h"
#define PATRACKDEPS
#endif

#define NPHYS (PHYSTOP / 4096)        // Number of physical pages

// This is more like a reference count, where a normal, non COW V page would have cow_refcount[ its pa ] = 1
extern uint64 cow_refcount[NPHYS];                 // COW count of physical page (where i = Address / PGSIZE)
extern struct spinlock cow_ref_lock;            // Locking to protect updates
