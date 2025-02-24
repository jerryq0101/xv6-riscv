#include "kernel/types.h"
#include "user/user.h"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define PGSIZE 4096

// void print_memory_statistics(struct memstat *s, int i, int start_time);

int main(int argc, char *argv[])
{

        ////////////////// COPYINSTR ISSUE RESOLUTION SECTION START /////////////////

        int pid = fork();

        if (pid == 0)
        {
                static char big[2];

                big[0] = 'x';
                big[1] = 'x';
                
                int i = 0;
                while (1)
                {
                        i++;
                }
                exit(34);
        }

        int exitcode = 0;
        wait(&exitcode);

        printf("Exit Code: %d\n", exitcode);

        ////////////////// COPYINSTR ISSUE RESOLUTION SECTION END /////////////////


        ////////////////// COPYOUT ISSUE RESOLUTION SECTIO start /////////////////////////////////

        // int pid1 = fork();                                              // Doing a fork forces the uvmcopy situation to happen.

        // if (pid1 == 0)
        // {
        //         // open README
        //         int fd = open("README", 0);
        //         uint64 addr_should_err = 0LL;                           // 0, a virtual address pointing to the beginning of the page table, perhaps to code
        //         if(fd < 0){
        //                 printf("open(README) failed\n");
        //                 exit(1);
        //         }
        
        //         // Read from the file into 0LL
        //         int n = read(fd, (void *) addr_should_err, PGSIZE * 2);
        
        //         // Then see what happens
        //         if(n > 0){
        //                 printf("read(fd, %p, 8192) returned %d, not -1 or 0\n", (void*)addr_should_err, n);
        //                 exit(1);
        //         }
        //         close(fd);
        // }


        ////////////////// COPYOUT ISSUE RESOLUTION SECTION end /////////////////////////////////


        // // Get starting time in ticks
        // int start_time = uptime();
        
        // // Lazy allocating user memory (In this implementation) 
        // int *arr = malloc(PGSIZE * 1000 * sizeof(int));
        // struct memstat *s = malloc(sizeof(struct memstat));
        
        // for (int i = 0; i < PGSIZE * 1000; i+=PGSIZE)
        // {
        //         arr[i] += i;
        //         print_memory_statistics(s, i, start_time);
        // }

        // int childProcessID = fork();
        // if (childProcessID == 0)
        // {
        //         malloc(PGSIZE * 8);
        //         getmemstat(s);
        //         printf("Child at time %d: pages_curr_allocated: %ld, total_allocations: %ld\n", 
        //                uptime() - start_time, 
        //                s->total_allocated_pages, 
        //                s->total_allocations);
                
        //         // int a = 0;
        //         while (1)
        //         {
        //                 getpid();
        //         }
        //         exit(0);
        // }
        // int thing = 0;
        // sleep(1);
        // kill(childProcessID);
        // wait(&thing);

        // printf("Kill result: %d\n", thing);
        
        // getmemstat(s);
        // printf("AFTER RUN at time %d: pages_curr_allocated: %ld, total_allocations: %ld\n", 
        //        uptime() - start_time,
        //        s->total_allocated_pages, 
        //        s->total_allocations);

        // exit(0);
}

// void print_memory_statistics(struct memstat *s, int i, int start_time)
// {
//         getmemstat(s);
//         printf("time %d, index %d: pages_curr_allocated: %ld, total_allocations: %ld\n", 
//                uptime() - start_time,
//                i, 
//                s->total_allocated_pages, 
//                s->total_allocations);
// }

