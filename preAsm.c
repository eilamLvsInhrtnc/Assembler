#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#define MAX_IN_LINE 80
#define COMMANDS_COUNT 16

const char commands[16][4] = { "mov" , "cmp" , "add" , "sub" , "not" , "clr" ,"lea" , "inc" , "dec" , "jmp" , "bne", "red" ,"prn" ,"jsr" , "rts" , "stop"};

void loadLineFromFile(FILE* , char[] , char*);
int isValidName(char*);
int lineContainsEnd(char[]);

void main(int argc , char *argv[]) {
    char** macroTbl; // list of macro names.
    char** macroContent; // list of macro code with the same index as the macro names.
    int fileIdx = 0 , macrIdx = 0;
    if (argc == 1) { // no input files.
        fprintf(stderr , "%s: No input files.\n" , argv[0]);
        exit(1);
    }
    FILE *finalAsm = fopen("C:\\Project\\AssemblerProject\\FinalAsm.txt" , "w"); // final file.
    for (; fileIdx < argc; fileIdx++) { // go over every file given.


        FILE *inputFile = fopen(argv[fileIdx] , "r"); // open current file


        char line[MAX_IN_LINE + 1]; // line buffer , 80 characters + \0
        int expectingMcroend = 0;
        while (getLineFromFile(inputFile , line , argv[fileIdx]) != NULL) { // get lines until line is null - end of file.
            if (expectingMcroend == 1) { // if expecting mcroend , the line is inside the macro, now copying it into the macroContent array.
                int status;
                while (( status = lineContainsEndAndValid(line))== 0) {
                    realloc(macroContent , macrIdx*sizeof(char*));
                    strcat(macroContent[macrIdx] , line);
                }
                if (status == 2) 
                    fprintf(stderr , "Error in file %s , extranous text after 'mcro' statement.\n" , argv[fileIdx]);
            }
            char* token = strtok(line , " \t"); // strtok to seperate the lines into tokens using spaces and tabs.
            while (token != NULL) { // until the line is finished
                if (strcmp(token , "mcro") == 0){ // macro detected

                    token = strtok(NULL , " \t");
                    if (isValidName(token))
                    realloc(macroTbl , (macrIdx + 1)*sizeof(char*));
                    macrIdx++;
                    strcpy(macroTbl[macrIdx - 1] , token);
                    token = strtok(NULL , " \t");
                    if (token != NULL) 
                        fprintf(stderr , "Error in file %s , extranous text after 'mcro' statement.\n" , argv[fileIdx]);
                    
                    expectingMcroend = 1;
                }
                token = strtok(NULL , " \t");
            }
        }
        
    }
    
}

char* getLineFromFile(FILE *fp , char line[] , char* fileName) {
    char ch;
    int lineIdx = 0;
    while ((ch = getc(fp)) != EOF && ch != '\n') {
        if (lineIdx == MAX_IN_LINE) // we want to allow 80 characters, no more.
            fprintf(stderr , "Error in file %s , More than 80 character in a line.\n" , fileName);

        line[lineIdx] = ch;
        lineIdx++;
    }
    line[lineIdx] = '\0'; // Null-terminate the string
    return line;
}

int isValidName (char* name) {
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(name , commands[i]) == 0)
            return 0;
    }
    return 1;
}
/**
 * @param line , line from input.
 * returns:
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
