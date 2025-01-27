#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int arg, char *argv[])
{
        printf("Read count: %d\n", getreadcount());
        exit(0);
}
