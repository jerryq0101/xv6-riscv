#ifndef PA_TRACK_H
#define PA_TRACK_H
#include "memlayout.h"
#include "types.h"

#define NPHYS (PHYSTOP / 4096)        // Number of physical pages

// Global ref count tracker
// This is a reference count, where a normal, non COW V page would have cow_refcount[ its pa ] = 1
extern uint cow_refcount[NPHYS];                                // COW count of physical page (where i = Address / PGSIZE)
extern struct spinlock cow_ref_lock;                            // Locking to protect updates

#endif /* PA_TRACK_H */
