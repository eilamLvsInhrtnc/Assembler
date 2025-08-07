#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

char* removeStartEndSpaces(char*);
char* getLineFromFile(FILE*, char[], char* , int*);
char* decToBinary10Bit(int);

#define MAX_IN_LINE 80
#define COMMANDS_COUNT 16
#define MAX_LABEL_LENGTH 30


extern const char commands[16][4];
extern int errorCode;
extern int symbolIdx;
extern const int ICF;
extern const int DCF;

typedef struct Symbol {
    char *label;
    int adress;
    char *labelType;
}Symbol;

typedef struct Opcode {
    char* opcode;
    char* binaryRep;
}Opcode;

extern const Opcode opcodes[16];

#endif
