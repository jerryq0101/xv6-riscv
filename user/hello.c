#include "kernel/types.h"
#include "user/user.h"
// #include "stdio.h"
#pragma GCC diagnostic ignored "-Wunused-variable"

#define PGSIZE 4096

// void print_memory_statistics(struct memstat *s, int i, int start_time);

// Convert int to string (char array)
void itoa(int n, char *str)
{
        int i = 0, sign;

        // Handle negative numbers
        if ((sign = n) < 0)
                n = -n;

        // Generate digits in reverse order
        do
        {
                str[i++] = n % 10 + '0'; // Convert digit to ASCII
        } while ((n /= 10) > 0);

        // Add negative sign if needed
        if (sign < 0)
                str[i++] = '-';

        str[i] = '\0'; // Null-terminate the string

        // Reverse the string
        int j, k;
        char c;
        for (j = 0, k = i - 1; j < k; j++, k--)
        {
                c = str[j];
                str[j] = str[k];
                str[k] = c;
        }
}

int main(int argc, char *argv[])
{
        // The first part:
        // dpcow_dpeff
        // run different percentage of page accesses and calculate page savings
        // Max 50 Pages
        // Can't use files due to QEMU vm file system.
        for (int i = 1; i < 50; i++)           // Touching every page
        {
                // Input: pages_per_access
                int pages_per_access = i;
                char buffer[20];
                itoa(pages_per_access, buffer);

                if (fork() == 0)
                {
                        printf("i: %d ", i);
                        printf(" | ");
                        char *args[] = {"dpcow_dpeff", buffer, "0", 0};
                        exec("dpcow_dpeff", args);
                        exit(1);
                }
                wait(0);
        }

        // printf("\n------COW Feature Testing-------\n");
        // printf("\n------COW heap written to vs pages saved-------\n");

        // // dpcow_encow
        // // run different percentage of writes and calculate page savings
        // // 10 children
        // // 100 pages allocated in the parent, we decide how % of heap to write to in every children.
        // #define CONST_CHILDREN "10"
        // // 1 to 100 write percentage
        // for (int i = 1; i < 100; i++)           // Touching every page
        // {
        //         // Input: write percentage
        //         int write_perc = i;
        //         char buffer[20];
        //         itoa(write_perc, buffer);

        //         if (fork() == 0)
        //         {
        //                 printf("i: %d ", i);
        //                 printf(" | ");
        //                 char *args[] = {"dpcow_encow", CONST_CHILDREN, buffer, "0", 0};
        //                 exec("dpcow_encow", args);
        //                 exit(1);
        //         }
        //         wait(0);
        // }
        

        // The second part: Comparison of extra faults

        // // DP and COW: 
        // // Tracking extra faults and allocations for sample load
        // for (int i = 1; i < 50; i++)           // Touching every page
        // {
        //         // Input: pages_per_access
        //         int pages_per_access = i;
        //         char buffer[20];
        //         itoa(pages_per_access, buffer);

        //         if (fork() == 0)
        //         {
        //                 printf("i: %d ", i);
        //                 printf(" | ");
        //                 char *args[] = {"dpcow_dpeff", buffer, "1", 0};
        //                 exec("dpcow_dpeff", args);
        //                 exit(1);
        //         }
        //         wait(0);
        // }
        
        // printf("\n ------COW extra faults----- \n");

        // // COW extra faults
        // #define CONST_CHILDREN "10"
        // // 1 to 100 write percentage
        // for (int i = 1; i < 100; i++)           // Touching every page
        // {
        //         // Input: write percentage
        //         int write_perc = i;
        //         char buffer[20];
        //         itoa(write_perc, buffer);

        //         if (fork() == 0)
        //         {
        //                 printf("i: %d ", i);
        //                 printf(" | ");
        //                 char *args[] = {"dpcow_encow", CONST_CHILDREN, buffer, "1", 0};
        //                 exec("dpcow_encow", args);
        //                 exit(1);
        //         }
        //         wait(0);
        // }



        /// MORE SHIT BELOW WHEN I WAS SOLVING PROBLEMS ///

        // int a = fork();
        // if (a == 0)
        // {
        //         int arr[100000];
        //         for (int i = 0; i < 100000; i++)
        //         {
        //                 arr[i] = i;
        //         }
        // }

        // kill(a);
        // printf("Process og number %d\n", a);
        // int actual_thing = 0;
        // wait(&actual_thing);
        // printf("Waited process number%d\n", actual_thing);


        //////// SBRKFAIL

        // // test running fork with the above allocated page 
        // int pid = fork();
        // if(pid < 0){
        //         printf("fork failed\n");
        //         exit(1);
        // }
        // if (pid == 0)
        // {
        //         // allocate a lot of memory.
        //         // this should produce a page fault,
        //         // and thus not complete.
        //         // a = sbrk(0);
        //         #define BIG 4096 * 10
        //         sbrk(10 * BIG);
        //         int n = 0;
        //         for (int i = 0; i < 10 * BIG; i += PGSIZE)
        //         {
        //                 n += *(a + i);
        //         }
        //         // print n so the compiler doesn't optimize away
        //         // the for loop.
        //         printf("%s: allocate a lot of memory succeeded %d\n", s, n);
        //         exit(1);
        // }

        // // This print fixes everything - "gives enough time for the process to realize its been killed???"
        // //  printf("did we get here? 1\n");

        // wait(&xstatus);
        // printf("The actual xstatus: %d\n", xstatus);
        // if (xstatus != -1 && xstatus != 2)
        // {
        //         exit(1);
        // }

        ////////

        ////////////////// COPYINSTR ISSUE RESOLUTION SECTION START /////////////////

        // int pid = fork();

        // if (pid == 0)
        // {
        //         static char big[2];

        //         big[0] = 'x';
        //         big[1] = 'x';
                
        //         int i = 0;
        //         while (1)
        //         {
        //                 i++;
        //         }
        //         exit(34);
        // }

        // int exitcode = 0;
        // wait(&exitcode);

        // printf("Exit Code: %d\n", exitcode);

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

