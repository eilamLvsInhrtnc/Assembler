/*
    *Eilam Gazit , Eyal Hets Cohen.
    *secondPass.c - second pass for assembler.c
    *this file finishes the binary representation and the creation of the object, entry, and external files. 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "secondPass.h"
/**
 * @param fileSrc The source file to read assembly code from.
 * 
 * this function is main function of the second pass.
 * it finishes the binary representation of the code and creates the object, entry, and external files and writes the needed data into them.
 */
void secondPass(char *fileSrc) {
    printf("Commencing second pass...\n");
    FILE *secPass = fopen(fileSrc, "r"); // open .am file to read from.
    if (secPass == NULL) { // if file pointer didnt open
        fprintf(stderr, "Error: unable to open %s.\n" , fileSrc);
        return;
    }

    char line[MAX_IN_LINE + NULL_TERMINATOR_LENGTH]; // line buffer
    int lineCounter = 1; // initialize line counter

    while(getLineFromFile(secPass , line , fileSrc , &lineCounter) != NULL) { // while there are lines to read
        char *lineCopy = strdup(line);
        if (line[0] == '\0' || line[0] == ';') {
            lineCounter++;
            continue; // Skip empty lines and comments
        }
        char *instruction = strtok(line, " \t");
        if (instruction[strlen(instruction)-1] == ':') {
            instruction = strtok(NULL, " \t"); // Get the actual instruction
        }
        if ( strcmp(instruction , ".string") == 0 || strcmp(instruction , "mat") == 0 || strcmp(instruction , ".data") == 0 ) continue; // Skip data directives
        if (strcmp(instruction , ".entry") == 0) {
            int updated = 0; // Flag to check if entry was found
            instruction = strtok(NULL, " \t");
            for (int i = 0; i < symbolIdx; i++) {
                if (strcmp(symbolTable[i].label, instruction) == 0) {
                    symbolTable[i].labelType = "entry"; // Mark as entry
                    updated = 1; // found entry
                    break;
                }
            }
            if (updated == 0) { // if not found, error.
                fprintf(stderr , "Error: in line %d: Entry '%s' not found in symbol table.\n", lineCounter, instruction);
                errorCode = 1;
                continue;
            }
        }
        for (int i = 0; i < binRepIdx; i++) { // find the binary representation of the current line.
            if (binRep[i].lineNumber == lineCounter) { // found at index i
                // Process multi-line binaryString (split by '\n')
                char *temp = strdup(binRep[i].binaryString); // Create a copy to avoid modifying original
                char *token = strtok(temp, "\n");
                char newBinary[100]; // new binary string to buffer for the new binary representation
                newBinary[0] = '\0'; // Initialize the empty string
                int lineCount = 0; 
                while (token != NULL) { // while there are tokens to process
                    // If token is not pure binary there must be a label
                    if (strspn(token, "01-") != strlen(token)) { // check if the line has a label if there is, enter the if.
                        int found = 0; // Flag to check if label is found
                        token = removeStartEndSpaces(token);
                        for (int j = 0; j < symbolIdx; j++) {
                            if (strcmp(symbolTable[j].label, token) == 0) {
                                found = 1; // found label
                                char binary[BITS_IN_WORD + NULL_TERMINATOR_LENGTH];
                                if (strcmp(symbolTable[j].labelType, "external") == 0) { // if label is external
                                    decToBinary8Bit(0 , binary); // external = 0000000001
                                    extTable = realloc(extTable, (extCount + 1) * sizeof(Symbol)); // add space for new external symbol
                                    extTable[extCount].label = strdup(symbolTable[j].label);
                                    extTable[extCount].adress = binRep[i].address + lineCount; // address for .ext file
                                    extTable[extCount].labelType = "external";
                                    extCount++;
                                    strcat(binary , "01"); // external coding.
                                } 
                                else {
                                    decToBinary8Bit(symbolTable[j].adress, binary);
                                    strcat(binary , "10"); // data coding
                                }

                                strcat(newBinary, binary);
                                strcat(newBinary, "\n");
                                break;
                            }
                        }
                        if (found == 0) { // if label not found
                            fprintf(stderr, "Error: in line %d: Label '%s' not found in symbol table.\n", lineCounter, token);
                            errorCode = 1;
                        }
                    } 
                    else {
                        strcat(newBinary, token); // if no label, binary string is already complete so just append.
                        strcat(newBinary, "\n");
                    }
                    lineCount++;
                    token = strtok(NULL, "\n");
                }
                // Replace original binaryString with finished version
                free(binRep[i].binaryString);
                binRep[i].binaryString = strdup(newBinary);
                free(temp);
            }
        }
        lineCounter++; // Increment line counter for error messages
    }
    fclose(secPass);
    if (errorCode == 1) {
        exit(1);
    }
    loadInFiles(fileSrc); // Load in files after second pass
}
/**
 * @param fileSrc The source file to change its extention to .ob , .ent , .ext
 * 
 * this function creates the .ob , .ent , .ext files and writes the needed data into them.
 */
void loadInFiles(char *fileSrc) {
    char buffer[6];
    char *objectFile = objectFileName(fileSrc); // get .ob file name
    FILE *obFile = fopen(objectFile, "w"); // open .ob file to write to
    if (obFile == NULL) { // if file pointer didnt open
        fprintf(stderr, "Error: unable to open %s for writing.\n", objectFile);
        fclose(obFile);
        errorCode = 1;
        return;
    }
    decToBase4FourBits(ICF - 100 , buffer); // first line is the code length in base 4
    fputs(buffer, obFile); // Write code length in base 4
    fputs("\t", obFile); // tab for space.
    decToBase4FourBits(DCF , buffer); // second thing in first line is the data length in base 4
    fputs(buffer, obFile); // Write data length in base 4
    fputs("\n", obFile); // go to next line
    for (int i = 0; i < binRepIdx; i++) { // write code binary representation first.
        if (strcmp(binRep[i].lineType, "code") == 0) { // if line is code
            char *lineForStrtok = strdup(binRep[i].binaryString);
            char *token = strtok(lineForStrtok, "\n"); // seperate the binary string by lines
            int lineCount = 0; // line count for address
            while (token != NULL) { // while there are tokens to process
                decToBase4FourBits(binRep[i].address + lineCount , buffer); // turn numeral address into base 4
                fputs(buffer, obFile); // Write address in base 4
                fputs("\t", obFile); // add space
                binary10BitToBase4FiveBit(token, buffer); // Convert 10-bit binary to 5-bit base 4
                fputs(buffer, obFile); // Write binary string
                fputs("\n", obFile); // next line
                token = strtok(NULL, "\n"); // next token (line)
                lineCount++; // increment line count
            }
        }
    }
    for (int i = 0; i < binRepIdx; i++) { // write data binary representation next.
        if (strcmp(binRep[i].lineType, "data") == 0) { // if line is data
            char *lineForStrtok = strdup(binRep[i].binaryString);
            char *token = strtok(lineForStrtok, "\n"); // seperate the binary string by lines
            int lineNumber = 0; // line number for address
            while (token != NULL) {
                decToBase4FourBits(binRep[i].address + lineNumber , buffer); // turn numeral address into base 4
                fputs(buffer, obFile); // Write address in base 4
                fputs("\t", obFile); // add space
                binary10BitToBase4FiveBit(token, buffer); // Convert 10-bit binary to 5-bit base 4
                fputs(buffer, obFile); // Write binary string
                fputs("\n", obFile); // next line
                token = strtok(NULL, "\n"); // next token (line)
                lineNumber++; // increment line number
            }
        }
    }
    fclose(obFile); // finished .ob file, close it.
    if (extCount > 0) { // if there are external symbols
        char *externalFile = externalFileName(fileSrc); // get .ext file name
        FILE *extFile = fopen(externalFile, "w"); // open .ext file to write to
        if (extFile == NULL) { // if file pointer didnt open
            fprintf(stderr, "Error: unable to open %s for writing.\n", objectFile);
            fclose(extFile);
            errorCode = 1;
            return;
        }
        for (int i = 0; i < extCount; i++) { // write external symbols to .ext file
            fputs(extTable[i].label, extFile); // Write label
            fputs("\t", extFile); // add space
            decToBase4FourBits(extTable[i].adress , buffer); // Convert address to base 4
            fputs(buffer, extFile); // Write address in base 4    
            fputs("\n", extFile); // next line
        }
        fclose(extFile); // finished .ext file, close it.
    }
    if (entryCount > 0){ // if there are entry symbols
        char *entryFile = entryFileName(fileSrc); // get .ent file name
        FILE *entFile = fopen(entryFile, "w"); // open .ent file to write to
        if (entFile == NULL) { // if file pointer didnt open
            fprintf(stderr, "Error: unable to open %s for writing.\n", entryFile);
            fclose(entFile);
            errorCode = 1;
            return;
        }
        for (int i = 0; i < symbolIdx; i++) { // write entry symbols to .ent file
            if (strcmp(symbolTable[i].labelType, "entry") == 0) { // if label is entry
                fputs(symbolTable[i].label, entFile); // Write label
                fputs("\t", entFile); // add space
                decToBase4FourBits(symbolTable[i].adress, buffer);
                fputs(buffer, entFile); // Write address in base 4
                fputs("\n", entFile); // next line
            }
        }
        fclose(entFile); // finished .ent file, close it.
    }
}
/**
 * @param fileName name of file to create new name for
 * @returns:
 * new file name with .ob extension for secondPass.
 */
char* objectFileName(char *fileName) {
    char newFileName[FILENAME_MAX];
    int i = 0;
    for (;i < strlen(fileName); i++) {
        if (fileName[i] == '.') break;
        newFileName[i] = fileName[i];
    }
    newFileName[i] = '\0';
    strcat(newFileName , ".ob"); // add .ob extension
    return strdup(newFileName);
}
/**
 * @param fileName name of file to create new name for
 * @returns:
 * new file name with .ext extension for secondPass.
 */
char* externalFileName(char *fileName) {
    char newFileName[FILENAME_MAX];
    int i = 0;
    for (;i < strlen(fileName); i++) {
        if (fileName[i] == '.') break;
        newFileName[i] = fileName[i];
    }
    newFileName[i] = '\0';
    strcat(newFileName , ".ext"); // add .ext extension
    return strdup(newFileName);
}
/**
 * @param fileName name of file to create new name for
 * @returns:
 * new file name with .ent extension for secondPass.
 */
char* entryFileName(char *fileName) {
    char newFileName[FILENAME_MAX];
    int i = 0;
    for (;i < strlen(fileName); i++) {
        if (fileName[i] == '.') break;
        newFileName[i] = fileName[i];
    }
    newFileName[i] = '\0';
    strcat(newFileName , ".ent"); // add .ent extension
    return strdup(newFileName);
}
