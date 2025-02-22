#ifndef PA_TRACK_H
#define PA_TRACK_H
#include "memlayout.h"
#include "types.h"

#define NPHYS (PHYSTOP / 4096)        // Number of physical pages
#define COW_WRITABILITY_TABLE_SIZE 64

// Global ref count tracker
// This is a reference count, where a normal, non COW V page would have cow_refcount[ its pa ] = 1
extern volatile uint cow_refcount[NPHYS];                       // COW count of physical page (where i = Address / PGSIZE)
extern struct spinlock cow_ref_lock;                            // Locking to protect updates


// Per Process (inside PCB) Original-Writability Tracker (since COW resetting PTE_W permissions)
typedef struct cow_writable {
        uint64 va;
        int was_writable;
        cow_writable_t *next;
} cow_writable_t;

static inline int cow_hash(uint64 va)
{
        return (va / PGSIZE) % COW_WRITABILITY_TABLE_SIZE;
}

#endif /* PA_TRACK_H */
