#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    // Calculate the size of the new PATH env variable
    int path_len = strlen(getenv("PATH")) + 1;

    for (int i = 1; i < argc; ++i) {
        path_len += strlen(argv[i]) + 2;
    }

    // Concatenate the arguments and the current PATH
    char *path = malloc(path_len);
    strcpy(path, getenv("PATH"));

    for (int i = 1; i < argc; ++i) {
        strcat(path, ":");
        strcat(path, argv[i]);
    }

    // Set the new PATH variable
    path[path_len - 1] = '\0';
    setenv("PATH", path, 1);

    // Handle user input
    char *history[102];
    int pids[102], index = 0, shell_pid = getpid();

    for (;; ++index) {
        // Print prompt
        printf("$ ");
        fflush(stdout);

        // Get input from user
        char input[102] = { 0 };
        fgets(input, sizeof(input), stdin);

        // Update command history (pid will be updated later, if necessary)
        history[index] = strdup(input);
        pids[index] = shell_pid;

        // Split the input into command and arguments
        char *args[102] = {0}, *token = strtok(input, " \n");
        int i = 0;        

        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " \n");
        }

        args[i] = NULL;

        // Builtin command - exit
        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        // Builtin command - cd
        if (strcmp(args[0], "cd") == 0) {
            if (chdir(args[1]) == 0) {
                continue;
            }

            perror("cd failed");
            return 1;
        }

        // Built in command - history
        if (strcmp(args[0], "history") == 0) {
            for (int i = 0; i < index + 1; ++i) {
                printf("%d %s", pids[i], history[i]);
            }

            continue;
        }

        // Fork a child process
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            return 1;
        }

        if (pid == 0) {
            // In the child process, run the command
            if (execvp(args[0], args) == -1) {
                perror("execvp failed");
                return 1;
            }
        } else {
            // In the parent process, wait for the child to finish, and update pid history
            pids[index] = pid;
            wait(NULL);
        }
    }

    // Free allocated memory
    free(path);
    
    for (int i = 0; i < index; ++i) {
        free(history[i]);
    }

    return 0;
}