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
    FILE *firstPass = fopen("FinalAsm.am", "r");
    if (firstPass == NULL) {
        fprintf(stderr, "Error opening FinalAsm.am\n");
        return NULL;
    }

    int IC = 100, DC = 0;
    Symbol *symbolTable = NULL;
    char line[MAX_IN_LINE];
    int lineCounter = 1;

    while (getLineFromFile(firstPass, line, "FinalAsm.am", &lineCounter) != NULL) {
        // Skip empty lines and comments
        char *originalLine = strdup(line);
        if (line[0] == '\0' || line[0] == ';') {
            continue;
        }

        char *token = strtok(line, " \t");
        if (token == NULL) continue;

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
            symbolTable[symbolIdx].label = strdup(token);
            symbolTable[symbolIdx].adress = 0; // Externs have no address in the first pass
            symbolTable[symbolIdx].labelType = "external";
            symbolIdx++;
            continue;
        }
        else if (strcmp(token, ".entry") == 0) {
            // Handled in second pass
            continue;
        }

        // Check for label
        int isLabel = 0;
        int labelLength = strlen(token);
        if (token[strlen(token)-1] == ':') {
            isLabel = 1;
            char label[MAX_LABEL_LENGTH];
            strncpy(label, token, sizeof(label));
            label[strlen(label)-1] = '\0';  // Remove colon

            // Validate label
            if (!isValidLabel(label, macroTbl, symbolTable , lineCounter)) {
                errorCode = 1;
                token = strtok(NULL, " \t");
                if (!token) continue;  // Skip line if no token after label
            }
            else {
                // Add label to symbol table
                symbolTable = realloc(symbolTable, (symbolIdx + 1) * sizeof(Symbol));
                symbolTable[symbolIdx].label = strdup(label);
                symbolTable[symbolIdx].adress = IC; // Set address to current IC
                symbolTable[symbolIdx].labelType = "code"; // Default type
                symbolIdx++;
            }

            token = strtok(NULL, " \t");
            if (!token) continue;
        }
        if (token[0] == '.') {
            int dataWords = countWordsForData(originalLine , lineCounter);
            if (dataWords < 0) {
                errorCode = 1;
            } 
            else {
                DC += dataWords;
                
                // If we had a label before, mark it as data
                if (isLabel && symbolIdx > 0) {
                    symbolTable[symbolIdx-1].labelType = "data";
                    symbolTable[symbolIdx-1].adress = DC - dataWords;
                }
            }
        }
        else {
            // Handle instructions
            int codeWords = countWordsForCode(originalLine , lineCounter);
            if (codeWords < 0) {
                errorCode = 1;
            } 
            else {
                IC += codeWords;
            }
        }
        lineCounter++; // Increment line counter for error messages
    }

    for (int i = 0; i < symbolIdx; i++) {
        if (strcmp(symbolTable[i].labelType , "data") == 0)
            symbolTable[i].adress += IC; // Adjust data addresses to follow code
    }
    fclose(firstPass);
    printf("First pass completed.");
    const int ICF = IC; // Instruction Counter Final
    const int DCF = DC; // Data Counter Final
    return symbolTable;
}

// Simplified isValidLabel function
int isValidLabel(char *label, char **macroTbl, Symbol *symbolTable , int lineCounter) {
    // Check length
    if (strlen(label) > MAX_LABEL_LENGTH) {
        fprintf(stderr, "Error: Label '%s' exceeds max length (%d)\n", 
                label, MAX_LABEL_LENGTH);
        return 0;
    }
    
    // Check first character
    if (!isalpha(label[0])) {
        fprintf(stderr, "Error: in line: %d Label '%s' must start with a letter\n", lineCounter , label);
        return 0;
    }
    
    // Check if reserved word
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(label, commands[i]) == 0) {
            fprintf(stderr, "Error: in line: %d Label '%s' is a reserved command\n", lineCounter , label);
            return 0;
        }
    }
    
    // Check if macro name
    if (macroTbl) {
        for (int i = 0; macroTbl[i] != NULL; i++) {
            if (strcmp(label, macroTbl[i]) == 0) {
                fprintf(stderr, "Error: in line: %d Label '%s' is a macro name\n", lineCounter , label);
                return 0;
            }
        }
    }
    
    // Check for duplicates
    for (int i = 0; i < symbolIdx; i++) {
        if (strcmp(label, symbolTable[i].label) == 0) {
            fprintf(stderr, "Error: in line: %d Duplicate label '%s'\n", lineCounter , label);
            return 0;
        }
    }
    
    return 1;
}

/**
 *  @param line The assembly code line to analyze (may be modified).
 *  @return The number of words required for the instruction, or 0 if an error occurs.
 *
 *  This function parses an instruction line, determines the instruction type,
 *  checks the number of operands, and calculates the required words based on operand types.
 *  It handles instructions with labels, validates operand count, and checks for register/memory addressing.
 **/
int countWordsForCode(char *line , int lineCounter){
    int words = 0;
    int oc = 0; // Operands count
    char *instruction, *operands[10], *opcode;
    char originalLine[100];
    strcpy(originalLine, line); // Copy for error messages

    line[strcspn(line,"\n")] = 0; // Remove newline character
    instruction = strtok(line, " \t"); // Get first token (instruction or label)

    if (instruction == NULL)
        return 0;

    if (instruction[strlen(instruction)-1] == ':') // If first token is a label, skip to instruction
        instruction = strtok(NULL, " \t");

    if (instruction == NULL)
        return 0;

    int expectedOperands = getExpectedOperandsCount(instruction);  // Get expected operand count for instruction
    if (expectedOperands == -1) {
        fprintf(stderr, "Error: in line %d: Unknown instruction '%s'", lineCounter , instruction);
        errorCode = 1;
        return 0;
    }

    words = 1; // Base word
    opcode = strtok(NULL, "");
    if (opcode != NULL) {
        char *p = strtok(opcode, ","); // Parse operands, separated by commas
        while (p && oc < 10) {
            operands[oc++] = removeStartEndSpaces(p);
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

/**
 * @param line The assembly data directive line to analyze (may be modified).
 * @return The number of words required for the data, or 0 if an error occurs.
 *
 * This function parses a line containing a data directive (.data, .string, or .mat),
 * validates its syntax, and calculates the number of words needed to store the data.
 * It handles labels, checks for errors in formatting, and supports matrix dimensions.
 **/
int countWordsForData(char *line , int lineCounter){
    int dc = 0; // data count

    char originalLine[100];
    strncpy(originalLine, line, sizeof(originalLine));
    originalLine[sizeof(originalLine)-1] = '\0';
    char *pointerToOriginalLine = removeStartEndSpaces(originalLine); // pointer to original line without spaces in the start

    char *label = strchr(pointerToOriginalLine, ':'); // If label exists, skip it
    if (label != NULL) pointerToOriginalLine = removeStartEndSpaces(label + 1);

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
        while (token != NULL) {
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
        if (open1 == NULL || close1 == NULL || close1 < open1) {
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
            while (token != NULL) {
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

int getExpectedOperandsCount(char *instr) {
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
