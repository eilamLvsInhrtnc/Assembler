#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

#define MAX_IN_LINE 80
#define COMMANDS_COUNT 16
#define MAX_LABEL_LENGTH 30
#define TOTAL_WORDS 156 
#define MAX_WORDS_FOR_CODE 5
#define BITS_IN_WORD 10
#define NULL_TERMINATOR_LENGTH 1

char* removeStartEndSpaces(char*);
char* getLineFromFile(FILE*, char[], char* , int*);
void intToBinary10Bit(int , char*);
void decToBinary8Bit(int , char*);
void binary10BitToBase4FiveBit(char* , char*);
void decToBase4FourBits(int , char*);

extern const char commands[16][4];
extern int errorCode;
extern int symbolIdx;
extern int binRepIdx;
extern int ICF;
extern int DCF;
extern int extCount;
extern int entryCount;

typedef struct Symbol {
    char *label;
    int adress;
    char *labelType;
}Symbol;

typedef struct Opcode {
    char *opcode;
    char *binaryRep;
    int expectedOperands; // Number of expected operands for the opcode
    char *operandN1legalAddressingModes;
    char *operandN2legalAddressingModes;
}Opcode;

typedef struct BinRep {
    char *binaryString;
    char *lineType;
    int lineNumber; // Line number in the source file
    int address; // address
}BinRep;

extern const Opcode opcodes[16];
extern Symbol *symbolTable;
extern Symbol *extTable;
extern BinRep *binRep;

#endif
