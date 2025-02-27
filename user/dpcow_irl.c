// realworld.c - Simulates a server fork/exec pattern
#include "kernel/types.h"
#include "user/user.h"

#define BASE_MEMORY (30*4096)    // 30 pages of base memory
#define CLIENT_COUNT 3           // Simulate 3 clients

// Represents a server handling concurrent requests
int main() {
  struct memstat start, end;
  getmemstat(&start);
  
  // Server initializes shared resources
  char *shared_data = malloc(BASE_MEMORY);
  for (int i = 0; i < BASE_MEMORY; i++) {
    shared_data[i] = i & 0xFF;  // Initialize all memory
  }
  
  printf("Server initialized with %d KB shared memory\n", BASE_MEMORY/1024);
  
  // Handle multiple client requests
  for (int client = 0; client < CLIENT_COUNT; client++) {
    int pid = fork();
    if (pid == 0) {
      // Child process - represents handling a client request
      printf("Child %d: Processing request\n", client);
      
      // Read shared data (benefits from COW)
      int checksum = 0;
      for (int i = 0; i < BASE_MEMORY; i++) {
        checksum += shared_data[i];
      }
      
      // Modify small portion (forces COW for this region)
      int client_region = BASE_MEMORY / CLIENT_COUNT;
      for (int i = 0; i < client_region; i++) {
        shared_data[client * client_region + i] = client;
      }
      
      // Some clients exec into a different program
      if (client % 2 == 0) {
        char *args[] = {"echo", "Client processed request", 0};
        exec("echo", args);
      }
      
      printf("Child %d: Request complete (checksum: %d)\n", client, checksum);
      exit(0);
    }
  }
  
  // Wait for all client handlers to complete
  for (int i = 0; i < CLIENT_COUNT; i++) {
    wait(0);
  }
  
  getmemstat(&end);
  
  // Print combined effectiveness metrics
  printf("\nCOMBINED EFFECTIVENESS:\n");
  printf("Demand page faults: %ld\n", end.demand_page_faults - start.demand_page_faults);
  printf("COW page faults: %ld\n", end.cow_page_faults - start.cow_page_faults);
  printf("Memory saved by demand paging: ~%ld pages\n", 
         (BASE_MEMORY/4096 * CLIENT_COUNT) - 
         (end.total_allocations - start.total_allocations));
  printf("Memory saved by COW: ~%ld pages\n", 
         (end.cow_pages_shared - start.cow_pages_shared) - 
         (end.cow_copies_made - start.cow_copies_made));
  printf("Total memory savings: ~%ld KB\n", 
         ((end.cow_pages_shared - start.cow_pages_shared) - 
          (end.cow_copies_made - start.cow_copies_made) +
          ((BASE_MEMORY/4096 * CLIENT_COUNT) - 
           (end.total_allocations - start.total_allocations))) * 4);
  
  free(shared_data);
  exit(0);
}
