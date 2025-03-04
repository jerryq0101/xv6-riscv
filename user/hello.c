#include "kernel/types.h"
#include "user/user.h"
// #include "stdio.h"
#pragma GCC diagnostic ignored "-Wunused-variable"

#define PGSIZE 4096

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
        printf("\n------DP heap written to vs pages saved-------\n");
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

        printf("\n------COW heap written to vs pages saved-------\n");

        // dpcow_encow
        // run different percentage of writes and calculate page savings
        // 10 children
        // 100 pages allocated in the parent, we decide how % of heap to write to in every children.
        #define CONST_CHILDREN "10"
        // 1 to 100 write percentage
        for (int i = 1; i < 100; i++)           // Touching every page
        {
                // Input: write percentage
                int write_perc = i;
                char buffer[20];
                itoa(write_perc, buffer);

                if (fork() == 0)
                {
                        printf("i: %d ", i);
                        printf(" | ");
                        char *args[] = {"dpcow_encow", CONST_CHILDREN, buffer, "0", 0};
                        exec("dpcow_encow", args);
                        exit(1);
                }
                wait(0);
        }
        

        // The second part: Comparison of extra faults
        printf("\n------DP heap written to vs page faults-------\n");
        // DP and COW: 
        // Tracking extra faults and allocations for sample load
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
                        char *args[] = {"dpcow_dpeff", buffer, "1", 0};
                        exec("dpcow_dpeff", args);
                        exit(1);
                }
                wait(0);
        }
        
        printf("\n------COW heap written to vs page faults-------\n");

        // COW extra faults
        #define CONST_CHILDREN "10"
        // 1 to 100 write percentage
        for (int i = 1; i < 100; i++)           // Touching every page
        {
                // Input: write percentage
                int write_perc = i;
                char buffer[20];
                itoa(write_perc, buffer);

                if (fork() == 0)
                {
                        printf("i: %d ", i);
                        printf(" | ");
                        char *args[] = {"dpcow_encow", CONST_CHILDREN, buffer, "1", 0};
                        exec("dpcow_encow", args);
                        exit(1);
                }
                wait(0);
        }
}
