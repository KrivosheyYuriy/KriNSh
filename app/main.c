#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    // Uncomment this block to pass the first stage

    // Wait for user input
    char input[100];
    do {
        printf("$ ");
        fflush(stdout);
        fgets(input, 100, stdin);

        input[strlen(input) - 1] = '\0';

        if (!(strcmp(input, "exit") && strcmp(input, "\\q"))) // задание 3
            exit(0);
        printf("%s\n", input); // задание 1

    }
    while (!feof(stdin)); // задание 2
    return 0;
}
