#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"
#include "firstPass.h"
#include "util.h"
#include "binRep.h"

void buildFirstWord(const Opcode* op, int srcAddr, int destAddr, char* bin) {
    int opcodeNum = 0;
    for (int i = 0; i < 4; i++) {
        opcodeNum <<= 1;
        if (op->binaryRep[i] == '1') opcodeNum |= 1;
    }
    int val = 0;
    val |= (opcodeNum << 6); // put the opcode number in bits 9-6
    val |= ((srcAddr & 3) << 4); // put the source operand addressing mode in bits 5-4
    val |= ((destAddr & 3) << 2); // put the destination operand addressing mode in bits 3-2
    val |= 0; // code type absolute = 00 for first word
    intToBinary10Bit(val, bin); // number to binary representation
}

// Pack immediate value + 2 bits code type = 00
void packImmediate(int num, char* bin) {
    char val8 = (char)num; // 8 bits for the number
    int val = val8 << 2; // bits 1-0 = 00
    intToBinary10Bit(val, bin); // number to binary representation
}

// Pack two registers into one word: 4 bits src (bits 9-6), 4 bits dest (bits 5-2), 2 bits code 00
void packTwoRegisters(char* regSrc, char* regDest, char* bin) {
    int rSrc = (regSrc && regSrc[0] == 'r') ? (regSrc[1] - '0') : 0;
    int rDest = (regDest && regDest[0] == 'r') ? (regDest[1] - '0') : 0;
    int val = 0;
    val |= (rSrc << 6);   // bits 9-6 = source register number
    val |= (rDest << 2);  // bits 5-2 = destination register number
    // bits 1-0 = 00 absolute
    intToBinary10Bit(val, bin); // number to binary representation
}

// Pack single register operand: 4 bits reg (bits 9-6), 4 bits zero (bits 5-2), 2 bits code 00
void packSingleRegister(char* reg, char* bin) {
    int r = (reg && reg[0] == 'r') ? (reg[1] - '0') : 0;
    int val = 0;
    val |= (r << 6); // bits 9-6 = register number
    // bits 5-2 zero
    // bits 1-0 = 00 absolute
    intToBinary10Bit(val, bin);
}

// Extract matrix registers inside brackets: LABEL[rX][rY]
void extractMatrixRegs(char* operand, char* regA, char* regB) {
    char* start = strchr(operand, '[');
    if (!start) { regA[0] = '\0'; regB[0] = '\0'; return; }
    char* end = strchr(start, ']');
    if (!end) { regA[0] = '\0'; regB[0] = '\0'; return; }
    int lenA = end - (start + 1);
    strncpy(regA, start + 1, lenA);
    regA[lenA] = '\0';

    char* start2 = strchr(end + 1, '[');
    if (!start2) { regB[0] = '\0'; return; }
    char* end2 = strchr(start2, ']');
    if (!end2) { regB[0] = '\0'; return; }
    int lenB = end2 - (start2 + 1);
    strncpy(regB, start2 + 1, lenB);
    regB[lenB] = '\0';
}

int getAdressType(char *operand) {
    if (!operand) return -1;
    if (operand[0] == '#') return 0; // Immediate
    if (strchr(operand, '[')) return 2; // Matrix
    if (operand[0] == 'r' && isdigit(operand[1]) && operand[2] == '\0') {
        int regNum = operand[1] - '0';
        if (regNum >= 0 && regNum <= 7) return 3; // Register
    }
    return 1; // Label/direct
}

const Opcode* getOpcode(char* opcode) {
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(opcode, opcodes[i].opcode) == 0) {
            return &opcodes[i];
        }
    }
    return NULL;
}

void appendOperand(char* operand1, char* operand2, int srcAddr, int destAddr, char* outputBinary, int maxLen) {
    char buffer[20];

    // If both operands exist and both are registers, pack both into one word
    if (operand1 && operand2 && srcAddr == 3 && destAddr == 3) {
        packTwoRegisters(operand1, operand2, buffer);
        strncat(outputBinary, buffer, maxLen - strlen(outputBinary) - 1);
        strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1);
        return; // done - no separate words needed
    }

    // Else handle each operand separately
    if (operand1) {
        if (srcAddr == 3) { // single register
            packTwoRegisters(operand1 , "r0" , buffer); // pack with 0000.
            strncat(outputBinary, buffer, maxLen - strlen(outputBinary) - 1);
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1);
        }
        else if (srcAddr == 0) { // immediate
            int num = atoi(operand1 + 1);
            packImmediate(num, buffer);
            strncat(outputBinary, buffer, maxLen - strlen(outputBinary) - 1);
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1);
        }
        else if (srcAddr == 1) { // label
            strncat(outputBinary, operand1, maxLen - strlen(outputBinary) - 1);
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1);
        }
        else if (srcAddr == 2) { // matrix
            // print label part
            char label[100];
            int i = 0;
            while (operand1[i] && operand1[i] != '[') {
                label[i] = operand1[i];
                i++;
            }
            label[i] = '\0';
            strncat(outputBinary, label, maxLen - strlen(outputBinary) - 1);
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1);

            // pack matrix registers word
            char reg1[10], reg2[10];
            extractMatrixRegs(operand1, reg1, reg2);
            if (reg1[0] && reg2[0]) {
                packTwoRegisters(reg1, reg2, buffer);
                strncat(outputBinary, buffer, maxLen - strlen(outputBinary) - 1);
                strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1);
            }
            else {
                strncat(outputBinary, "0000000000\n", maxLen - strlen(outputBinary) - 1);
            }
        }
    }

    if (operand2) {
        if (destAddr == 3) { // single register
            packTwoRegisters("r0" , operand2 , buffer); // pack with 0000.
            strncat(outputBinary, buffer, maxLen - strlen(outputBinary) - 1);
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1);
        }
        else if (destAddr == 0) { // immediate
            int num = atoi(operand2 + 1);
            packImmediate(num, buffer);
            strncat(outputBinary, buffer, maxLen - strlen(outputBinary) - 1);
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1);
        }
        else if (destAddr == 1) { // label
            strncat(outputBinary, operand2, maxLen - strlen(outputBinary) - 1);
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1);
        }
        else if (destAddr == 2) { // matrix
            char label[100];
            int i = 0;
            while (operand2[i] && operand2[i] != '[') {
                label[i] = operand2[i];
                i++;
            }
            label[i] = '\0';
            strncat(outputBinary, label, maxLen - strlen(outputBinary) - 1);
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1);

            char reg1[10], reg2[10];
            extractMatrixRegs(operand2, reg1, reg2);
            if (reg1[0] && reg2[0]) {
                packTwoRegisters(reg1, reg2, buffer);
                strncat(outputBinary, buffer, maxLen - strlen(outputBinary) - 1);
                strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1);
            }
            else {
                strncat(outputBinary, "0000000000\n", maxLen - strlen(outputBinary) - 1);
            }
        }
    }
}

char* codeToBinary(char *line) {
    char* binaryString = calloc((MAX_WORDS_FOR_CODE)*10, sizeof(char));
    if (!binaryString) return NULL;

    char* lineCopy = strdup(line);
    if (!lineCopy) {
        free(binaryString);
        return NULL;
    }
    lineCopy[strcspn(lineCopy, "\n")] = 0;  // remove newline

    char* operation = strtok(lineCopy, " \t");
    if (operation[strlen(operation) - 1] == ':') { // skip label if present.
        operation = strtok(NULL, " \t");
    }
    if (!operation) {
        free(lineCopy);
        free(binaryString);
        return NULL;
    }

    const Opcode* op = getOpcode(operation);
    if (!op) {
        free(lineCopy);
        free(binaryString);
        return NULL;
    }

    char* operand1 = strtok(NULL, ",");
    char* operand2 = NULL;
    if (op->expectedOperands == 2) {
        operand2 = strtok(NULL, ",");
    }

    if (operand1) while (*operand1 == ' ' || *operand1 == '\t') operand1++;
    if (operand2) while (*operand2 == ' ' || *operand2 == '\t') operand2++;

    int srcAddr = (operand1) ? getAdressType(operand1) : 0;
    int destAddr = (operand2) ? getAdressType(operand2) : 0;
    if (op->expectedOperands == 1) {
        destAddr = srcAddr; // The only operand is destination
        srcAddr = 0;        // No source
        operand2 = operand1;
        operand1 = NULL;    // So appendOperand knows it's only a dest
    }

    char firstWord[11];
    buildFirstWord(op, srcAddr, destAddr, firstWord);
    strcat(binaryString, firstWord);
    strcat(binaryString, "\n");

    appendOperand(operand1, operand2, srcAddr, destAddr, binaryString, 2048);

    free(lineCopy);
    return binaryString;
}

char* dataToBinary(char *line , int words , int lineCounter) {
    char *binaryString = (char *)malloc((words*BITS_IN_WORD + NULL_TERMINATOR_LENGTH)*sizeof(char)); // Adjust size as needed
    if (!binaryString) return NULL;

    char *lineCopy = strdup(line);
    if (!lineCopy) {
        free(binaryString);
        return NULL;
    }

    lineCopy[strcspn(lineCopy, "\n")] = 0;  // Remove newline char

    char *dataOrlabel = strtok(lineCopy, " \t");

    if (!dataOrlabel) {
        free(lineCopy);
        free(binaryString);
        return NULL;
    }

    int isData = strcmp(dataOrlabel, ".data") == 0;
    int isString = strcmp(dataOrlabel, ".string") == 0;
    int isMat = strcmp(dataOrlabel, ".mat") == 0;

    if (!isData && !isString && !isMat) {
        dataOrlabel = strtok(NULL, " \t");
        if (!dataOrlabel) {
            free(lineCopy);
            free(binaryString);
            return NULL;
        }
        isData = strcmp(dataOrlabel, ".data") == 0;
        isString = strcmp(dataOrlabel, ".string") == 0;
        isMat = strcmp(dataOrlabel, ".mat") == 0;
    }

    if (isData) {
        char *currentNumber = strtok(NULL, ",");
        char *ptrBinaryString = binaryString;
        char *lineCopyData = strdup(lineCopy);

        while (currentNumber != NULL) {
            char *value = removeStartEndSpaces(currentNumber);
            int number = atoi(value);
            char binaryNumber[11];
            intToBinary10Bit(number , binaryNumber);

            for (int i = 0; i < 10; i++) {
                *ptrBinaryString++ = binaryNumber[i];
            }
            *ptrBinaryString++ = '\n';
            currentNumber = strtok(NULL, ",");
        }
        *ptrBinaryString = '\0';
    } 
    else if(isString){
        char *ptrBinaryString = binaryString;
        char *lineCopyString = strdup(line);
        char *stringStart = lineCopyString;
        while (*stringStart && *stringStart != '"') {
            stringStart++; // Move to the first quote
        }
        if (stringStart != NULL) {
            stringStart+=1; // Move past the opening quote
            char *stringEnd = strchr(stringStart, '"'); // Find closing quote
            if (stringEnd) {
                *stringEnd = '\0'; // Terminate string at closing quote
                for (char *pointer = stringStart; *pointer != '\0'; pointer++) {
                
                    int asciiValue = (int)(*pointer);
                    char binaryChar[11];
                    intToBinary10Bit(asciiValue , binaryChar);
                    for (int i = 0; i < 10; i++) {
                        *ptrBinaryString++ = binaryChar[i];
                    }
                    *ptrBinaryString++ = '\n';
                }

                // Add binary for null terminator character
                char nullBinary[11];
                intToBinary10Bit(0 , nullBinary);
                for (int i = 0; i < 10; i++) {
                    *ptrBinaryString++ = nullBinary[i];
                }
                *ptrBinaryString++ = '\n';
                *ptrBinaryString = '\0'; // Null terminate the full string
            }
            else {
                fprintf(stderr, "Error: in line %d: missing \" .\n", lineCounter);
                errorCode = 1;
                return NULL;
            }
        }
        else {
            fprintf(stderr, "Error: in line %d: missing \" .\n", lineCounter);
            errorCode = 1;
            return NULL;
        }
    }
    else if (isMat) { // Handle matrix directive
        char *lineCopyMat = strdup(line);
        if (!lineCopyMat) {
            free(binaryString);
            return NULL;
        }

        // Skip the ".mat" part to start parsing from after it
        char *afterMat = strstr(lineCopyMat, ".mat");
        if (!afterMat) {
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }
        afterMat += 4; // move pointer past ".mat"

        char *ptrBinaryString = binaryString;
        int rows = 0, cols = 0;

        // Find rows count
        char *openBracketRows = strchr(afterMat, '[');
        if (!openBracketRows) {
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }
        rows = atoi(openBracketRows + 1);

        char *closeBracketRows = strchr(openBracketRows, ']');
        if (!closeBracketRows) {
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }

        // Find columns count
        char *openBracketCols = strchr(closeBracketRows + 1, '[');
        if (!openBracketCols) {
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }
        cols = atoi(openBracketCols + 1);

        char *closeBracketCols = strchr(openBracketCols, ']');
        if (!closeBracketCols) {
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }

        // Start of matrix values
        char *valuesStart = closeBracketCols + 1;
        int expectedCount = rows * cols;
        int count = 0;

        char *value = strtok(valuesStart, ", \t");
        while(value != NULL) {
            value = removeStartEndSpaces(value);
            int number = atoi(value);
            char binaryNumber[11]; 
            intToBinary10Bit(number , binaryNumber);
            for (int i = 0; i < 10; i++) {
                *ptrBinaryString++ = binaryNumber[i];
            }
            *ptrBinaryString++ = '\n';
            count++;
            value = strtok(NULL, ", \t");
        }

        // Fill remaining cells with zeros if needed
        while (count < expectedCount) {
            char binaryNumber[11];
            intToBinary10Bit(0 , binaryNumber);
            for (int i = 0; i < 10; i++) {
                *ptrBinaryString++ = binaryNumber[i];
            }
            *ptrBinaryString++ = '\n';
            count++;
        }

        *ptrBinaryString = '\0'; // Null-terminate final string
        free(lineCopyMat);
    }

    else {
        free(lineCopy);
        free(binaryString);
        return NULL;
    }

    free(lineCopy);
    return binaryString;
}
