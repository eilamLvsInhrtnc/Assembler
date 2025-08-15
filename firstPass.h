#ifndef FIRSTPASS_H
#define FIRSTPASS_H

#include <stdio.h>
#include "util.h"

int isValidLabel(char* , char** , Symbol* , int);
int isReg(char*);
int countWordsForCode(char * , int);
int countWordsForData(char * , int);
void firstPass(char * , char *);
int getExpectedOperandsCount(char *);
int getAdressType(char *);


#endif
