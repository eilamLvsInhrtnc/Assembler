/*
    *Eilam Gazit , Eyal Hets Cohen.
    *firstPass.c - first pass for assembler.c
    *this file handles the creation of the symbol table and partially creates the binary representation of the code.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"
#include "firstPass.h"
#include "util.h"
#include "binRep.h"

/**
 * @param fileDest The destination file for the first pass.
 * @param fileSrc The source file to read assembly code from.
 * 
 * this function is main function of the first pass.
 * it supervises the first pass and calls the necessary functions for it.
 */
void firstPass(char *fileDest , char *fileSrc) {
    char **macroTbl = NULL;
    int status = spreadMacros(fileDest , fileSrc , &macroTbl); // call pre-assembler.
    if (status == 1) {
        fprintf(stderr, "Macro processing failed\n");
        exit(1);
    }

    printf("Commencing first pass...\n");
    FILE *firstPass = fopen(fileDest, "r"); // .am file to read from.
    if (firstPass == NULL) {
        fprintf(stderr, "Error: unable to open %s.\n" , fileDest);
        return;
    }

    int IC = 100, DC = 0; // ic and dc initialization.
    char line[MAX_IN_LINE];
    int lineCounter = 1;

    while (getLineFromFile(firstPass, line, fileDest, &lineCounter) != NULL) {
        char *lineForCode = strdup(line);
        char *originalLine = strdup(line);
        if (line[0] == '\0' || line[0] == ';') { // skip empty lines and comments
            lineCounter++; 
            continue;
        }

        char *token = strtok(line, " \t");
        if (token == NULL) continue;

        if (strcmp(token, ".extern") == 0) { // handle extern.
            token = strtok(NULL, " \t");
            if (!token) {
                fprintf(stderr, "Error line %d: Missing label after .extern\n", lineCounter);
                errorCode = 1;
                continue;
            }
            symbolTable = realloc(symbolTable, (symbolIdx + 1) * sizeof(Symbol)); // add space
            symbolTable[symbolIdx].label = strdup(token); // copy label
            symbolTable[symbolIdx].adress = 0; // Externs have no address in the first pass
            symbolTable[symbolIdx].labelType = "external"; // external type
            symbolIdx++;
            lineCounter++;
            continue;
        }
        else if (strcmp(token, ".entry") == 0) { // entrys are handled in the second pass.
            entryCount++;
            lineCounter++;
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
                symbolTable = realloc(symbolTable, (symbolIdx + 1) * sizeof(Symbol)); // add space
                symbolTable[symbolIdx].label = strdup(label); // copy label
                symbolTable[symbolIdx].adress = IC; // Set address to current IC
                symbolTable[symbolIdx].labelType = "code"; // Default type
                symbolIdx++;
            }

            token = strtok(NULL, " \t");
            if (!token) continue;
        }
        if (token[0] == '.') { // if data line (.string .data .mat)
            int dataWords = countWordsForData(originalLine , lineCounter);
            if (dataWords < 0) {
                errorCode = 1;
            }
            else {
                binRep = realloc(binRep , (binRepIdx + 1) * sizeof(BinRep)); // add space 
                binRep[binRepIdx].lineType = "data"; // set line type
                binRep[binRepIdx].binaryString = strdup(dataToBinary(originalLine , dataWords , lineCounter)); // convert to binary
                binRep[binRepIdx].lineNumber = lineCounter; // Store line number for error messages
                binRep[binRepIdx].address = DC; // Set address for data
                binRepIdx++;
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
                binRep = realloc(binRep , (binRepIdx + 1) * sizeof(BinRep)); // add space
                binRep[binRepIdx].lineType = "code"; // set line type
                binRep[binRepIdx].binaryString = strdup(codeToBinary(lineForCode)); // convert to binary
                binRep[binRepIdx].lineNumber = lineCounter; // Store line number for error messages
                binRep[binRepIdx].address = IC; // Set address for code
                binRepIdx++;
                IC += codeWords;
            }
        }
        lineCounter++; // Increment line counter for error messages
    }

    for (int i = 0; i < symbolIdx; i++) {
        if (strcmp(symbolTable[i].labelType , "data") == 0)
            symbolTable[i].adress += IC; // Adjust data addresses to follow code
    }
    for (int j = 0; j < binRepIdx; j++) {
        if (strcmp(binRep[j].lineType , "data") == 0)
            binRep[j].address += IC; // Adjust data addresses to follow code
    }
    fclose(firstPass);
    if (IC + DC > TOTAL_WORDS) { // if total words exceed limit
        fprintf(stderr, "Error: Total words (%d) exceed limit (%d).\n", IC + DC, TOTAL_WORDS);
        errorCode = 1;
        return;
    }
    printf("First pass completed.\n");
    ICF = IC; // Instruction Counter Final
    DCF = DC; // Data Counter Final
    return;
}

/**
 * @param label The label to validate.
 * @param macroTbl macro table to check in
 * @param symbolTable symbol table to check in
 * @param lineCounter The current line number for error reporting.
 * 
 * @return 1 if the label is valid, 0 otherwise.
 */
int isValidLabel(char *label, char **macroTbl, Symbol *symbolTable , int lineCounter) {
    int len = strlen(label);
    if (len > MAX_LABEL_LENGTH) { // if label exceeds max length
        fprintf(stderr, "Error: Label '%s' exceeds max length (%d).\n", label, MAX_LABEL_LENGTH);
        errorCode = 1;
        return 0;
    }

    if (!isalpha(label[0])) { // first char must be alphabetic
        fprintf(stderr, "Error: in line %d: Label '%s' must start with an alphabetic character.\n", lineCounter , label);
        errorCode = 1;
        return 0;
    }

    for (int k = 0; k < len; k++) { // check if label contains only valid characters
        if (!(isalpha(label[k]) || isdigit(label[k]))) {
            fprintf(stderr, "Error: in line: %d unknown character in label %s.\n", lineCounter , label);
            errorCode = 1;
            return 0;
        }
    }
    
    for (int i = 0; i < COMMANDS_COUNT; i++) { // check if label is a command
        if (strcmp(label, commands[i]) == 0) {
            fprintf(stderr, "Error: in line: %d Label '%s' is a reserved command.\n", lineCounter , label);
            errorCode = 1;
            return 0;
        }
    }
    
    if (macroTbl != NULL) { // check if label is a macro
        for (int i = 0; macroTbl[i] != NULL; i++) {
            if (strcmp(label, macroTbl[i]) == 0) {
                fprintf(stderr, "Error: in line: %d Label '%s' is a macro name.\n", lineCounter , label);
                errorCode = 1;
                return 0;
            }
        }
    }
    
    for (int i = 0; i < symbolIdx; i++) { // check for duplicate labels in symbol table
        if (strcmp(label, symbolTable[i].label) == 0) {
            fprintf(stderr, "Error: in line: %d Duplicate label '%s'.\n", lineCounter , label);
            errorCode = 1;
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
    if (expectedOperands == -1) { // If instruction is unknown
        fprintf(stderr, "Error: in line %d: Unknown instruction '%s'\n", lineCounter , instruction);
        errorCode = 1;
        return 0;
    }

    words = 1; // Base word
    opcode = strtok(NULL, "");
    if (opcode != NULL) {
        char *p = strtok(opcode, ","); // tokenize operands, separated by commas
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
    strncpy(originalLine, line, strlen(originalLine));
    originalLine[strlen(originalLine)-1] = '\0';
    char *pointerToOriginalLine = removeStartEndSpaces(originalLine); // pointer to original line without spaces in the start

    char *label = strchr(pointerToOriginalLine, ':'); // If label exists, skip it
    if (label != NULL) pointerToOriginalLine = removeStartEndSpaces(label + 1);

    if (strncmp(pointerToOriginalLine, ".data", 5) == 0 && (isspace(pointerToOriginalLine[5]) || pointerToOriginalLine[5] == '\0')) {  // Handle .data directive
        char *params = removeStartEndSpaces(pointerToOriginalLine + 5);
        if (*params == ',' || params[strlen(params) - 1] == ',') {
            fprintf(stderr, "Error: in line %d: illegal comma.\n" , lineCounter); // Check for comma errors
            errorCode = 1;
            return 0;
        }
        for (int i = 0; params[i]; i++) {
            if (params[i] == ',' && params[i + 1] == ',') {
                fprintf(stderr, "Error: in line %d: illegal comma.\n", lineCounter); // Check for double commas
                errorCode = 1;
                return 0;
            }
        }
        char *token = strtok(params, ","); // Parse and count numbers
        while (token != NULL) {
            token = removeStartEndSpaces(token);
            if (!*token) { // Check for empty value between commas
                fprintf(stderr, "Error: in line %d: illegal comma.\n", lineCounter);
                errorCode = 1;
                return 0;
            }
            char *p = (*token == '+' || *token == '-') ? token + 1 : token; // Validate that token is a number and allow +/- signs
            for (; *p; p++) {
                if (!isdigit(*p)) {
                    fprintf(stderr, "Error: in line %d: invalid number.\n", lineCounter);
                    errorCode = 1;
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
            errorCode = 1;
            return 0;
        }
        while (*param && *param != '\"') { // Count characters until closing quote
            dc++;
            param++;
        }
        if (*param != '\"') {  // Check for closing quote
            fprintf(stderr, "Error: in line %d: missing \" .\n", lineCounter);
            errorCode = 1;
            return 0;
        }
        dc++; // for null terminator
    }
    else if (strncmp(pointerToOriginalLine, ".mat", 4) == 0 && (isspace(pointerToOriginalLine[4]) || pointerToOriginalLine[4] == '\0')) {  // Handle .mat directive
        char *args = removeStartEndSpaces(pointerToOriginalLine + 4);

        char *open1 = strchr(args, '['); // take size in brackets
        char *close1 = strchr(args, ']'); // take size in brackets
        if (open1 == NULL || close1 == NULL || close1 < open1) {
            fprintf(stderr, "Error: in line %d: invalid brackets.\n", lineCounter);
            errorCode = 1;
            return 0;
        }
        *close1 = '\0';
        int rows = atoi(open1 + 1);
        char *open2 = strchr(close1 + 1, '['); // take size in brackets
        char *close2 = strchr(close1 + 1, ']'); // take size in brackets
        if (!open2 || !close2 || close2 < open2) {  // if no second brackets or invalid
            fprintf(stderr, "Error: in line %d: invalid brackets.\n", lineCounter);
            errorCode = 1;
            return 0;
        }
        *close2 = '\0';
        int cols = atoi(open2 + 1);
        if (rows <= 0 || cols <= 0) { // if rows or cols are not positive
            fprintf(stderr, "Error: in line %d: invalid dimensions.\n", lineCounter);
            errorCode = 1;
            return 0;
        }
        int expectedCount = rows * cols; // expected amount of numbers
        char *valuesStart = removeStartEndSpaces(close2 + 1); // Parse and validate matrix values
        if (*valuesStart) {
            char *token = strtok(valuesStart, ", \t");
            while (token != NULL) { // check for commas and validate numbers
                token = removeStartEndSpaces(token);
                if (!*token) { // Check for empty matrix value
                    fprintf(stderr, "Error: in line %d: illegal comma.\n" , lineCounter);
                    errorCode = 1;
                    return 0;
                }
                char *p = (*token == '+' || *token == '-') ? token + 1 : token; // Validate that token is a number
                for (; *p; p++) {
                    if (!isdigit(*p)) {
                        fprintf(stderr, "Error: in line %d: invalid number.\n", lineCounter);
                        errorCode = 1;
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
        errorCode = 1;
        return 0;
    }
    return dc;

}
/**
 * @param instruction The instruction to check.
 * @return The expected number of operands for the instruction, or -1 if the instruction is
 */
int getExpectedOperandsCount(char *instruction) {
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(instruction, opcodes[i].opcode) == 0) {
            return opcodes[i].expectedOperands; // Return expected operand count for instruction
        }
    }
    return -1; // Return -1 if instruction not found
}
