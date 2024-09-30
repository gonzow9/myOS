#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"
#include <ctype.h>

#define MAX_COMMANDS 10

int parseInput(char ui[]);

// Start of everything
int main(int argc, char *argv[]) {
    printf("Shell version 1.3 created September 2024\n");
    help();

    char prompt = '$';  				// Shell prompt
    char userInput[MAX_USER_INPUT];		// user's input stored here
    int errorCode = 0;					// zero means no error, default

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }
    
    //init shell memory
    mem_init();

    // Determine if we are in batch mode (input is from a file) or interactive mode
    // Returns 1 if interactive, 0 if batch mode
    int isInteractive = isatty(STDIN_FILENO);

    while(1) {		
        // Only display prompt in interactive mode					
        if (isInteractive) {
            printf("%c ", prompt);
        }

        if (fgets(userInput, MAX_USER_INPUT - 1, stdin) == NULL) {
            // If we reach EOF in batch mode, break the loop and return to interactive mode
            if (!isInteractive) {
                break;
            }
        }

        errorCode = parseInput(userInput);
        if (errorCode == -1) exit(99);	// ignore all other errors
        memset(userInput, 0, sizeof(userInput));
    }

    return 0;
}

int wordEnding(char c) {
    // You may want to add ';' to this at some point,
    // or you may want to find a different way to implement chains.
    return c == '\0' || c == '\n' || c == ' ';
}

int parseSingleInput(char inp[]) {
    char tmp[200], *words[100];                            
    int ix = 0, w = 0;
    int wordlen;
    int errorCode;
    for (ix = 0; inp[ix] == ' ' && ix < 1000; ix++); // skip white spaces
    while (inp[ix] != '\n' && inp[ix] != '\0' && ix < 1000) {
        // extract a word
        for (wordlen = 0; !wordEnding(inp[ix]) && ix < 1000; ix++, wordlen++) {
            tmp[wordlen] = inp[ix];                        
        }
        tmp[wordlen] = '\0';
        words[w] = strdup(tmp);
        w++;
        if (inp[ix] == '\0') break;
        ix++; 
    }
    errorCode = interpreter(words, w);
    return errorCode;
}

int parseInput(char inp[]) {
    char *commands[MAX_COMMANDS]; 
    int commandCount = 0;
    char *command;
    int errorCode = 0;

    // Split the input by semicolon (;)
    command = strtok(inp, ";");
    while (command != NULL && commandCount < 10) {
        commands[commandCount++] = command;
        command = strtok(NULL, ";");
    }

    // Execute each command one by one
    for (int i = 0; i < commandCount; i++) {
        // Remove leading and trailing spaces
        char *trimmedCommand = commands[i];
        while (isspace(*trimmedCommand)) trimmedCommand++;
        char *end = trimmedCommand + strlen(trimmedCommand) - 1;
        while (end > trimmedCommand && isspace(*end)) end--;
        end[1] = '\0';

        // Parse and execute the now single command
        errorCode = parseSingleInput(trimmedCommand);
        if (errorCode == -1) {
            break;  // Exit on fatal error (like `quit`)
        }
    }

    return errorCode;
}
