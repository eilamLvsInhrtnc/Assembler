/*
    *Eilam Gazit , Eyal Hets Cohen.
    *binRep.h - header file for binRep.c
*/
#ifndef BINREP_H
#define BINREP_H

#include <stdio.h>
#include "util.h"

void buildFirstWord(const Opcode* , int , int , char*);
void packImmediate(int, char*);
void packTwoRegisters(char* , char* , char*);
void packSingleRegister(char* , char*);
void extractMatrixRegs(char* , char* , char*);
int getAdressType(char*);
const Opcode* getOpcode(char*);
void appendOperand(char*, char*, int, int, char*, int);

char* dataToBinary(char* , int , int);
char* codeToBinary(char*);

#endif
