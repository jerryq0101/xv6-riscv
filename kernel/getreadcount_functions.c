// kernel/getreadcount_functions.c
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "readcounterstate.h"

static int read_count = 0;  // Static counter in kernel memory

void increment_state(void) {
    read_count++;  // Simple increment
}

int get_state(void) {
    return read_count;
}
