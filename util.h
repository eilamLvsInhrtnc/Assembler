/*
    *Eilam Gazit , Eyal Hets Cohen.
    *util.h - header file for util.c
*/
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
#define NEWLINE_LENGTH 1

char* removeStartEndSpaces(char*);
char* getLineFromFile(FILE*, char[], char* , int*);
void intToBinary10Bit(int , char*);
void decToBinary8Bit(int , char*);
void binary10BitToBase4FiveBit(char* , char*);
void decToBase4FourBits(int , char*);

extern const char commands[16][4]; /// global array of commands
extern int errorCode; // global error code
extern int symbolIdx; // global symbolTable index
extern int binRepIdx; // global binRep index
extern int ICF; // global final instruction counter
extern int DCF; // global final data counter
extern int extCount; // global external symbols count
extern int entryCount; // global entry symbols count

typedef struct Symbol { // symbol struct to hold labels.
    char *label; // label name
    int adress; // address of the label
    char *labelType; // type of the label (code , data , entry , external)
}Symbol;

typedef struct Opcode { // opcode struct to hold opcode information.
    char *opcode; // Opcode name
    char *binaryRep; // 4-bit binary representation of the opcode
    int expectedOperands; // Number of expected operands for the opcode
    char *operandN1legalAddressingModes; // Legal addressing modes for operand 1
    char *operandN2legalAddressingModes; // Legal addressing modes for operand 2
}Opcode;

typedef struct BinRep {
    char *binaryString; // Binary representation of the instruction
    char *lineType; // Type of the line (code , data)
    int lineNumber; // Line number in the source file
    int address; // address of the line (current IC or current DC)
}BinRep;

extern const Opcode opcodes[16]; // global map of opcodes
extern Symbol *symbolTable; // global symbol table
extern Symbol *extTable; // global external symbols table
extern BinRep *binRep; // global binary representation table

#endif/*
    *Eilam Gazit , Eyal Hets Cohen.
    *util.h - header file for util.c
*/
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
#define NEWLINE_LENGTH 1

char* removeStartEndSpaces(char*);
char* getLineFromFile(FILE*, char[], char* , int*);
void intToBinary10Bit(int , char*);
void decToBinary8Bit(int , char*);
void binary10BitToBase4FiveBit(char* , char*);
void decToBase4FourBits(int , char*);

extern const char commands[16][4]; /// global array of commands
extern int errorCode; // global error code
extern int symbolIdx; // global symbolTable index
extern int binRepIdx; // global binRep index
extern int ICF; // global final instruction counter
extern int DCF; // global final data counter
extern int extCount; // global external symbols count
extern int entryCount; // global entry symbols count

typedef struct Symbol { // symbol struct to hold labels.
    char *label; // label name
    int adress; // address of the label
    char *labelType; // type of the label (code , data , entry , external)
}Symbol;

typedef struct Opcode { // opcode struct to hold opcode information.
    char *opcode; // Opcode name
    char *binaryRep; // 4-bit binary representation of the opcode
    int expectedOperands; // Number of expected operands for the opcode
    char *operandN1legalAddressingModes; // Legal addressing modes for operand 1
    char *operandN2legalAddressingModes; // Legal addressing modes for operand 2
}Opcode;

typedef struct BinRep {
    char *binaryString; // Binary representation of the instruction
    char *lineType; // Type of the line (code , data)
    int lineNumber; // Line number in the source file
    int address; // address of the line (current IC or current DC)
}BinRep;

extern const Opcode opcodes[16]; // global map of opcodes
extern Symbol *symbolTable; // global symbol table
extern Symbol *extTable; // global external symbols table
extern BinRep *binRep; // global binary representation table

#endif
