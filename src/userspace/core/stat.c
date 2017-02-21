#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("stat usage: stat %%path%%\n");
        return 1;
    }
    struct stat sb;
    if (stat(argv[1], &sb) != 0) {
        printf("stat error: %i\n", errno);
        return 1;
    }

    if (S_ISREG(sb.st_mode)) {
        printf("type: file\n");
    } else if (S_ISDIR(sb.st_mode)) {
        printf("type: directory\n");
    } else if (S_ISLNK(sb.st_mode)) {
        printf("type: symlink\n");
    } else if (S_ISCHR(sb.st_mode)) {
        printf("type: char device\n");
    } else if (S_ISBLK(sb.st_mode)) {
        printf("type: char device\n");
    }
    printf("size: %i bytes\n", sb.st_size);
    printf("blocks: %i\n", sb.st_blocks);
    return 0;
}
