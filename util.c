/*
    *Eilam Gazit , Eyal Hets Cohen.
    *util.c - utility functions for assembler.c
    *this file has utility functions that are used throughout the project.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util.h"

int errorCode = 0; // initializing global error code
int symbolIdx = 0; // initializing symbolTable index
int binRepIdx = 0; // initializing binRep index
int ICF = 0; // initializing final instruction counter
int DCF = 0; // initializing final data counter
int extCount = 0; // initializing external symbols count
int entryCount = 0; // initializing entry symbols count
Symbol *symbolTable = NULL; // initializing symbol table
Symbol *extTable = NULL; // initializing external symbols table
BinRep *binRep = NULL; // initializing binary representation table

const Opcode opcodes[16] = { // initializing opcode map.
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
const char commands[16][4] = { "mov" , "cmp" , "add" , "sub" , "lea" , "clr" ,"not" , "inc" , "dec" , "jmp" , "bne", "jsr" ,"red" ,"prn" , "rts" , "stop"}; // initializing commands map.

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
/**
 * @param num number to convert
 * @param bin buffer to hold binary string
 * 
 * Convert number to 8-bit binary string 
 */
void decToBinary8Bit(int num, char* bin) {
    for (int i = 7; i >= 0; i--) {
        bin[7 - i] = ((num >> i) & 1) ? '1' : '0';
    }
    bin[8] = '\0';
}

/**
 * @param num number to convert
 * @param bin buffer to hold binary string
 * 
 * Convert number to 10-bit binary string 
 */
void intToBinary10Bit(int num , char* bin) {
    for (int i = 9; i >= 0; i--) {
        bin[9 - i] = ((num >> i) & 1) ? '1' : '0';
    }
    bin[10] = '\0';
}
/**
 * @param binaryString 10-bit binary string
 * @param buffer buffer to hold base 4 string
 * 
 * Convert binaryString to 5-bit base 4 string 
 */
void binary10BitToBase4FiveBit(char *binaryString , char *buffer) {
    for (int i = 0; i < 5; i++) {
        char b1 = binaryString[2*i];
        char b2 = binaryString[2*i + 1];
        int value = (b1 - '0') * 2 + (b2 - '0');
        buffer[i] = 'a' + value;
    }
    buffer[5] = '\0';
}
/**
 * @param number number to convert
 * @param buffer buffer to hold base 4 string
 * 
 * Convert number to 4-bit base 4 string
 */
void decToBase4FourBits(int number, char *buffer) {
    char nums[4] = {'a','b','c','d'};
    for (int i = 3; i >= 0; i--) {
        buffer[i] = nums[number % 4];
        number /= 4;
    }
    buffer[4] = '\0';
}
