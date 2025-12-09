#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>

void usage(const char *progname) {
    fprintf(stderr, "Usage: %s <filename>\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage(argv[0]);
    }

    const char *filename = argv[1];
    int fd;
    struct iovec iov[2];
    ssize_t nr;

    // Data to write
    char *buf1 = "Hello, ";
    char *buf2 = "world!\n";

    // Open the file for writing
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Initialize the iovec structures for writing
    iov[0].iov_base = buf1;
    iov[0].iov_len = strlen(buf1);
    iov[1].iov_base = buf2;
    iov[1].iov_len = strlen(buf2);

    // Write the data
    nr = writev(fd, iov, 2);
    if (nr == -1) {
        perror("writev");
        close(fd);
        exit(EXIT_FAILURE);
    }
    printf("Wrote %zd bytes\n", nr);

    close(fd);

    // Open the file for reading
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Buffers to read into
    char buf3[7];
    char buf4[7];

    // Initialize the iovec structures for reading
    iov[0].iov_base = buf3;
    iov[0].iov_len = sizeof(buf3) - 1;
    iov[1].iov_base = buf4;
    iov[1].iov_len = sizeof(buf4) - 1;

    // Read the data
    nr = readv(fd, iov, 2);
    if (nr == -1) {
        perror("readv");
        close(fd);
        exit(EXIT_FAILURE);
    }
    printf("Read %zd bytes\n", nr);

    // Null-terminate the buffers
    buf3[iov[0].iov_len] = '\0';
    buf4[iov[1].iov_len] = '\0';

    printf("Read data: '%s%s'\n", buf3, buf4);

    close(fd);
    return 0;
}