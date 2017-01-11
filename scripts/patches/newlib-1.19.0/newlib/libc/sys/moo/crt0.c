#include <fcntl.h>

extern void exit(int code);
extern int main (int argc, char *argv[]);

void _start(int argc, char *argv[]) {
    int ex = main(argc, argv);
    exit(ex);
}
