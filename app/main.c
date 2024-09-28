#include <stdio.h>
#include <string.h>

int main() {
    // Uncomment this block to pass the first stage

    // Wait for user input
    char input[100];
    while (2) {
        printf("$ ");
        fflush(stdout);
        fgets(input, 100, stdin);

        input[strlen(input) - 1] = '\0';
        printf("%s: command not found\n", input);
    }
    return 0;
}
