#ifndef FIRSTPASS_H
#define FIRSTPASS_H

typedef struct Symbol {
    char *label;
    int adress;
    char *labelType;
}Symbol;

extern int symbolIdx;
extern int errorCode;
int isLabel(char* , char** , Symbol*);
int isOpcode(char*);
int isReg(char*);
int countWordsForCode(char *);
int countWordsForData(char *);
int checkDupe(Symbol* , int , char*);
Symbol** firstPass(int argc , char *argv[]);

#endif
