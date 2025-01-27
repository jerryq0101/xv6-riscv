#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// void increment_state() {
//     int count = 0;
//     int fd = open("state.txt", O_RDONLY | O_CREATE);              // This will create the file on the Virtual file system, when first called
//         // The created file should be empty in the beginning, so we will read the EOF character

//     if (fd >= 0) {
//         if (read(fd, &count, sizeof(count)) < 0) {
//             printf("read failed\n");
//         }
//         close(fd);
//     } else {
//         printf("read open failed\n");
//     }
    
//     count++;
    
//     fd = open("state.txt", O_WRONLY | O_TRUNC);
//     if (fd >= 0) {
//         if (write(fd, &count, sizeof(count)) < 0) {
//             printf("write failed\n");
//         }
//         close(fd);
//     } else {
//         printf("write open failed\n");
//     }
// }

// int get_state() {
//     int fd = open("./state.txt", O_RDONLY);
//     int count = 0;
//     if (fd >= 0)                // Case the file doens't exist yet, so therefore 0
//     {
//         read(fd, &count, sizeof(count));
//         close(fd);
//     }
//     return count;
// }

int main(int arg, char *argv[])
{
        printf("Read count: %d\n", getreadcount());
        exit(0);
}
