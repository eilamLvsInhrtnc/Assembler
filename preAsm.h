/*
    *Eilam Gazit , Eyal Hets Cohen.
    *preAsm.h - header file for preAsm.c
*/
#ifndef PREASM_H
#define PREASM_H

#include <stdio.h>

int spreadMacros(char* , char*, char***);
int isValidName(char*);
int lineContainsEndAndValid(char[]);
void copyIntoFile(FILE* , FILE* , int , char** , char** , char*);
int loadMacroIntoTables(char***, char*** , FILE* , char* );
char* preAsmFileName(char*);

#endif
