#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"

#define MAX_IN_LINE 80
#define COMMANDS_COUNT 16
#define MAX_LABEL_LENGTH 30

extern const char commands[16][4]; // Commands array from preAsm.h
int lineCounter = 1;

typedef struct Symbol {
    char *label;
    int adress;
    char *labelType;
}Symbol;

int isLabel(char* , char** , Symbol*);

void main(int argc , char *argv[]) {
    char **macroTbl = NULL;         // Table for macro names , for label validation
    int status = spreadMacros(argc , argv , &macroTbl);
    if (status != 1)
        exit(1);
    
    // מעבר ראשון ושני....
    // 101     106    203
    // HELLO   LOOP   STR   
    // code    code   data
    printf("Commencing first pass...\n");
    FILE *firstPass = fopen("FinalAsm.am", "r");
    int IC , DC;
    DC = IC = 0;
    Symbol *symbolTable = NULL; // will be used to store the symbols, their addresses and their type
    char line[MAX_IN_LINE];
    while (getLineFromFile(firstPass , line , NULL , lineCounter) != NULL) {
        char *token = strtok(line , " \t");
        while (token != NULL) {
            if ((token[0] == ';') || (strcmp(token , ".entry") == 0)) { // if the line is a comment OR if there is .entry (handeled in the second pass)
                break; // skip the rest of the line , go to the next line
            }
            if (isLabel(token , macroTbl , symbolTable) == 1) {
                char *labelNoColon = strtok(token, ":"); // remove the colon from the label
                symbolTable = realloc(symbolTable, (IC + 1) * sizeof(Symbol)); // reallocate memory for the symbol table
                symbolTable[IC].label = labelNoColon;
                symbolTable[IC].adress = IC; // set the address to the current instruction counter
                token = strtok(NULL, " \t"); // move to the next token after the label
                if (token[0] == '.') {
                    symbolTable[IC].labelType = "data"; // if the label is a data label

                }
                else {
                    symbolTable[IC].labelType = "code"; // if the label is a code label
                    
                }
                
            }
            token = strtok(NULL , " \t");
        }
        lineCounter++; // increase line counter
    }

    const int ICF = IC;
    const int DCF = DC;



}


int isLabel(char *token , char **macroTbl , Symbol *symbolTable) {
    if (strlen(token) >= MAX_LABEL_LENGTH + 1) { // MaxNameLength + 1 (for ':')
        fprintf(stderr, "Error: in line %d: Label '%s' is too long. Maximum length is %d characters.\n", lineCounter, token, MAX_LABEL_LENGTH);
        return 0;  // if more, not a valid label
    }
    if (token[strlen(token) - 1] != ':' || isalpha(token[0]) == 0) { // if the last char is not ':' or first letter isnt alphabet , it is not a label
        fprintf(stderr, "Error: in line %d: Label '%s' is not valid. Labels must start with a letter and end with a colon.\n", lineCounter, token);
        return 0; // not a valid label
    }
    Symbol *tmpPtr = symbolTable;
    char* labelNoColon = strtok(token, ":"); // remove the colon from the label
    while (tmpPtr != NULL) {
        if (strcmp(labelNoColon , tmpPtr->label) == 0) {
            fprrintf(stderr , "Error: in line %d: Label '%s' is already defined.\n",lineCounter , labelNoColon);
            return 0; // label already exists
        }
        tmpPtr++;
    }
    for (int i = 0; i < COMMANDS_COUNT; i++) { // check if the label is a command
        if (strcmp(labelNoColon, commands[i]) == 0) {
            fprintf(stderr, "Error: in line %d: Label '%s' cannot be a command name.\n",lineCounter, labelNoColon);
            return 0; // label is a command
        }
    }
    for (int i = 0; macroTbl[i] != NULL; i++) { // check if the label is a macro
        if (strcmp(token, macroTbl[i]) == 0) {
            fprintf(stderr, "Error: in line %d: Label '%s' cannot be a macro name.\n", lineCounter, token);
            return 0; // not a valid label
        }
    }
    return 1; // if label name didnt fail any of the checks, it is a valid label
}

int isOpcode(char *command) {
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(command, commands[i]) == 0)
            return 1;
    }
    return 0;
}
