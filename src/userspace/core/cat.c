#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("cat usage: cat %%filename%%\n");
        return 1;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        printf("can't open file %s\n", argv[1]);
        return 1;
    }
    char buffer[100];
    int c = 0;
    while((c = read(fd, buffer, 100)) > 0) {
        write(STDOUT_FILENO, buffer, c);
    }
    return 0;
}
