#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
int errorCode = 0;
int symbolIdx = 0;
const Opcode opcodes[16] = {
    { "mov",  "0000" },
    { "cmp",  "0001" },
    { "add",  "0010" },
    { "sub",  "0011" },
    { "not",  "0100" },
    { "clr",  "0101" },
    { "lea",  "0110" },
    { "inc",  "0111" },
    { "dec",  "1000" },
    { "jmp",  "1001" },
    { "bne",  "1010" },
    { "red",  "1011" },
    { "prn",  "1100" },
    { "jsr",  "1101" },
    { "rts",  "1110" },
    { "stop", "1111" }
};
const char commands[16][4] = { "mov" , "cmp" , "add" , "sub" , "not" , "clr" ,"lea" , "inc" , "dec" , "jmp" , "bne", "red" ,"prn" ,"jsr" , "rts" , "stop"};

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
char* decToBinary10Bit(int num) {
    char binaryString[11];
    for (int i = 9; i >= 0; i--) {
        binaryString[9 - i] = ((num >> i) & 1) ? '1' : '0';
    }
    binaryString[10] = '\0'; // Null-terminate the string
    printf("%s\n" , binaryString);
    return strdup(binaryString);
}
