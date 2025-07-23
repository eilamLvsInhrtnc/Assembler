#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"

#define MAX_IN_LINE 80
#define COMMANDS_COUNT 16
#define MAX_LABEL_LENGTH 30

extern const char commands[16][4]; // Commands array from preAsm.h
int lineCounter = 1;

typedef struct Symbol {
    char *label;
    int adress;
    char *labelType;
}Symbol;

int errorCode = 0; // Global error code for error handling

int isLabel(char* , char** , Symbol*);
int isOpcode(char*);
int checkDupe(Symbol* , int , char*);
int isReg(char*);
int countWordsForCode(char *);
char *removeSpaces(char *);
int countWordsForData(char *);

int main(int argc , char *argv[]) {
    char **macroTbl = NULL;         // Table for macro names , for label validation
    int status = spreadMacros(argc , argv , &macroTbl);
    if (status != 1)
        exit(1);
    
    // מעבר ראשון ושני....
    // 101     106    203
    // HELLO   LOOP   STR   
    // code    code   data
    printf("Commencing first pass...\n");
    FILE *firstPass = fopen("FinalAsm.am", "r");
    int IC = 100 , DC = 0;
    int symbolIdx = 0; // index for the symbol table
    Symbol *symbolTable = NULL; // will be used to store the symbols, their addresses and their type
    char line[MAX_IN_LINE];
    while (getLineFromFile(firstPass , line , NULL , lineCounter) != NULL) {
        char *token = strtok(line , " \t");
        while (token != NULL) {
            if ((token[0] == ';') || (strcmp(token , ".entry") == 0)) { // if the line is a comment OR if there is .entry (handeled in the second pass)
                break; // skip the rest of the line , go to the next line
            }
            if (strcmp(token , ".extern")) {
                token = strtok(NULL, " \t");
                if (token == NULL) {
                    fprintf(stderr, "Error: in line %d: No label found.\n", lineCounter);
                    errorCode = 1; // set error code
                    continue; // skip to next line
                }
                symbolTable = realloc(symbolTable, (symbolIdx + 1) * sizeof(Symbol)); // reallocate memory for the symbol table
                symbolIdx++;
                symbolTable[symbolIdx].label = token;
                checkDupe(symbolTable, symbolIdx, token); // check for duplicate labels
                symbolTable[symbolIdx].adress = 0; // set the address to the current instruction counter
                symbolTable[symbolIdx].labelType = "external"; // set the label type to extern
                
            }
            int L = countWordsForCode(line); // count the number of words in the line
            if (L == 0) {
                fprintf(stderr, "Error: in line %d: No valid instruction found.\n", lineCounter);
                errorCode = 1; // set error code
                continue; // skip to next line
            }
            if (isLabel(token , macroTbl , symbolTable) == 1) {
                char *labelNoColon = strtok(token, ":"); // remove the colon from the label
                symbolTable = realloc(symbolTable, (symbolIdx + 1) * sizeof(Symbol)); // reallocate memory for the symbol table
                symbolIdx++;
                symbolTable[symbolIdx].label = labelNoColon;
                checkDupe(symbolTable, symbolIdx, labelNoColon); // check for duplicate labels
                symbolTable[symbolIdx].adress = IC; // set the address to the current instruction counter
                IC += L; // increase the instruction counter by the number of words in the line
                token = strtok(NULL, " \t"); // move to the next token after the label
                if (token[0] == '.') {
                    symbolTable[symbolIdx].labelType = "data"; // if the label is a data label
                    IC -=L; // if the label is a data label, we do not increase the instruction counter
                    symbolTable[symbolIdx].adress = DC;
                    DC += countWordsForData(line); // increase the data counter by the number of words in the line
                }
                else {
                    symbolTable[symbolIdx].labelType = "code"; // if the label is a code label
                }
                
            }
            token = strtok(NULL , " \t");
        }
        lineCounter++; // increase line counter
    }

    const int ICF = IC;
    const int DCF = DC;


    return 0; // End of first pass
}


int isLabel(char *token , char **macroTbl , Symbol *symbolTable) {
    if (strlen(token) >= MAX_LABEL_LENGTH + 1) { // MaxNameLength + 1 (for ':')
        fprintf(stderr, "Error: in line %d: Label '%s' is too long. Maximum length is %d characters.\n", lineCounter, token, MAX_LABEL_LENGTH);
        errorCode = 1; // set error code
        return 0;  // if more, not a valid label
    }
    if (token[strlen(token) - 1] != ':' || isalpha(token[0]) == 0) { // if the last char is not ':' or first letter isnt alphabet , it is not a label
        fprintf(stderr, "Error: in line %d: Label '%s' is not valid. Labels must start with a letter and end with a colon.\n", lineCounter, token);
        errorCode = 1; // set error code
        return 0; // not a valid label
    }
    Symbol *tmpPtr = symbolTable;
    char* labelNoColon = strtok(token, ":"); // remove the colon from the label
    while (tmpPtr != NULL) {
        if (strcmp(labelNoColon , tmpPtr->label) == 0) {
            fprrintf(stderr , "Error: in line %d: Label '%s' is already defined.\n",lineCounter , labelNoColon);
            errorCode = 1; // set error code
            return 0; // label already exists
        }
        tmpPtr++;
    }
    for (int i = 0; i < COMMANDS_COUNT; i++) { // check if the label is a command
        if (strcmp(labelNoColon, commands[i]) == 0) {
            fprintf(stderr, "Error: in line %d: Label '%s' cannot be a command name.\n",lineCounter, labelNoColon);
            errorCode = 1; // set error code
            return 0; // label is a command
        }
    }
    for (int i = 0; macroTbl[i] != NULL; i++) { // check if the label is a macro
        if (strcmp(token, macroTbl[i]) == 0) {
            fprintf(stderr, "Error: in line %d: Label '%s' cannot be a macro name.\n", lineCounter, token);
            errorCode = 1; // set error code
            return 0; // not a valid label
        }
    }
    return 1; // if label name didnt fail any of the checks, it is a valid label
}

int isOpcode(char *command) {
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(command, commands[i]) == 0)
            return 1;
    }
    return 0;
}

int getExpectedOperandsCount(const char *instr) {
    if (strcmp(instr, "mov") == 0 || strcmp(instr, "cmp") == 0 ||
        strcmp(instr, "add") == 0 || strcmp(instr, "sub") == 0 ||
        strcmp(instr, "lea") == 0) {
        return 2;
    }
    if (strcmp(instr, "clr") == 0 || strcmp(instr, "not") == 0 ||
        strcmp(instr, "inc") == 0 || strcmp(instr, "dec") == 0 ||
        strcmp(instr, "jmp") == 0 || strcmp(instr, "bne") == 0 ||
        strcmp(instr, "jsr") == 0 || strcmp(instr, "red") == 0 ||
        strcmp(instr, "prn") == 0) {
        return 1;
    }
    if (strcmp(instr, "rts") == 0 || strcmp(instr, "stop") == 0) {
        return 0;
    }
    return -1; // invalid instruction
}

int countWordsForCode(char *line){
    int words = 0;
    int oc = 0; // operands count
    char *instruction, *operands[10], *opcode;
    char originalLine[100];
    strcpy(originalLine, line); // copy for error messages

    line[strcspn(line,"\n")] = 0;
    instruction = strtok(line, " \t");

    if (!instruction)
        return 0;

    if (instruction[strlen(instruction)-1] == ':')
        instruction = strtok(NULL, " \t");

    if (!instruction)
        return 0;

    int expectedOperands = getExpectedOperandsCount(instruction);
    if (expectedOperands == -1) {
        fprintf(stderr, "Error: Unknown instruction '%s' in line: %s\n", instruction, originalLine);
        errorCode = 1;
        return 0;
    }

    words = 1; // base word
    opcode = strtok(NULL, "");
    if (opcode) {
        char *p = strtok(opcode, ",");
        while (p && oc < 10) {
            operands[oc++] = removeSpaces(p);
            p = strtok(NULL, ",");
        }
    }

    if (oc != expectedOperands) {
        fprintf(stderr, "Error: in line %d: Instruction '%s' expects %d operands but got %d.\n", lineCounter , instruction, expectedOperands, oc);
        errorCode = 1;
        return 0;
    }
    if (oc == 2 && isReg(operands[0]) && isReg(operands[1])) {
        words += 1;
        return words;
    }
    for (int i = 0; i < oc; i++) {
        if (strchr(operands[i], '['))
            words += 2;
        else
            words += 1;
    }

    return words;
}
// inc r1
// mov r2, r3
// STRING: jmp A#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"

#define MAX_IN_LINE 80
#define COMMANDS_COUNT 16
#define MAX_LABEL_LENGTH 30

extern const char commands[16][4]; // Commands array from preAsm.h
int lineCounter = 1;

typedef struct Symbol {
    char *label;
    int adress;
    char *labelType;
}Symbol;

int errorCode = 0; // Global error code for error handling

int isLabel(char* , char** , Symbol*);
int isOpcode(char*);
int checkDupe(Symbol* , int , char*);
int isReg(char*);
int countWordsForCode(char *);
char *removeSpaces(char *);

int main(int argc , char *argv[]) {
    char **macroTbl = NULL;         // Table for macro names , for label validation
    int status = spreadMacros(argc , argv , &macroTbl);
    if (status != 1)
        exit(1);
    
    // מעבר ראשון ושני....
    // 101     106    203
    // HELLO   LOOP   STR   
    // code    code   data
    printf("Commencing first pass...\n");
    FILE *firstPass = fopen("FinalAsm.am", "r");
    int IC = 100 , DC = 0;
    int symbolIdx = 0; // index for the symbol table
    Symbol *symbolTable = NULL; // will be used to store the symbols, their addresses and their type
    char line[MAX_IN_LINE];
    while (getLineFromFile(firstPass , line , NULL , lineCounter) != NULL) {
        char *token = strtok(line , " \t");
        while (token != NULL) {
            if ((token[0] == ';') || (strcmp(token , ".entry") == 0)) { // if the line is a comment OR if there is .entry (handeled in the second pass)
                break; // skip the rest of the line , go to the next line
            }
            if (strcmp(token , ".extern")) {
                token = strtok(NULL, " \t");
                if (token == NULL) {
                    fprintf(stderr, "Error: in line %d: No label found.\n", lineCounter);
                    errorCode = 1; // set error code
                    continue; // skip to next line
                }
                symbolTable = realloc(symbolTable, (symbolIdx + 1) * sizeof(Symbol)); // reallocate memory for the symbol table
                symbolIdx++;
                symbolTable[symbolIdx].label = token;
                checkDupe(symbolTable, symbolIdx, token); // check for duplicate labels
                symbolTable[symbolIdx].adress = 0; // set the address to the current instruction counter
                symbolTable[symbolIdx].labelType = "external"; // set the label type to extern
                
            }
            int L = countWordsForCode(line); // count the number of words in the line
            if (L == 0) {
                fprintf(stderr, "Error: in line %d: No valid instruction found.\n", lineCounter);
                errorCode = 1; // set error code
                continue; // skip to next line
            }
            if (isLabel(token , macroTbl , symbolTable) == 1) {
                char *labelNoColon = strtok(token, ":"); // remove the colon from the label
                symbolTable = realloc(symbolTable, (symbolIdx + 1) * sizeof(Symbol)); // reallocate memory for the symbol table
                symbolIdx++;
                symbolTable[symbolIdx].label = labelNoColon;
                checkDupe(symbolTable, symbolIdx, labelNoColon); // check for duplicate labels
                symbolTable[symbolIdx].adress = IC; // set the address to the current instruction counter
                IC += L; // increase the instruction counter by the number of words in the line
                token = strtok(NULL, " \t"); // move to the next token after the label
                if (token[0] == '.') {
                    symbolTable[symbolIdx].labelType = "data"; // if the label is a data label
                    IC -=L; // if the label is a data label, we do not increase the instruction counter
                    symbolTable[symbolIdx].adress = DC;
                    DC += countWordsForData(line); // increase the data counter by the number of words in the line
                }
                else {
                    symbolTable[symbolIdx].labelType = "code"; // if the label is a code label
                }
                
            }
            token = strtok(NULL , " \t");
        }
        lineCounter++; // increase line counter
    }

    const int ICF = IC;
    const int DCF = DC;


    return 0; // End of first pass
}


int isLabel(char *token , char **macroTbl , Symbol *symbolTable) {
    if (strlen(token) >= MAX_LABEL_LENGTH + 1) { // MaxNameLength + 1 (for ':')
        fprintf(stderr, "Error: in line %d: Label '%s' is too long. Maximum length is %d characters.\n", lineCounter, token, MAX_LABEL_LENGTH);
        errorCode = 1; // set error code
        return 0;  // if more, not a valid label
    }
    if (token[strlen(token) - 1] != ':' || isalpha(token[0]) == 0) { // if the last char is not ':' or first letter isnt alphabet , it is not a label
        fprintf(stderr, "Error: in line %d: Label '%s' is not valid. Labels must start with a letter and end with a colon.\n", lineCounter, token);
        errorCode = 1; // set error code
        return 0; // not a valid label
    }
    Symbol *tmpPtr = symbolTable;
    char* labelNoColon = strtok(token, ":"); // remove the colon from the label
    while (tmpPtr != NULL) {
        if (strcmp(labelNoColon , tmpPtr->label) == 0) {
            fprrintf(stderr , "Error: in line %d: Label '%s' is already defined.\n",lineCounter , labelNoColon);
            errorCode = 1; // set error code
            return 0; // label already exists
        }
        tmpPtr++;
    }
    for (int i = 0; i < COMMANDS_COUNT; i++) { // check if the label is a command
        if (strcmp(labelNoColon, commands[i]) == 0) {
            fprintf(stderr, "Error: in line %d: Label '%s' cannot be a command name.\n",lineCounter, labelNoColon);
            errorCode = 1; // set error code
            return 0; // label is a command
        }
    }
    for (int i = 0; macroTbl[i] != NULL; i++) { // check if the label is a macro
        if (strcmp(token, macroTbl[i]) == 0) {
            fprintf(stderr, "Error: in line %d: Label '%s' cannot be a macro name.\n", lineCounter, token);
            errorCode = 1; // set error code
            return 0; // not a valid label
        }
    }
    return 1; // if label name didnt fail any of the checks, it is a valid label
}

int isOpcode(char *command) {
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(command, commands[i]) == 0)
            return 1;
    }
    return 0;
}

int getExpectedOperandsCount(const char *instr) {
    if (strcmp(instr, "mov") == 0 || strcmp(instr, "cmp") == 0 ||
        strcmp(instr, "add") == 0 || strcmp(instr, "sub") == 0 ||
        strcmp(instr, "lea") == 0) {
        return 2;
    }
    if (strcmp(instr, "clr") == 0 || strcmp(instr, "not") == 0 ||
        strcmp(instr, "inc") == 0 || strcmp(instr, "dec") == 0 ||
        strcmp(instr, "jmp") == 0 || strcmp(instr, "bne") == 0 ||
        strcmp(instr, "jsr") == 0 || strcmp(instr, "red") == 0 ||
        strcmp(instr, "prn") == 0) {
        return 1;
    }
    if (strcmp(instr, "rts") == 0 || strcmp(instr, "stop") == 0) {
        return 0;
    }
    return -1; // invalid instruction
}

int countWordsForCode(char *line){
    int words = 0;
    int oc = 0; // operands count
    char *instruction, *operands[10], *opcode;
    char originalLine[100];
    strcpy(originalLine, line); // copy for error messages

    line[strcspn(line,"\n")] = 0;
    instruction = strtok(line, " \t");

    if (!instruction)
        return 0;

    if (instruction[strlen(instruction)-1] == ':')
        instruction = strtok(NULL, " \t");

    if (!instruction)
        return 0;

    int expectedOperands = getExpectedOperandsCount(instruction);
    if (expectedOperands == -1) {
        fprintf(stderr, "Error: Unknown instruction '%s' in line: %s\n", instruction, originalLine);
        errorCode = 1;
        return 0;
    }

    words = 1; // base word
    opcode = strtok(NULL, "");
    if (opcode) {
        char *p = strtok(opcode, ",");
        while (p && oc < 10) {
            operands[oc++] = removeSpaces(p);
            p = strtok(NULL, ",");
        }
    }

    if (oc != expectedOperands) {
        fprintf(stderr, "Error: in line %d: Instruction '%s' expects %d operands but got %d.\n", lineCounter , instruction, expectedOperands, oc);
        errorCode = 1;
        return 0;
    }
    if (oc == 2 && isReg(operands[0]) && isReg(operands[1])) {
        words += 1;
        return words;
    }
    for (int i = 0; i < oc; i++) {
        if (strchr(operands[i], '['))
            words += 2;
        else
            words += 1;
    }

    return words;
}
int countWordsForData(char *line){
    int dc = 0; // data count

    char originalLine[100];
    strncpy(originalLine, line, sizeof(originalLine));
    originalLine[sizeof(originalLine)-1] = '\0';
    char *pOL = trimLeading(originalLine); // pointer to original line without spaces in the start

    char *label = strchr(pOL, ':');
    if (label) pOL = trimLeading(label + 1);

    if (strncmp(pOL, ".data", 5) == 0 && (isspace(pOL[5]) || pOL[5] == '\0')) {
        char *params = trimLeading(pOL + 5);
        if (*params == ',' || params[strlen(params) - 1] == ',') {
            fprintf(stderr, "Error: comma before first or after last number in .data\n");
            return 0;
        }
        char *token = strtok(params, ",");
        while (token) {
            token = trimLeading(token);
            if (!*token) {
                fprintf(stderr, "Error: empty value between commas in .data\n");
                return 0;
            }
            char *p = (*token == '+' || *token == '-') ? token + 1 : token;
            for (; *p; p++) {
                if (!isdigit(*p)) {
                    fprintf(stderr, "Error: invalid character in .data number: '%s'\n", token);
                    return 0;
                }
            }
            dc++;
            token = strtok(NULL, ",");
        }
    } 
    else if (strncmp(pOL, ".string", 7) == 0 && (isspace(pOL[7]) || pOL[7] == '\0')) {
        char *param = trimLeading(pOL + 7);
        if (*param++ != '\"') {
            fprintf(stderr, "Error: .string must start with a quote\n");
            return 0;
        }
        while (*param && *param != '\"') {
            dc++;
            param++;
        }
        if (*param != '\"') {
            fprintf(stderr, "Error: .string missing closing quote\n");
            return 0;
        }
        dc++; // for null terminator
    } 
    else if (strncmp(pOL, ".mat", 4) == 0 && (isspace(pOL[4]) || pOL[4] == '\0')) {
        char *open = strchr(pOL, '['), *close = strchr(pOL, ']');
        if (!open || !close || close < open) {
            fprintf(stderr, "Error: .mat missing or invalid brackets\n");
            return 0;
        }
        *close = '\0';
        char *token = strtok(open + 1, ",; \t");
        while (token) {
            token = trimLeading(token);
            if (!*token) {
                fprintf(stderr, "Error: empty matrix cell in .mat\n");
                return 0;
            }
            char *p = (*token == '+' || *token == '-') ? token + 1 : token;
            for (; *p; p++) {
                if (!isdigit(*p)) {
                    fprintf(stderr, "Error: invalid matrix value: '%s'\n", token);
                    return 0;
                }
            }
            dc++;
            token = strtok(NULL, ",; \t");
        }
    } 
    else {
        fprintf(stderr, "Error: No directive found\n");
        return 0;
    }
    return dc;

}
char *trimLeading(char *str) {
    while (isspace(*str)) str++;
    return str;
}

// inc r1
// mov r2, r3
// STRING: jmp A
