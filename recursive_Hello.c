#include "stdio.h"
#include <stdio.h>

void printHello(int x) {
    if(x) {
        printf("Hello, World! num: %d\n", x);
        printHello(x - 1);
    }
}

int main() {
    int x = 0; // Default value 0
    
    printf("Enter number: ");
    x = getchar( );
    printf("\n");

    printf("\n\nNumber: %d\n\n", x);

    printHello(x);

    return 0;
}