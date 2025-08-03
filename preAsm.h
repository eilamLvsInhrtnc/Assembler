#ifndef PREASM_H
#define PREASM_H

int spreadMacros(int , char*[] , char***);
int isValidName(char*);
int lineContainsEndAndValid(char[]);
void copyIntoFile(int, char*[], FILE*, int[], int, char**, char**);
int* loadMacroIntoTables(char***, char***, int, char*[]);
char* getLineFromFile(FILE*, char[], char* , int);
char* removeStartEndSpaces(char*);
extern const char commands[16][4];

#endif
