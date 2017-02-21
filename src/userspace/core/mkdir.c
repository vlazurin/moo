#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("mkdir usage: mkdir %%path%%\n");
        return 1;
    }

    int err = mkdir(argv[1], 0);
    if (err) {
        printf("mkdir error: %d\n", err);
        return 1;
    }

    return 0;
}
