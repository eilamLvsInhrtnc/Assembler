#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "preAsm.h"
#include "util.h"

/**
 * @param argc number of command line arguments
 * @param argv array of command line arguments
 * 
 * Main function of the assembler program.
 * Handles command-line arguments, initializes macro processing,
 * and coordinates the assembly process.
 * Returns 1 on error, 0 on successful execution.
 */
int spreadMacros(char* fileDest , char* fileSrc , char*** macroTblPtr) {
    printf("File %s : starting macro processing.\n" , fileSrc);
    char** macroTbl = NULL;         // Table for macro names
    char** macroContent = NULL;     // Table for macro content
    FILE* dest = fopen(fileDest, "w");
    if (dest == NULL) { // if file pointer is null ,
        fprintf(stderr, "Error: File %s couldn't be opened\n", fileDest); // send error message
        return 1; // error code.
    }
    FILE* src = fopen(fileSrc, "r");
    if (src == NULL) { // if file pointer is null ,
        fprintf(stderr, "Error: File %s couldn't be opened\n", fileSrc); // send error message
        return 1; // error code.
    }
    
    int macrIdx = loadMacroIntoTables(&macroTbl, &macroContent, src , fileSrc); // load macros from input files into tables
    fclose(src); // close input file after reading macros
    src = fopen(fileSrc, "r");
    if (src == NULL) { // if file pointer is null ,
        fprintf(stderr, "Error: File %s couldn't be opened\n", fileSrc); // send error message
        return 1; // error code.
    }
    copyIntoFile(dest , src , macrIdx, macroTbl, macroContent , fileSrc); // copy files to output with macro spread

    if (macroTbl != NULL) { // free not needed memory
        for (int i = 0; i < macrIdx; i++) free(macroTbl[i]);
        free(macroTbl);
    }
    if (macroContent != NULL) { // free not needed memory
        for (int i = 0; i < macrIdx; i++) free(macroContent[i]);
        free(macroContent);
    }
    fclose(dest); // close final file, will be opened again in different methods
    *macroTblPtr = macroTbl; // return the macro table pointer to the caller
    if (errorCode == 1) { // if there was an error in the macro processing
        return 1; // return 1 = error status.
    }
    return 0; // return 0 = status of success
}

/**
 * @param macroTblPtr pointer to macro names table
 * @param macroContentPtr pointer to macro content table
 * @param argc number of command line arguments
 * @param argv array of command line arguments
 * 
 * @returns:
 * array indicating which files should be copied (index 0 contains macro count)
 * 
 * Processes input files to identify macro definitions.
 * Populates macro tables with names and content.
 * Skips files with errors in macro definitions.
 */
int loadMacroIntoTables(char*** macroTblPtr, char*** macroContentPtr, FILE* src , char* fileSrc) {

    char** macroTbl = NULL;        // "2d array" = string array to keep macro names
    char** macroContent = NULL;    // string array to store macro content (body of macro)
    int macrIdx = 0;               // macro index , index for tables.

    if (src == NULL) { // if file pointer is null ,
        fprintf(stderr, "Error: File %s couldn't be opened\n", fileSrc); // send error message
        errorCode = 1;
        return 0; // skip this file.
    }

    char line[MAX_IN_LINE + 2];   // MAX_IN_LINE + 2 = 80 + '\n' + '\0'
    int skipCurrent = 0;          // 'boolean' to skip current file on error
    int inMacro = 0;              // is in macro?
    int lineCounter = 1;          // line counter for error messages

    while (getLineFromFile(src, line, fileSrc , &lineCounter) != NULL && !skipCurrent) { // go over the file , line by line.
        char cleanLine[MAX_IN_LINE + 2];  // trimmed string buffer
        strcpy(cleanLine, line);
        char* trimmed = removeStartEndSpaces(cleanLine);  // trim string
            
        if (inMacro == 1) {
            int status = lineContainsEndAndValid(trimmed);
            if (status == 1) {  // Valid mcroend found
                inMacro = 0;
                macrIdx++;
            } 
            else if (status == 2) {  // Extraneous text after mcroend
                fprintf(stderr, "Error: in file: %s line %d: Extraneous text after 'mcroend'\n", fileSrc , lineCounter);
                skipCurrent = 1;
                errorCode = 1;
            } 
            else {  // Regular line inside macro , because status is 0
                if (macroContent[macrIdx] == NULL) { // if place is null, initialize it and put the string in it
                    macroContent[macrIdx] = malloc(strlen(line) + 2); // initialize and allocate
                    sprintf(macroContent[macrIdx], "%s\n", line); // put the string in it
                }
                else { // if there is more content in macro
                    char* newContent = malloc(strlen(macroContent[macrIdx]) + strlen(line) + 2); // add memory
                    sprintf(newContent, "%s%s\n", macroContent[macrIdx], line); // add the more content
                    free(macroContent[macrIdx]); // free prev pointer to old content
                    macroContent[macrIdx] = newContent; // store the new pointer into old one.
                }
            }
        } 
        else { // if not in a macro.
            char* token = strtok(trimmed, " \t"); // seperate into tokens with ' ' or '\t'
            if (token && strcmp(token, "mcro") == 0) { // if macro defenition starts
                token = strtok(NULL, " \t"); // forward to see name of macro.
                if (token == NULL || !isValidName(token)) { // if there is no name or invalid name.
                    fprintf(stderr, "Error: in file %s: line %d: Invalid macro name\n", fileSrc , lineCounter);
                    skipCurrent = 1; // skip corrent file.
                    errorCode = 1;
                    continue; // skip corrent iteration.
                }                
                if (strtok(NULL, " \t") != NULL) { // if more text after macro name.
                    fprintf(stderr, "Error: in file %s: line %d: Extraneous text after macro name\n", fileSrc, lineCounter);
                    skipCurrent = 1; // skip corrent file.
                    errorCode = 1;
                    continue; // skip corrent iteration.
                } 
                macroTbl = realloc(macroTbl, (macrIdx + 1) * sizeof(char*)); // Allocate space for new macro
                macroContent = realloc(macroContent, (macrIdx + 1) * sizeof(char*)); // Allocate space for new macro
                macroTbl[macrIdx] = strdup(token);  // Store macro name
                macroContent[macrIdx] = NULL;       // Initialize content
                inMacro = 1;  // put inMacro mode
            }
        }
        lineCounter++; // Increment line counter for error messages
    }
    fclose(src); // close current file.
    *macroTblPtr = macroTbl; // return the filled table
    *macroContentPtr = macroContent; // return the filled table
    return macrIdx;
}

/**
 * @param argc number of command line arguments
 * @param argv array of command line arguments
 * @param outputFile destination file for final assembly
 * @param filesToCopy array indicating which files to process
 * @param macrIdx number of macros defined
 * @param macroTbl table of macro names
 * @param macroContent table of macro content
 * 
 * Copies input files to output with macro expansion.
 * Skips macro definitions and replaces macro calls with their content.
 */
void copyIntoFile(FILE* dest , FILE* src, int macrIdx, char** macroTbl, char** macroContent , char* fileSrc) {
    char line[MAX_IN_LINE + 2];  // line buffer
    int inMacro = 0;           // is inside macro ? boolean
    int lineCounter = 1; // line counter for error messages
    while (getLineFromFile(src, line, fileSrc , &lineCounter) != NULL) { // go over the file , line by line.
        char cleanLine[MAX_IN_LINE + 2];  // trimmed string buffer
        strcpy(cleanLine, line);
        char* trimmed = removeStartEndSpaces(cleanLine);  // trim string
            
        if (inMacro == 1) {
            if (strcmp(trimmed, "mcroend") == 0) {
                inMacro = 0;  // end of macro
            }
        } 
        else {
            char* token = strtok(trimmed, " \t"); // process lines outside macro
            if (token && strcmp(token, "mcro") == 0) {
                inMacro = 1;  // start of macro definition
            } 
            else {
                int isMacroCall = 0; // Check if line is a macro call , using 'boolean'
                for (int j = 0; j < macrIdx; j++) {
                    if (strcmp(trimmed, macroTbl[j]) == 0) {
                        fputs(macroContent[j], dest); // spread macro by putting its content
                        isMacroCall = 1;
                        break;
                    }
                }
                if (isMacroCall == 0) {
                    fputs(line, dest); // Copy regular line to output
                    fputc('\n', dest);  // Add missing newline
                }
            }
        }
        lineCounter++; // Increment line counter for error messages
    }
}

/**
 * @param name name of macro to check if valid
 * 
 * @returns:
 * 0 if the name is invalid
 * 1 if the name is valid
 */
int isValidName(char* name) {
    if (!name) return 0;  // Null pointer check
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(name, commands[i]) == 0) return 0;
    }
    return 1;
}

/**
 * @param line line from input.
 * 
 * @returns:
 * 0 if no mcroend in line at all
 * 1 if there is only mcroend
 * 2 if there is mcroend and more (invalid.)
 */
int lineContainsEndAndValid(char line[]) {
    char* token = strtok(line, " \t");
    while (token) {
        if (strcmp(token, "mcroend") == 0) {
            token = strtok(NULL, " \t");
            return (token == NULL) ? 1 : 2;  // Return 2 if extra text, 1 if clean
        }
        token = strtok(NULL, " \t");
    }
    return 0;  // No mcroend found
}
/**
 * @param fileName name of file to create new name for
 * @returns:
 * new file name with .am extension for preAsm.
 */
char* preAsmFileName(char *fileName) {
    char newFileName[FILENAME_MAX];
    int i = 0;
    for (;i < strlen(fileName); i++) {
        if (fileName[i] == '.') break;
        newFileName[i] = fileName[i];
    }
    newFileName[i] = '\0';
    strcat(newFileName , ".am");
    return strdup(newFileName);
}
