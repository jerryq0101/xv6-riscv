#include "kernel/types.h"
#include "user/user.h"
#include "kernel/pstat.h"

// LOTTERY Testing
int main(int argc, char *argv[])
{
        if (argc != 4) {
                printf("Usage: %s <tickets_proc1> <tickets_proc2> <tickets_proc3>\n", argv[0]);
                exit(1);
        }

        for (int i = 1; i < 4; i++) {
                for (char *p = argv[i]; *p != '\0'; p++) {
                        if (*p < '0' || *p > '9') {
                                printf("Error: %s is not a valid number\n", argv[i]);
                                exit(1);
                        }
                }
        }
        
        // Process 1
        int proc_1 = fork();
        if (proc_1 == 0)
        {
                settickets(atoi(argv[1]));
                while (1) {/* keep it running */}
        }

        // Process 2
        int proc_2 = fork();
        if (proc_2 == 0)
        {
                settickets(atoi(argv[2]));
                while(1){ /* keep running */}
        }

        // Process 3
        int proc_3 = fork();
        if (proc_3 == 0)
        {
                settickets(atoi(argv[3]));
                while(1) {/*keep running*/}
        }

        printf("Process 1 Process 2 Process 3\n");
        for (int i = 0; i < 100; i++)
        {
                struct pstat ps;
                getpinfo(&ps);
                for (int i = 0; i < NPROC; i++)
                {
                        if (ps.pid[i] == proc_1)
                        {
                                printf("%d\t", ps.ticks[i]);
                        }
                        if (ps.pid[i] == proc_2)
                        {
                                printf("%d\t", ps.ticks[i]);
                        }
                        if (ps.pid[i] == proc_3)
                        {
                                printf("%d\t", ps.ticks[i]);
                        }
                }
                printf("\n");
                sleep(1);                       // Tick Measurements of every one second
        }

        kill(proc_1);
        kill(proc_2);
        kill(proc_3);

        return 0;
}
