#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define DEVICE_TYPE 'M' // Unique identifier for the device

// Define ioctl commands
#define IOCTL_READ  _IOR(DEVICE_TYPE, 1, int)       // Read an integer
#define IOCTL_WRITE _IOW(DEVICE_TYPE, 2, int)       // Write an integer
#define IOCTL_RDWR  _IOWR(DEVICE_TYPE, 3, struct my_data) // Read/Write struct

struct my_data {
    int val1;
    int val2;
};

int main() {
    int fd = open("/dev/my_device", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    int value = 0;
    struct my_data data = {10, 20};

    // Read an integer
    if (ioctl(fd, IOCTL_READ, &value) == 0) {
        printf("Value read from device: %d\n", value);
    }

    // Write an integer
    value = 100;
    if (ioctl(fd, IOCTL_WRITE, &value) == 0) {
        printf("Value written to device: %d\n", value);
    }

    // Read/Write a struct
    if (ioctl(fd, IOCTL_RDWR, &data) == 0) {
        printf("Modified data: val1=%d, val2=%d\n", data.val1, data.val2);
    }

    close(fd);
    return 0;
}