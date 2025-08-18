#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"
#include "firstPass.h"
#include "util.h"
#include "binRep.h"

/**
 * @param op the opcode to get
 * @param srcAddr the source address
 * @param destAddr the destination address
 * @param bin the output binary string
 *
 * Builds the first word of the code in binary representation.
 */
void buildFirstWord(const Opcode* op, int srcAddr, int destAddr, char* bin) {
    int opcodeNum = 0;
    for (int i = 0; i < 4; i++) {
        opcodeNum <<= 1; // Shift left to make room for the next bit
        if (op->binaryRep[i] == '1') opcodeNum |= 1; // Get opcode number
    }
    int val = 0;
    val |= (opcodeNum << 6); // put the opcode number in bits 9-6
    val |= ((srcAddr & 3) << 4); // put the source operand addressing mode in bits 5-4
    val |= ((destAddr & 3) << 2); // put the destination operand addressing mode in bits 3-2
    val |= 0; // code type absolute = 00 for first word
    intToBinary10Bit(val, bin); // number to binary representation
}

/**
 * @param num the number to pack as immediate
 * @param bin the output binary string
 *
 * Packs an immediate value into a binary representation.
 */
void packImmediate(int num, char* bin) {
    char val8 = (char)num; // 8 bits for the number
    int val = val8 << 2; // bits 1-0 = 00
    intToBinary10Bit(val, bin); // number to binary representation
}

/**
 * @param regSrc the source register
 * @param regDest the destination register
 * @param bin the output binary string
 * 
 * Packs two registers into a binary representation.
 */
void packTwoRegisters(char* regSrc, char* regDest, char* bin) {
    int rSrc = (regSrc && regSrc[0] == 'r') ? (regSrc[1] - '0') : 0;
    int rDest = (regDest && regDest[0] == 'r') ? (regDest[1] - '0') : 0;
    int val = 0;
    val |= (rSrc << 6);   // bits 9-6 = source register number
    val |= (rDest << 2);  // bits 5-2 = destination register number
    // bits 1-0 = 00 absolute
    intToBinary10Bit(val, bin); // number to binary representation
}
/**
 * @param reg the register to pack
 * @param bin the output binary string
 * 
 * Packs a single register into a binary representation.
 */
void packSingleRegister(char* reg, char* bin) {
    int r = (reg && reg[0] == 'r') ? (reg[1] - '0') : 0; // Get register number
    int val = 0; 
    val |= (r << 6); // bits 9-6 = register number
    // bits 5-2 = 00
    intToBinary10Bit(val, bin); // number to binary representation
}

/**
 * @param operand the operand string
 * @param regA the first register
 * @param regB the second register
 * 
 * Extracts matrix registers from the operand string.
 * 
 */
void extractMatrixRegs(char* operand, char* regA, char* regB) {
    char* start = strchr(operand, '['); // Find the start of the first bracket
    if (!start) {
        regA[0] = '\0'; // No matrix registers found
        regB[0] = '\0'; // No matrix registers found
        return; 
    }
    char* end = strchr(start, ']'); // Find the end of the first bracket
    if (!end) { 
        regA[0] = '\0'; 
        regB[0] = '\0'; 
        return; 
    }
    int lengthA = end - (start + 1); // Length of the first matrix register
    strncpy(regA, start + 1, lengthA); // Copy the first matrix register
    regA[lengthA] = '\0'; // Null terminate the string

    char* start2 = strchr(end + 1, '['); // Find the start of the second bracket
    if (!start2) { 
        regB[0] = '\0'; 
        return; 
    }
    char* end2 = strchr(start2, ']'); // Find the end of the second bracket
    if (!end2) { 
        regB[0] = '\0'; 
        return; 
    }
    int lengthB = end2 - (start2 + 1); // Length of the second matrix register
    strncpy(regB, start2 + 1, lengthB); // Copy the second matrix register
    regB[lengthB] = '\0';
}
/**
 * @param operand the operand to get his address type
 * 
 * @return the address type: 0=immediate, 1=label, 2=matrix, 3=register, -1=error
 */
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
/**
 * @param opcode the opcode to find
 * 
 * @return the matching opcode, or NULL if not found
 */
const Opcode* getOpcode(char* opcode) {
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(opcode, opcodes[i].opcode) == 0) { // Compare opcode strings
            return &opcodes[i]; // Return matching opcode
        }
    }
    return NULL;
}
/**
 * @param operand1 first operand
 * @param operand2 second operand
 * @param srcAddr source address type
 * @param destAddr destination address type
 * @param outputBinary output binary string
 * @param maxLen maximum length of the output binary string
 *
 * This function appends the binary representation of the operands to the outputBinary string.
 */
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
            strncat(outputBinary, buffer, maxLen - strlen(outputBinary) - 1); // Append packed register to output
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1); // Add newline after packed register
        }
        else if (destAddr == 0) { // immediate
            int num = atoi(operand2 + 1);
            packImmediate(num, buffer); // Pack immediate value
            strncat(outputBinary, buffer, maxLen - strlen(outputBinary) - 1); // Append packed immediate to output
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1); // Add newline after packed immediate
        }
        else if (destAddr == 1) { // label
            strncat(outputBinary, operand2, maxLen - strlen(outputBinary) - 1); // Append label to output
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1); // Add newline after label
        }
        else if (destAddr == 2) { // matrix
            char label[100];
            int i = 0;
            while (operand2[i] && operand2[i] != '[') {
                label[i] = operand2[i];
                i++;
            }
            label[i] = '\0';
            strncat(outputBinary, label, maxLen - strlen(outputBinary) - 1); // Append matrix label to output
            strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1); // Add newline after matrix label

            char reg1[10], reg2[10];
            extractMatrixRegs(operand2, reg1, reg2); // Extract matrix registers
            if (reg1[0] && reg2[0]) {
                packTwoRegisters(reg1, reg2, buffer); // Pack matrix registers
                strncat(outputBinary, buffer, maxLen - strlen(outputBinary) - 1); // Append packed registers to output
                strncat(outputBinary, "\n", maxLen - strlen(outputBinary) - 1); // Add newline after packed registers
            }
            else {
                strncat(outputBinary, "0000000000\n", maxLen - strlen(outputBinary) - 1); // Append default matrix registers
            }
        }
    }
}
/**
 * @param line the line to convert
 * 
 * @return the binary representation of the line
 *
 * the function converts the line into its binary representation by checking
 * the type of "code" it contains and then converting it accordingly.
 */
char* codeToBinary(char *line) {
    char* binaryString = calloc((MAX_WORDS_FOR_CODE)*10, sizeof(char)); // Adjust size as needed
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
    
    int srcAddr = (operand1) ? getAdressType(operand1) : 0; // Determine source address type
    int destAddr = (operand2) ? getAdressType(operand2) : 0; // Determine destination address type
    if (op->expectedOperands == 1) { // If only one operand is expected
        destAddr = srcAddr; // The only operand is destination
        srcAddr = 0;        // No source
        operand2 = operand1;
        operand1 = NULL;    // So appendOperand knows it's only a dest
    }

    char firstWord[11];
    buildFirstWord(op, srcAddr, destAddr, firstWord); // Build the first word of the instruction
    strcat(binaryString, firstWord); // Add first word to binary string
    strcat(binaryString, "\n"); // Add newline after first word

    appendOperand(operand1, operand2, srcAddr, destAddr, binaryString, 2048); // Append operands to binary string

    free(lineCopy);
    return binaryString;
}
/**
 * @param line the line to convert
 * @param words the number of words in the line
 * @param lineCounter the line number in the source file
 * 
 * @return the binary representation of the line
 *
 * the function converts the line into its binary representation by checking
 * the type of data it contains and then converting it accordingly.
 */
char* dataToBinary(char *line , int words , int lineCounter) {
    char *binaryString = (char *)malloc((words*BITS_IN_WORD + NULL_TERMINATOR_LENGTH)*sizeof(char)); // Adjust size as needed
    if (!binaryString) return NULL;

    char *lineCopy = strdup(line);
    if (!lineCopy) {
        free(binaryString);
        return NULL;
    }

    lineCopy[strcspn(lineCopy, "\n")] = 0;  // Remove newline char

    char *dataOrlabel = strtok(lineCopy, " \t"); // A string which we later check whether it's a data directive or a label

    if (!dataOrlabel) {
        free(lineCopy);
        free(binaryString);
        return NULL;
    }

    int isData = strcmp(dataOrlabel, ".data") == 0; // Check if it's a .data
    int isString = strcmp(dataOrlabel, ".string") == 0; // Check if it's a .string
    int isMat = strcmp(dataOrlabel, ".mat") == 0; // Check if it's a .mat

    if (!isData && !isString && !isMat) { // If it's not a data, string, or matrix directive, then it must be a label
        dataOrlabel = strtok(NULL, " \t"); // If it's a label then we advance to the next token to see whether it's .data or .string or .mat or none of them
        if (!dataOrlabel) {
            free(lineCopy);
            free(binaryString);
            return NULL;
        }
        isData = strcmp(dataOrlabel, ".data") == 0;
        isString = strcmp(dataOrlabel, ".string") == 0;
        isMat = strcmp(dataOrlabel, ".mat") == 0;
    }

    if (isData) { // Handle .data
        char *currentNumber = strtok(NULL, ","); 
        char *ptrBinaryString = binaryString; // Pointer for building binary string
        char *lineCopyData = strdup(lineCopy);

        while (currentNumber != NULL) {
            char *value = removeStartEndSpaces(currentNumber); // Removing spaces
            int number = atoi(value); // Turning string to integer so we can use it in the intToBinary10Bit function
            char binaryNumber[11];
            intToBinary10Bit(number , binaryNumber);

            for (int i = 0; i < 10; i++) {
                *ptrBinaryString++ = binaryNumber[i]; // Copy binary number to string
            }
            *ptrBinaryString++ = '\n';
            currentNumber = strtok(NULL, ",");
        }
        *ptrBinaryString = '\0';
    } 
    else if(isString){ // Handle .string
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
    else if (isMat) { // Handle .mat
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
        char *openBracketRows = strchr(afterMat, '['); // Find opening bracket for rows
        if (!openBracketRows) {
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }
        rows = atoi(openBracketRows + 1);

        char *closeBracketRows = strchr(openBracketRows, ']'); // Find closing bracket for rows
        if (!closeBracketRows) {
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }

        // Find columns count
        char *openBracketCols = strchr(closeBracketRows + 1, '['); // Find opening bracket for columns
        if (!openBracketCols) {
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }
        cols = atoi(openBracketCols + 1);

        char *closeBracketCols = strchr(openBracketCols, ']'); // Find closing bracket for columns
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
            value = removeStartEndSpaces(value); // trim
            int number = atoi(value); // convert to integer
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
