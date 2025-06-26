#ifndef PREASM_C
#define PREASM_C

int spreadMacros(int , char*[]);
int isValidName(char*);
int lineContainsEndAndValid(char[]);
void copyIntoFile(int, char*[], FILE*, int*, int, char**, char**);
int* loadMacroIntoTables(char***, char***, int, char*[]);
char* getLineFromFile(FILE*, char[], char*);
char* trim(char*);

#endif 
