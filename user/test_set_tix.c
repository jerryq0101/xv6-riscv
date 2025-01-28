#include "kernel/types.h"
#include "user/user.h"
#include "kernel/pstat.h"

int main(int argc, char *argv[])
{
        printf("Testing ticket setting\n");
        int num = atoi(argv[1]);

        printf("Ticket setting status %d\n", settickets(num));

        // get current stats
        struct pstat stat;
        if (getpinfo(&stat) < 0) {
            printf("getpinfo failed\n");
            exit(1);
        }

        printf("| INUSE | Tickets | PID | Ticks |\n");
        for (int i = 0; i < NPROC; i++)
        {
                printf("%d      %d      %d      %d\n", stat.inuse[i], stat.tickets[i], stat.pid[i], stat.ticks[i]);
        }
        
        exit(0);
}
