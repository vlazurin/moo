#include <fcntl.h>

extern void exit(int code);
extern int main (int argc, char **argv);
char *p[3][10] = {
{"dash"},
{"a1"},
{"a2"}
};
void _start() {
    int ex = main(3, &p);
    exit(ex);
}
