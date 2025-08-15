#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "secondPass.h"

void secondPass(char *fileSrc) {
    printf("Commencing second pass...\n");
    FILE *secPass = fopen(fileSrc, "r");
    if (secPass == NULL) {
        fprintf(stderr, "Error: unable to open %s.\n" , fileSrc);
        return;
    }

    char line[MAX_IN_LINE];
    int lineCounter = 1;

    while(getLineFromFile(secPass , line , fileSrc , &lineCounter) != NULL) {
        char *lineCopy = strdup(line);
        if (line[0] == '\0' || line[0] == ';') {
            lineCounter++;
            continue; // Skip empty lines and comments
        }
        char *instruction = strtok(line, " \t");
        if (instruction[strlen(instruction)-1] == ':') {
            instruction = strtok(NULL, " \t"); // Get the actual instruction
        }
        if ( strcmp(instruction , ".string") == 0 || strcmp(instruction , "mat") == 0 || strcmp(instruction , ".data") == 0 ) continue; // Skip data directives
        if (strcmp(instruction , ".entry") == 0) {
            instruction = strtok(NULL, " \t");
            for (int i = 0; i < symbolIdx; i++) {
                if (strcmp(symbolTable[i].label, instruction) == 0) {
                    symbolTable[i].labelType = "entry"; // Mark as entry
                    break;
                }
            }
            fprintf(stderr , "Error: in line %d: Entry '%s' not found in symbol table.\n", lineCounter, instruction);
            errorCode = 1;
            continue;
        }
        
        lineCounter++; // Increment line counter for error messages
    }
}
