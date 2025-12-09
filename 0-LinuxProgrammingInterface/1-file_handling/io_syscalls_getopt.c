#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <getopt.h>

void print_usage(const char *prog_name) {
    printf("Usage: %s -f <file> -o <operation> -b <buffer> [-s <offset>]\n", prog_name);
    printf("Operations:\n");
    printf("  read\n");
    printf("  write\n");
    printf("  lseek\n");
    printf("  lseek_write\n");
}

int main(int argc, char *argv[]) {
    const char *file_path = NULL;
    const char *operation = NULL;
    const char *buffer = NULL;
    int offset = 0;

    int opt;
    while ((opt = getopt(argc, argv, "f:o:b:s:")) != -1) {
        switch (opt) {
            case 'f':
                file_path = optarg;
                break;
            case 'o':
                operation = optarg;
                break;
            case 'b':
                buffer = optarg;
                break;
            case 's':
                offset = atoi(optarg);
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (!file_path || !operation || !buffer) {
        print_usage(argv[0]);
        return 1;
    }

    int fd = open(file_path, O_RDWR, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (strcmp(operation, "read") == 0) {
        char read_buf[1024];
        ssize_t bytes_read = read(fd, read_buf, sizeof(read_buf) - 1);
        if (bytes_read == -1) {
            perror("read");
            close(fd);
            return 1;
        }
        read_buf[bytes_read] = '\0';
        printf("Read: %s\n", read_buf);
    } else if (strcmp(operation, "write") == 0) {
        ssize_t bytes_written = write(fd, buffer, strlen(buffer));
        if (bytes_written == -1) {
            perror("write");
            close(fd);
            return 1;
        }
        printf("Written: %s\n", buffer);
    } else if (strcmp(operation, "lseek") == 0) {
        off_t new_offset = lseek(fd, offset, SEEK_SET);
        if (new_offset == (off_t)-1) {
            perror("lseek");
            close(fd);
            return 1;
        }
        printf("Seeked to offset: %ld\n", new_offset);

        char read_buf[1024];
        ssize_t bytes_read = read(fd, read_buf, sizeof(read_buf) - 1);
        if (bytes_read == -1) {
            perror("read");
            close(fd);
            return 1;
        }
        read_buf[bytes_read] = '\0';
        printf("Read after seek: %s\n", read_buf);
    } else if (strcmp(operation, "lseek_write") == 0) {
        off_t new_offset = lseek(fd, offset, SEEK_SET);
        if (new_offset == (off_t)-1) {
            perror("lseek");
            close(fd);
            return 1;
        }
        printf("Seeked to offset: %ld\n", new_offset);

        ssize_t bytes_written = write(fd, buffer, strlen(buffer));
        if (bytes_written == -1) {
            perror("write");
            close(fd);
            return 1;
        }
        printf("Written after seek: %s\n", buffer);
    } else {
        print_usage(argv[0]);
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}