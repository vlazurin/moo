#include <fcntl.h>

extern void exit(int code);
extern int main (int argc, char *argv[]);

char **environ = {0};

void _start(int argc, char *argv[], char *envp[]) {
    environ = envp;
    int ex = main(argc, argv);
    exit(ex);
}
