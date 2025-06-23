#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#define MAX_IN_LINE 80
#define COMMANDS_COUNT 16

const char commands[16][4] = { "mov" , "cmp" , "add" , "sub" , "not" , "clr" ,"lea" , "inc" , "dec" , "jmp" , "bne", "red" ,"prn" ,"jsr" , "rts" , "stop"};

void loadLineFromFile(FILE* , char[] , char* , int*);
int isValidName(char*);
int lineContainsEnd(char[]);


void main(int argc , char *argv[]) {
    char** macroTbl;
    char** macroContent;
    if (loadMacroIntoTAbles(&macroTbl , &macroContent , argc , argv) != 0)
        exit(1);
    
}

int loadMacroIntoTables(char*** macroTblPtr , char*** macroContentPtr , int argc , char *argv[]) {
    int errStatus = 0;
    char** macroTbl = *(macroTblPtr); // list of macro names to initialize
    char** macroContent = *(macroContentPtr); // list of macro code with the same index as the macro names , to initialize.
    int fileIdx = 1 , macrIdx = 0;
    if (argc == 1) { // no input files.
        fprintf(stderr , "%s: No input files.\n" , argv[0]);
        errStatus = 1;
    }
    for (; fileIdx < argc; fileIdx++) { // go over every file given.


        FILE *inputFile = fopen(argv[fileIdx] , "r"); // open current file
        if(inputFile == NULL){
            fprintf(stderr, "%s: File %s couldn't be opened", argv[0], argv[fileIdx]);
            errStatus = 1;
            continue;
        }


        char line[MAX_IN_LINE + 1]; // line buffer , 80 characters + \0
        int expectingMcroend = 0;
        while (getLineFromFile(inputFile , line , argv[fileIdx] , &errStatus) != NULL) { // get lines until line is null - end of file.
            if (expectingMcroend == 1) { // if expecting mcroend , the line is inside the macro, now copying it into the macroContent array.
                int status;
                while (( status = lineContainsEndAndValid(line))== 0) {
                    realloc(macroContent , macrIdx*sizeof(char*));
                    strcat(macroContent[macrIdx] , line);
                }
                if (status == 2) {
                    fprintf(stderr , "Error in file %s , extranous text after 'mcro' statement.\n" , argv[fileIdx]);
                    errStatus = 1;
                }
            }
            char* token = strtok(line , " \t"); // strtok to seperate the lines into tokens using spaces and tabs.
            while (token != NULL) { // until the line is finished
                if (strcmp(token , "mcro") == 0){ // macro detected

                    token = strtok(NULL , " \t");
                    if (isValidName(token))
                    realloc(macroTbl , (macrIdx + 1)*sizeof(char*));
                    macrIdx++;
                    strcpy(macroTbl[macrIdx - 1] , token);
                    if (token != NULL) {
                        fprintf(stderr , "Error in file %s , extranous text after 'mcro' statement.\n" , argv[fileIdx]);
                        errStatus = 1;
                    }
                    expectingMcroend = 1;
                }
                token = strtok(NULL , " \t");
            }
        }
        
    }
    return errStatus;
}
/**
 * @param fp input file
 * @param line buffer for line
 * @param fileName only for error message
 * @param errStatusPtr error usage
 * 
 * @returns:
 * line from file input
 * 
 */
char* getLineFromFile(FILE *fp , char line[] , char* fileName , int* errStatusPtr) {
    char ch;
    int lineIdx = 0;
    while ((ch = getc(fp)) != EOF && ch != '\n') {
        if (lineIdx == MAX_IN_LINE) { // we want to allow 80 characters, no more.
            fprintf(stderr , "Error in file %s , More than 80 character in a line.\n" , fileName);
            *errStatusPtr = 1;
        }

        line[lineIdx] = ch;
        lineIdx++;
    }
    line[lineIdx] = '\0'; // Null-terminate the string
    return line;
}

/**
 * @param name array of command names (invalid names from macro).
 * 
 * @returns:
 * 0 if the name is invalid
 * 1 if the name is valid
 */
int isValidName (char* name) {
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(name , commands[i]) == 0) // compare the strings to see if they match
            return 0;
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
    char* token = strtok(line , " \t");
    while (token != NULL) {
        if (strcmp(token , "mcroend") == 0) {
            token = strtok(NULL , " \t");
            return (token == NULL) ? 1 : 2;
        }
        token = strtok(NULL , " \t");
    }
    return 0;
}
/**
 * @param argc number of command line arguments (files).
 * @param argv array of the command line arguments.
 * @param fp the file which all off the command line arguments will be copied into.
 * 
 * copies all the files from the input into one file.
 * if there are no files then the final file will be empty.
 */
void copyIntoFile(int argc, char *argv[], FILE *fp){
    for(int i = 1; i < argc; i++){ // iterate until all of the command line arguments have been read.
        FILE *input = fopen(argv[i], "r");  // open file.
        if(input == NULL){ 
            fprintf(stderr, "%s: File %s couldn't be opened", argv[0], argv[i]);
            continue; // continue the next iteration because the current file is null.
        }
        char ch;
        while((ch = getc(input)) != EOF){ // iterate until we reach the end of the file
            putc(ch, fp); // copy the characters of the current file into fp.
        }
        fclose(input);
    }
}
