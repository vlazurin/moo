#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

int main()
{
    int fd = open("/not exist", O_RDONLY);
    assert(fd == -ENOENT);
    fd = open("/home/hello.txt", O_RDONLY);
    assert(fd >= 0);
    return 0;
}
