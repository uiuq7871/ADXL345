#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>


#define DEVICE_FILE "/dev/adxl345"


int main() {
    int fd;
    int8_t data[3];  // 存儲讀取的 X, Y, Z

    fd = open(DEVICE_FILE, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open the device");
        return EXIT_FAILURE;
    }
while(1){
    ssize_t bytesRead = read(fd, data, sizeof(data));
    if (bytesRead < 0) {
        perror("Failed to read from the device");
        close(fd);
        return EXIT_FAILURE;
    }


    if (bytesRead != sizeof(data)) {
        fprintf(stderr, "Error: Number of bytes read is not correct\n");
        close(fd);
        return EXIT_FAILURE;
    }


    // 打印 X, Y, Z 值
    printf("ADXL345: X=%d, Y=%d, Z=%d\n", data[0], data[1], data[2]);
}
    close(fd);
    return EXIT_SUCCESS;
}
