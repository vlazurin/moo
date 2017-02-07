#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc == 1) {
        while(1) {
            printf("y\n");
        }
    } else {
        while(1) {
            for (int i = 1; i < argc; i++) {
                printf("%s%c", argv[i], ((i + 1) < argc ? ' ' : '\0'));
            }
            printf("\n");
        }
    }
}
