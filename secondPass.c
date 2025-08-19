#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "secondPass.h"

/**
 * @param fileSrc The source file name
 * 
 * This function performs the second pass of the assembler, processing the
 * instructions and generating the final machine code.
 */
void secondPass(char *fileSrc) {
    printf("Commencing second pass...\n");
    FILE *secPass = fopen(fileSrc, "r");
    if (secPass == NULL) {
        fprintf(stderr, "Error: unable to open %s.\n" , fileSrc);
        return;
    }

    char line[MAX_IN_LINE];
    int lineCounter = 1;

    while(getLineFromFile(secPass , line , fileSrc , &lineCounter) != NULL) { // Read each line
        char *lineCopy = strdup(line);
        if (line[0] == '\0' || line[0] == ';') { // Skip empty lines and comments
            lineCounter++;
            continue; // Skip empty lines and comments
        }
        char *instruction = strtok(line, " \t");
        if (instruction[strlen(instruction)-1] == ':') {
            instruction = strtok(NULL, " \t"); // Get the actual instruction
        }
        if ( strcmp(instruction , ".string") == 0 || strcmp(instruction , "mat") == 0 || strcmp(instruction , ".data") == 0 ) continue; // Skip data directives
        if (strcmp(instruction , ".entry") == 0) {
            int updated = 0;
            instruction = strtok(NULL, " \t");
            for (int i = 0; i < symbolIdx; i++) {
                if (strcmp(symbolTable[i].label, instruction) == 0) {
                    symbolTable[i].labelType = "entry"; // Mark as entry
                    updated = 1;
                    break;
                }
            }
            if (updated == 0) {
                fprintf(stderr , "Error: in line %d: Entry '%s' not found in symbol table.\n", lineCounter, instruction);
                errorCode = 1;
                continue;
            }
        }
        // step number 6 at the algorithm now.
        for (int i = 0; i < binRepIdx; i++) {
            if (binRep[i].lineNumber == lineCounter) {
                // Process multi-line binaryString (split by '\n')
                char *temp = strdup(binRep[i].binaryString); // Duplicate binary string for processing
                char *token = strtok(temp, "\n");
                char newBinary[MAX_WORDS_FOR_CODE * (BITS_IN_WORD + NULL_TERMINATOR_LENGTH)]; // Buffer for new binary string
                newBinary[0] = '\0';
                int lineCount = 0;
                while (token != NULL) {
                    // If token is not pure binary → must be a label
                    if (strspn(token, "01-") != strlen(token)) {
                        int found = 0;
                        for (int j = 0; j < symbolIdx; j++) {
                            if (strcmp(symbolTable[j].label, token) == 0) {
                                found = 1;
                                char binary[BITS_IN_WORD + NULL_TERMINATOR_LENGTH];
                                if (strcmp(symbolTable[j].labelType, "external") == 0) {
                                    decToBinary8Bit(0 , binary); // extern → 0
                                    extTable = realloc(extTable, (extCount + 1) * sizeof(Symbol)); // Reallocate memory for external table
                                    extTable[extCount].label = strdup(symbolTable[j].label); // Copy label to external table
                                    extTable[extCount].adress = binRep[i].address + lineCount; // Externs have no address in the second pass
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
                        if (!found) {
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
                // Replace original binaryString with resolved version
                free(binRep[i].binaryString);
                binRep[i].binaryString = strdup(newBinary);
                free(temp);
            }
        }
        lineCounter++; // Increment line counter for error messages
    }
    fclose(secPass);
    loadInFiles(fileSrc); // Load in files after second pass
    printf("Second pass completed.\n");
}
/**
 * @param fileSrc The source file name
 *
 * The function loads the object, external and entry files generated during the second pass.
 */
void loadInFiles(char *fileSrc) {
    char buffer[6];
    char *objectFile = objectFileName(fileSrc); // Get object file name
    FILE *obFile = fopen(objectFile, "w"); // Open object file for writing
    if (obFile == NULL) {
        fprintf(stderr, "Error: unable to open %s for writing.\n", objectFile);
        fclose(obFile);
        errorCode = 1;
        return;
    }
    decToBase4FourBits(ICF - 100 , buffer);
    fputs(buffer, obFile); // Write code length in base 4
    fputs("\t", obFile);
    decToBase4FourBits(DCF , buffer);
    fputs(buffer, obFile); // Write data length in base 4
    fputs("\n", obFile);
    for (int i = 0; i < binRepIdx; i++) {
        if (strcmp(binRep[i].lineType, "code") == 0) {
            char *lineForStrtok = strdup(binRep[i].binaryString); 
            char *token = strtok(lineForStrtok, "\n");
            int lineCount = 0;
            while (token != NULL) {
                decToBase4FourBits(binRep[i].address + lineCount , buffer);
                fputs(buffer, obFile); // Write address in base 4
                fputs("\t", obFile);
                binary10BitToBase4FiveBit(token, buffer); // Convert 10-bit binary to 5-bit base 4
                fputs(buffer, obFile); // Write binary string
                fputs("\n", obFile);
                token = strtok(NULL, "\n");
                lineCount++;
            }
        }
    }
    for (int i = 0; i < binRepIdx; i++) {
        if (strcmp(binRep[i].lineType, "data") == 0) {
            char *lineForStrtok = strdup(binRep[i].binaryString);
            char *token = strtok(lineForStrtok, "\n");
            int lineNumber = 0;
            while (token != NULL) {
                decToBase4FourBits(binRep[i].address + lineNumber , buffer);
                fputs(buffer, obFile); // Write address in base 4
                fputs("\t", obFile);
                binary10BitToBase4FiveBit(token, buffer); // Convert 10-bit binary to 5-bit base 4
                fputs(buffer, obFile); // Write binary string
                fputs("\n", obFile);
                token = strtok(NULL, "\n");
                lineNumber++;
            }
        }
    }
    fclose(obFile);
    if (extCount > 0) {
        char *externalFile = externalFileName(fileSrc); // Get external file name
        FILE *extFile = fopen(externalFile, "w"); // Open external file for writing
        if (extFile == NULL) {
            fprintf(stderr, "Error: unable to open %s for writing.\n", objectFile);
            fclose(extFile);
            errorCode = 1;
            return;
        }
        for (int i = 0; i < extCount; i++) {
            fputs(extTable[i].label, extFile); // Write label
            fputs("\t", extFile);
            decToBase4FourBits(extTable[i].adress , buffer);
            fputs(buffer, extFile); // Write address in base 4    
            fputs("\n", extFile);
        }
        fclose(extFile);
    }
    if (entryCount > 0){
        char *entryFile = entryFileName(fileSrc); // Get entry file name
        FILE *entFile = fopen(entryFile, "w"); // Open entry file for writing
        if (entFile == NULL) {
            fprintf(stderr, "Error: unable to open %s for writing.\n", entryFile);
            fclose(entFile);
            errorCode = 1;
            return;
        }
        for (int i = 0; i < symbolIdx; i++) {
            if (strcmp(symbolTable[i].labelType, "entry") == 0) {
                fputs(symbolTable[i].label, entFile); // Write label
                fputs("\t", entFile);
                decToBase4FourBits(symbolTable[i].adress, buffer);
                fputs(buffer, entFile); // Write address in base 4
                fputs("\n", entFile);
            }
        }
        fclose(entFile);
    }
}
/**
 * @param fileName The source file name
 * 
 * @returns a new file name with .ob extension for the object file
 */
char* objectFileName(char *fileName) {
    char newFileName[FILENAME_MAX];
    int i = 0;
    for (;i < strlen(fileName); i++) {
        if (fileName[i] == '.') break; // Find the first dot
        newFileName[i] = fileName[i]; // Copy characters until dot
    }
    newFileName[i] = '\0'; // Null terminate the string
    strcat(newFileName , ".ob"); // Append .ob extension
    return strdup(newFileName);
}
/**
 * @param fileName The source file name
 *
 * @returns a new file name with .ext extension for the external file
 */
char* externalFileName(char *fileName) {
    char newFileName[FILENAME_MAX];
    int i = 0;
    for (;i < strlen(fileName); i++) {
        if (fileName[i] == '.') break; // Find the first dot
        newFileName[i] = fileName[i]; // Copy characters until dot
    }
    newFileName[i] = '\0'; // Null terminate the string
    strcat(newFileName , ".ext"); // Append .ext extension
    return strdup(newFileName);
}
/**
 * @param fileName The source file name
 *
 * @returns a new file name with .ent extension for the entry file
 */
char* entryFileName(char *fileName) {
    char newFileName[FILENAME_MAX];
    int i = 0;
    for (;i < strlen(fileName); i++) {
        if (fileName[i] == '.') break; // Find the first dot
        newFileName[i] = fileName[i]; // Copy characters until dot
    }
    newFileName[i] = '\0'; // Null terminate the string
    strcat(newFileName , ".ent"); // Append .ent extension
    return strdup(newFileName);
}
