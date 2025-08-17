#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util.h"

int errorCode = 0;
int symbolIdx = 0;
int binRepIdx = 0;
int ICF = 0;
int DCF = 0;
int extCount = 0;
int entryCount = 0;
Symbol *symbolTable = NULL;
Symbol *extTable = NULL;
BinRep *binRep = NULL;

const Opcode opcodes[16] = {
    { "mov",  "0000" , 2 , "0123" , "123"},
    { "cmp",  "0001" , 2 , "0123" , "0123"},
    { "add",  "0010" , 2 , "0123" , "123"},
    { "sub",  "0011" , 2 , "0123" , "123"},
    { "lea",  "0100" , 2 , "12" , "123"},
    { "clr",  "0101" , 1 , "-" , "123"},
    { "not",  "0110" , 1 , "-" , "123"},
    { "inc",  "0111" , 1 , "-" , "123"},
    { "dec",  "1000" , 1 , "-" , "123"},
    { "jmp",  "1001" , 1 , "-" , "123"},
    { "bne",  "1010" , 1 , "-" , "123"},
    { "jsr",  "1011" , 1 , "-" , "123"},
    { "red",  "1100" , 1 , "-" , "123"},
    { "prn",  "1101" , 1 , "-" , "0123"},
    { "rts",  "1110" , 0 , "-" , "-"},
    { "stop", "1111" , 0 , "-" , "-"}
};
const char commands[16][4] = { "mov" , "cmp" , "add" , "sub" , "lea" , "clr" ,"not" , "inc" , "dec" , "jmp" , "bne", "jsr" ,"red" ,"prn" , "rts" , "stop"};

/**
 * @param str string to trim
 * 
 * @returns:
 * pointer to the trimmed string (no spaces in start or end)
 */
char* removeStartEndSpaces(char* str) {
    while (*str == ' ' || *str == '\t') { // Trim starting spaces
        str++;
    } 
    if (*str == '\0') { // If the string is all spaces, return it
        return str;
    }
    char* end = str;
    while (*end != '\0') {  // Find the end of the string and trim end spaces
        end++;
    }
    end--;  // Move back to the last character
    while (end > str && (*end == ' ' || *end == '\t')) {
        end--;
    }
    end[1] = '\0';  // finish after last character
    return str; // return new string
}

/**
 * @param fp input file
 * @param line buffer for line
 * @param fileName only for error message
 * @param lineCounter error usage
 * 
 * @returns:
 * line from file input
 * 
 */
char* getLineFromFile(FILE* fp, char line[], char* fileName, int *lineCounter) {
    if (fgets(line, MAX_IN_LINE, fp) == NULL) {
        return NULL;  // End of file
    }
    
    // Remove newline character if present
    int len = strlen(line);
    if (len > 0 && line[len-1] == '\n') {
        line[len-1] = '\0';
    }
    if (len > MAX_IN_LINE) { // Check if line exceeds maximum length
        fprintf(stderr, "Error in file %s: line %d Line exceeds 80 characters\n", fileName , *lineCounter); // error msg
        errorCode = 1;
        return NULL; // return null to skip this file.
    }
    return line;
}
// Convert number to 8-bit binary string (two's complement for negative)
void decToBinary8Bit(int num, char* bin) {
    unsigned char val = (unsigned char)num;
    for (int i = 7; i >= 0; i--) {
        bin[7 - i] = ((val >> i) & 1) ? '1' : '0';
    }
    bin[8] = '\0';
}

// Convert int val (0-1023) to 10-bit binary string in bin buffer (must hold 11 chars)
void intToBinary10Bit(int val , char* bin) {
    for (int i = 9; i >= 0; i--) {
        bin[9 - i] = ((val >> i) & 1) ? '1' : '0';
    }
    bin[10] = '\0';
}

void binary10BitToBase4FiveBit(char *binaryString , char *buffer) {
    for (int i = 0; i < 5; i++) {
        char b1 = binaryString[2*i];
        char b2 = binaryString[2*i + 1];
        int value = (b1 - '0') * 2 + (b2 - '0');
        buffer[i] = 'a' + value; // map 0->a, 1->b, 2->c, 3->d
    }
    buffer[5] = '\0';
}

void decToBase4FourBits(int number, char *buffer) {
    char nums[4] = {'a','b','c','d'};
    for (int i = 3; i >= 0; i--) {
        buffer[i] = nums[number % 4];
        number /= 4;
    }
    buffer[4] = '\0';
}
