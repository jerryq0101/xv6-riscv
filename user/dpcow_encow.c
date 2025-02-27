// enhanced_cowtest.c - Tests COW with different write patterns
#include "kernel/types.h"
#include "user/user.h"

#define MEMORY_SIZE (100*4096)  // 100 pages
#define CHILD_COUNT 5           // Create 5 children
#define WRITE_PERCENT 5         // Each child writes to 5% of memory

int main() {
  struct memstat before, after;
  getmemstat(&before);
  
  // Allocate and initialize memory
  char *memory = malloc(MEMORY_SIZE);
  for (int i = 0; i < MEMORY_SIZE; i++) {
    memory[i] = i & 0xFF;  // Initialize all memory
  }
  
  printf("Parent allocated %d pages\n", MEMORY_SIZE/4096);
  
  // Create multiple children
  int pids[CHILD_COUNT];
  for (int c = 0; c < CHILD_COUNT; c++) {
    if ((pids[c] = fork()) == 0) {
      // Child process - modify only a small section of memory
      int start_offset = (MEMORY_SIZE / CHILD_COUNT) * c;
      int write_size = MEMORY_SIZE * WRITE_PERCENT / 100;
      
      printf("Child %d writing to %d bytes starting at offset %d\n", 
             c, write_size, start_offset);
             
      // Write to a small portion - this triggers COW only for these pages
      for (int i = 0; i < write_size; i++) {
        memory[start_offset + i] = 0xFF;
      }
      
      // Sleep to give parent time to collect stats
      sleep(c + 1);
      exit(0);
    }
  }
  
  // Wait for all children
  for (int c = 0; c < CHILD_COUNT; c++) {
    wait(0);
  }
  
  getmemstat(&after);
  
  // Calculate metrics
//   uint64 total_virtual_pages = MEMORY_SIZE/4096 * CHILD_COUNT;
  uint64 pages_shared = after.cow_pages_shared - before.cow_pages_shared;
  uint64 pages_copied = after.cow_copies_made - before.cow_copies_made;
  uint theoretical_no_cow = MEMORY_SIZE/4096 * CHILD_COUNT;
  
  printf("\nCOW EFFECTIVENESS:\n");
  printf("Total processes: %d (parent + %d children)\n", CHILD_COUNT + 1, CHILD_COUNT);
  printf("Total memory size: %d pages\n", MEMORY_SIZE/4096);
  printf("Without COW would require: %d pages\n", theoretical_no_cow);
  printf("Pages shared through COW: %ld\n", pages_shared);
  printf("Pages copied due to writes: %ld\n", pages_copied);
  printf("Memory saved by COW: %ld pages (%ld KB)\n", 
         pages_shared - pages_copied,
         (pages_shared - pages_copied) * 4);
  
         // Integer-only efficiency calculation
  uint64 efficiency_pct = (1000 * (pages_shared - pages_copied)) / theoretical_no_cow;
  printf("Memory efficiency: %ld.%ld%%\n", 
         efficiency_pct / 10, 
         efficiency_pct % 10);
  
  free(memory);
  exit(0);
}
