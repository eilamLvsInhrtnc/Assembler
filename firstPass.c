#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"
#include "firstPass.h"

#define MAX_IN_LINE 80
#define COMMANDS_COUNT 16
#define MAX_LABEL_LENGTH 30

extern const char commands[16][4];

int errorCode = 0; // Global error code for error handling
int symbolIdx = 0; // index for the symbol table

Symbol* firstPass(int argc, char *argv[]) {
    char **macroTbl = NULL;
    int status = spreadMacros(argc, argv, &macroTbl);
    if (status == 1) {
        fprintf(stderr, "Macro processing failed\n");
        return NULL;
    }

    printf("Commencing first pass...\n");
    FILE *fp = fopen("FinalAsm.am", "r");
    if (!fp) {
        fprintf(stderr, "Error opening FinalAsm.am\n");
        return NULL;
    }

    int IC = 100, DC = 0;
    Symbol *symbolTable = NULL;
    char line[MAX_IN_LINE];
    int lineCounter = 0;

    while (getLineFromFile(fp, line, "FinalAsm.am", &lineCounter) != NULL) {
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == ';') {
            continue;
        }

        char *token = strtok(line, " \t");
        if (!token) continue;

        // Handle directives
        if (strcmp(token, ".extern") == 0) {
            token = strtok(NULL, " \t");
            if (!token) {
                fprintf(stderr, "Error line %d: Missing label after .extern\n", lineCounter);
                errorCode = 1;
                continue;
            }

            // Add extern symbol
            symbolTable = realloc(symbolTable, (symbolIdx + 1) * sizeof(Symbol));
            symbolTable[symbolIdx] = (Symbol){
                .label = strdup(token),
                .adress = 0,
                .labelType = "external"
            };
            symbolIdx++;
            continue;
        }
        else if (strcmp(token, ".entry") == 0) {
            // Handled in second pass
            continue;
        }

        // Check for label
        int is_label = 0;
        if (token[strlen(token)-1] == ':') {
            is_label = 1;
            char label[MAX_LABEL_LENGTH];
            strncpy(label, token, sizeof(label));
            label[strlen(label)-1] = '\0';  // Remove colon

            // Validate label
            if (!isValidLabel(label, macroTbl, symbolTable)) {
                errorCode = 1;
                token = strtok(NULL, " \t");
                if (!token) continue;  // Skip line if no token after label
            }
            else {
                // Add label to symbol table
                symbolTable = realloc(symbolTable, (symbolIdx + 1) * sizeof(Symbol));
                symbolTable[symbolIdx] = (Symbol){
                    .label = strdup(label),
                    .adress = IC,
                    .labelType = "code"  // Default to code
                };
                symbolIdx++;
            }

            token = strtok(NULL, " \t");
            if (!token) continue;
        }

        // Handle data directives
        if (token[0] == '.') {
            int dataWords = countWordsForData(line);
            if (dataWords < 0) {
                errorCode = 1;
            } 
            else {
                DC += dataWords;
                
                // If we had a label before, mark it as data
                if (is_label && symbolIdx > 0) {
                    symbolTable[symbolIdx-1].labelType = "data";
                    symbolTable[symbolIdx-1].adress = DC - dataWords;
                }
            }
        }
        else {
            // Handle instructions
            int codeWords = countWordsForCode(line);
            if (codeWords < 0) {
                errorCode = 1;
            } 
            else {
                IC += codeWords;
            }
        }
    }

    fclose(fp);
    printf("First pass completed. IC=%d, DC=%d\n", IC, DC);
    return symbolTable;
}

// Simplified isValidLabel function
int isValidLabel(char *label, char **macroTbl, Symbol *symbolTable) {
    // Check length
    if (strlen(label) > MAX_LABEL_LENGTH) {
        fprintf(stderr, "Error: Label '%s' exceeds max length (%d)\n", 
                label, MAX_LABEL_LENGTH);
        return 0;
    }
    
    // Check first character
    if (!isalpha(label[0])) {
        fprintf(stderr, "Error: Label '%s' must start with a letter\n", label);
        return 0;
    }
    
    // Check if reserved word
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(label, commands[i]) == 0) {
            fprintf(stderr, "Error: Label '%s' is a reserved command\n", label);
            return 0;
        }
    }
    
    // Check if macro name
    if (macroTbl) {
        for (int i = 0; macroTbl[i] != NULL; i++) {
            if (strcmp(label, macroTbl[i]) == 0) {
                fprintf(stderr, "Error: Label '%s' is a macro name\n", label);
                return 0;
            }
        }
    }
    
    // Check for duplicates
    for (int i = 0; i < symbolIdx; i++) {
        if (strcmp(label, symbolTable[i].label) == 0) {
            fprintf(stderr, "Error: Duplicate label '%s'\n", label);
            return 0;
        }
    }
    
    return 1;
}

// Simplified countWordsForCode
int countWordsForCode(char *line) {
    // Just return 1 for now - we'll implement properly later
    return 1;
}

// Simplified countWordsForData
int countWordsForData(char *line) {
    // Just return 1 for now - we'll implement properly later
    return 1;
}
