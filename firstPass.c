#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"
#include "firstPass.h"

#define MAX_IN_LINE 80
#define COMMANDS_COUNT 16
#define MAX_LABEL_LENGTH 30

extern const char commands[16][4]; // Commands array from preAsm.h

typedef struct Symbol {
    char *label;
    int adress;
    char *labelType;
}Symbol;

int lineCounter = 1;
int errorCode = 0; // Global error code for error handling

Symbol* firstPass(int argc , char *argv[]) {
    char **macroTbl = NULL;         // Table for macro names , for label validation
    int status = spreadMacros(argc , argv , &macroTbl);
    if (status != 1)
        exit(1);
    
    printf("Commencing first pass...\n");
    FILE *firstPass = fopen("FinalAsm.am", "r");
    int IC = 100 , DC = 0;
    int symbolIdx = 0; // index for the symbol table
    Symbol *symbolTable = NULL; // will be used to store the symbols, their addresses and their type
    char line[MAX_IN_LINE];
    while (getLineFromFile(firstPass , line , NULL , lineCounter) != NULL) {
        char *token = strtok(line , " \t");
        while (token != NULL) {
            if ((token[0] == ';') || (strcmp(token , ".entry") == 0)) { // if the line is a comment OR if there is .entry (handeled in the second pass)
                break; // skip the rest of the line , go to the next line
            }
            if (strcmp(token , ".extern")) {
                token = strtok(NULL, " \t");
                if (token == NULL) {
                    fprintf(stderr, "Error: in line %d: No label found.\n", lineCounter);
                    errorCode = 1; // set error code
                    continue; // skip to next line
                }
                symbolTable = realloc(symbolTable, (symbolIdx + 1) * sizeof(Symbol)); // reallocate memory for the symbol table
                symbolIdx++;
                symbolTable[symbolIdx].label = token;
                checkDupe(symbolTable, symbolIdx, token); // check for duplicate labels
                symbolTable[symbolIdx].adress = 0; // set the address to the current instruction counter
                symbolTable[symbolIdx].labelType = "external"; // set the label type to extern
                
            }
            int L = countWordsForCode(line); // count the number of words in the line
            if (L == 0) {
                fprintf(stderr, "Error: in line %d: No valid instruction found.\n", lineCounter);
                errorCode = 1; // set error code
                continue; // skip to next line
            }
            if (isLabel(token , macroTbl , symbolTable) == 1) {
                char *labelNoColon = strtok(token, ":"); // remove the colon from the label
                symbolTable = realloc(symbolTable, (symbolIdx + 1) * sizeof(Symbol)); // reallocate memory for the symbol table
                symbolIdx++;
                symbolTable[symbolIdx].label = labelNoColon;
                checkDupe(symbolTable, symbolIdx, labelNoColon); // check for duplicate labels
                symbolTable[symbolIdx].adress = IC; // set the address to the current instruction counter
                IC += L; // increase the instruction counter by the number of words in the line
                token = strtok(NULL, " \t"); // move to the next token after the label
                if (token[0] == '.') {
                    symbolTable[symbolIdx].labelType = "data"; // if the label is a data label
                    IC -=L; // if the label is a data label, we do not increase the instruction counter
                    symbolTable[symbolIdx].adress = DC;
                    DC += countWordsForData(line); // increase the data counter by the number of words in the line
                }
                else {
                    symbolTable[symbolIdx].labelType = "code"; // if the label is a code label
                }
                
            }
            token = strtok(NULL , " \t");
        }
        lineCounter++; // increase line counter
    }

    const int ICF = IC;
    const int DCF = DC;


    return symbolTable; // End of first pass
}


int isLabel(char *token , char **macroTbl , Symbol *symbolTable) {
    if (strlen(token) >= MAX_LABEL_LENGTH + 1) { // MaxNameLength + 1 (for ':')
        fprintf(stderr, "Error: in line %d: Label '%s' is too long. Maximum length is %d characters.\n", lineCounter, token, MAX_LABEL_LENGTH);
        errorCode = 1; // set error code
        return 0;  // if more, not a valid label
    }
    if (token[strlen(token) - 1] != ':' || isalpha(token[0]) == 0) { // if the last char is not ':' or first letter isnt alphabet , it is not a label
        fprintf(stderr, "Error: in line %d: Label '%s' is not valid. Labels must start with a letter and end with a colon.\n", lineCounter, token);
        errorCode = 1; // set error code
        return 0; // not a valid label
    }
    Symbol *tmpPtr = symbolTable;
    char* labelNoColon = strtok(token, ":"); // remove the colon from the label
    while (tmpPtr != NULL) {
        if (strcmp(labelNoColon , tmpPtr->label) == 0) {
            fprrintf(stderr , "Error: in line %d: Label '%s' is already defined.\n",lineCounter , labelNoColon);
            errorCode = 1; // set error code
            return 0; // label already exists
        }
        tmpPtr++;
    }
    for (int i = 0; i < COMMANDS_COUNT; i++) { // check if the label is a command
        if (strcmp(labelNoColon, commands[i]) == 0) {
            fprintf(stderr, "Error: in line %d: Label '%s' cannot be a command name.\n",lineCounter, labelNoColon);
            errorCode = 1; // set error code
            return 0; // label is a command
        }
    }
    for (int i = 0; macroTbl[i] != NULL; i++) { // check if the label is a macro
        if (strcmp(token, macroTbl[i]) == 0) {
            fprintf(stderr, "Error: in line %d: Label '%s' cannot be a macro name.\n", lineCounter, token);
            errorCode = 1; // set error code
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

int getExpectedOperandsCount(const char *instr) {
    if (strcmp(instr, "mov") == 0 || strcmp(instr, "cmp") == 0 ||
        strcmp(instr, "add") == 0 || strcmp(instr, "sub") == 0 ||
        strcmp(instr, "lea") == 0) {
        return 2;
    }
    if (strcmp(instr, "clr") == 0 || strcmp(instr, "not") == 0 ||
        strcmp(instr, "inc") == 0 || strcmp(instr, "dec") == 0 ||
        strcmp(instr, "jmp") == 0 || strcmp(instr, "bne") == 0 ||
        strcmp(instr, "jsr") == 0 || strcmp(instr, "red") == 0 ||
        strcmp(instr, "prn") == 0) {
        return 1;
    }
    if (strcmp(instr, "rts") == 0 || strcmp(instr, "stop") == 0) {
        return 0;
    }
    return -1; // invalid instruction
}
/**
 *  @param line The assembly code line to analyze (may be modified).
 *  @return The number of words required for the instruction, or 0 if an error occurs.
 * 
 *  This function parses an instruction line, determines the instruction type,
 *  checks the number of operands, and calculates the required words based on operand types.
 *  It handles instructions with labels, validates operand count, and checks for register/memory addressing.
 */
int countWordsForCode(char *line){
    int words = 0; 
    int oc = 0; // Operands count
    char *instruction, *operands[10], *opcode;
    char originalLine[100];
    strcpy(originalLine, line); // Copy for error messages

    line[strcspn(line,"\n")] = 0; // Remove newline character
    instruction = strtok(line, " \t"); // Get first token (instruction or label)

    if (!instruction)
        return 0;

    if (instruction[strlen(instruction)-1] == ':') // If first token is a label, skip to instruction
        instruction = strtok(NULL, " \t");

    if (!instruction)
        return 0;

    int expectedOperands = getExpectedOperandsCount(instruction);  // Get expected operand count for instruction
    if (expectedOperands == -1) {
        fprintf(stderr, "Error: Unknown instruction '%s' in line: %s\n", instruction, originalLine);
        errorCode = 1;
        return 0;
    }

    words = 1; // Base word
    opcode = strtok(NULL, "");
    if (opcode) {
        char *p = strtok(opcode, ","); // Parse operands, separated by commas
        while (p && oc < 10) {
            operands[oc++] = removeSpaces(p);
            p = strtok(NULL, ",");
        }
    }

    if (oc != expectedOperands) { // Check operand count
        fprintf(stderr, "Error: in line %d: Instruction '%s' expects %d operands but got %d.\n", lineCounter , instruction, expectedOperands, oc);
        errorCode = 1;
        return 0;
    }
    if (oc == 2 && isReg(operands[0]) && isReg(operands[1])) { // If both operands are registers, add 1 word
        words += 1;
        return words;
    }
    for (int i = 0; i < oc; i++) { // For each operand, add words based on type
        if (strchr(operands[i], '['))
            words += 2; // Memory addressing
        else
            words += 1; // Regular operand
    }

    return words;
}

int isReg(char *op) {
    if (op[0] != 'r') return 0;
    if (!isdigit(op[1])) return 0;
    if (op[2] != '\0') return 0;
    return op[1] >= '0' && op[1] <= '7';
}

int checkDupe(Symbol* symbolTbl , int symbolIdx, char* token) {
    for (int i = 0; i < symbolIdx; i++) {
        if (strcmp(symbolTbl[i].label, token) == 0) {
            fprintf(stderr, "Error: in line %d: Label '%s' is already defined.\n", lineCounter, token);
            errorCode = 1; // set error code
            return 0; // label already exists
        }
    }
    return 1; // label is unique
}

/**
 * @param line The assembly data directive line to analyze (may be modified).
 * @return The number of words required for the data, or 0 if an error occurs.
 * 
 * This function parses a line containing a data directive (.data, .string, or .mat),
 * validates its syntax, and calculates the number of words needed to store the data.
 * It handles labels, checks for errors in formatting, and supports matrix dimensions.
 */
int countWordsForData(char *line){
    int dc = 0; // data count

    char originalLine[100];
    strncpy(originalLine, line, sizeof(originalLine));
    originalLine[sizeof(originalLine)-1] = '\0';
    char *pointerToOriginalLine = removeStartEndSpaces(originalLine); // pointer to original line without spaces in the start

    char *label = strchr(pointerToOriginalLine, ':'); // If label exists, skip it
    if (label) pointerToOriginalLine = removeStartEndSpaces(label + 1);

    if (strncmp(pointerToOriginalLine, ".data", 5) == 0 && (isspace(pointerToOriginalLine[5]) || pointerToOriginalLine[5] == '\0')) {  // Handle .data directive
        char *params = removeStartEndSpaces(pointerToOriginalLine + 5);
        if (*params == ',' || params[strlen(params) - 1] == ',') {
            fprintf(stderr, "Error: in line %d: illegal comma.\n" , lineCounter); // Check for comma errors
            return 0;
        }
        for (int i = 0; params[i]; i++) {
            if (params[i] == ',' && params[i + 1] == ',') {
                fprintf(stderr, "Error: in line %d: illegal comma.\n", lineCounter); // Check for double commas
                return 0;
            }
        }   
        char *token = strtok(params, ","); // Parse and count numbers
        while (token) {
            token = removeStartEndSpaces(token);
            if (!*token) { // Check for empty value between commas
                fprintf(stderr, "Error: in line %d: illegal comma.\n", lineCounter);
                return 0;
            }
            char *p = (*token == '+' || *token == '-') ? token + 1 : token; // Validate that token is a number and allow +/- signs
            for (; *p; p++) {
                if (!isdigit(*p)) {
                    fprintf(stderr, "Error: in line %d: invalid number.\n", lineCounter);
                    return 0;
                }
            }
            dc++; // Count valid data number
            token = strtok(NULL, ",");
        }
    } 
    else if (strncmp(pointerToOriginalLine, ".string", 7) == 0 && (isspace(pointerToOriginalLine[7]) || pointerToOriginalLine[7] == '\0')) { // Handle .string directive
        char *param = removeStartEndSpaces(pointerToOriginalLine + 7);
        if (*param++ != '\"') { // Must start with a quote
            fprintf(stderr, "Error: in line %d: missing \" .\n", lineCounter);
            return 0;
        }
        while (*param && *param != '\"') { // Count characters until closing quote
            dc++;
            param++;
        }
        if (*param != '\"') {  // Check for closing quote
            fprintf(stderr, "Error: in line %d: missing \" .\n", lineCounter);
            return 0;
        }
        dc++; // for null terminator
    } 
    else if (strncmp(pointerToOriginalLine, ".mat", 4) == 0 && (isspace(pointerToOriginalLine[4]) || pointerToOriginalLine[4] == '\0')) {  // Handle .mat directive
        char *args = removeStartEndSpaces(pointerToOriginalLine + 4);

        // parse rows
        char *open1 = strchr(args, '[');
        char *close1 = strchr(args, ']');
        if (!open1 || !close1 || close1 < open1) {
            fprintf(stderr, "Error: in line %d: invalid brackets.\n", lineCounter);
            return 0;
        }
        // parse columns
        *close1 = '\0';
        int rows = atoi(open1 + 1);
        char *open2 = strchr(close1 + 1, '[');
        char *close2 = strchr(close1 + 1, ']');
        if (!open2 || !close2 || close2 < open2) {
            fprintf(stderr, "Error: in line %d: invalid brackets.\n", lineCounter);
            return 0;
        }
        *close2 = '\0';
        int cols = atoi(open2 + 1);
        if (rows <= 0 || cols <= 0) {
            fprintf(stderr, "Error: in line %d: invalid dimensions.\n", lineCounter);
            return 0;
        }
        int expectedCount = rows * cols;
        char *valuesStart = removeStartEndSpaces(close2 + 1); // Parse and validate matrix values
        if (*valuesStart) {
            char *token = strtok(valuesStart, ", \t");
            while (token) {
                token = removeStartEndSpaces(token);
                if (!*token) { // Check for empty matrix value
                    fprintf(stderr, "Error: in line %d: illegal comma.\n" , lineCounter);
                    return 0;
                }
                char *p = (*token == '+' || *token == '-') ? token + 1 : token; // Validate that token is a number
                for (; *p; p++) {
                    if (!isdigit(*p)) {
                        fprintf(stderr, "Error: in line %d: invalid number.\n", lineCounter);
                        return 0;
                    }
                }
                token = strtok(NULL, ", \t");
            }
        }
        dc = expectedCount; // Set data count to expected matrix size
    } 
    else {
        fprintf(stderr, "Error: in line %d: No directive found\n" , lineCounter);
        return 0;
    }
    return dc;

}
