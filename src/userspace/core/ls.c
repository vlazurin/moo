#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

typedef struct dirent {
	uint32_t d_ino;
	char d_name[256];
} dirent;

typedef struct DIR {
	int fd;
	int cur_entry;
    struct dirent ent;
} DIR;

int main(int argc, char *argv[])
{
    struct DIR *dir = NULL;
    if (argc > 1) {
        dir = opendir(argv[1]);
    } else {
        dir = opendir("./");
    }
    if (dir == NULL) {
        printf("ls opendir error: %d\n", errno);
        return 1;
    }

    struct dirent *ent = NULL;
    ent = readdir(dir);
    while (ent != NULL) {
        printf("%s   ", ent->d_name);
        ent = readdir(dir);
    }
    printf("\n");
    int err = closedir(dir);
    if (err) {
        printf("ls closedir error: %d\n", errno);
        return 1;
    }
    return 0;
}
