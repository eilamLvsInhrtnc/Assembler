#ifndef FIRSTPASS_H
#define FIRSTPASS_H

typedef struct Symbol {
    char *label;
    int adress;
    char *labelType;
}Symbol;

extern int symbolIdx;
extern int errorCode;
extern const int ICF;
extern const int DCF;
int isValidLabel(char* , char** , Symbol* , int);
int isReg(char*);
int countWordsForCode(char * , int);
int countWordsForData(char * , int);
Symbol* firstPass(int argc , char *argv[]);
int getExpectedOperandsCount(char *);
char *getBinaryRep(char *line);

#endif
