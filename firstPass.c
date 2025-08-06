#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"
#include "firstPass.h"

int main(int argc, char *argv[]) {
    printf("Assembler Program Starting...\n");
    Symbol *symbolTable = NULL; // Initialize symbol table pointer
    symbolTable = firstPass(argc, argv);
    printf("First pass completed. Symbol table:\n");
    if (errorCode == 1) {
        exit(1);
    }
    if (symbolTable == NULL) {
        fprintf(stderr, "Error during first pass.\n");
        return 1; // Exit with error
    }
    for (int i = 0; i < symbolIdx; i++) {
        printf("Label: %s, Address: %d, Type: %s\n", symbolTable[i].label, symbolTable[i].adress, symbolTable[i].labelType);
    }

    // Here you would typically proceed to the second pass or further processing
    // For now, we just free the symbol table and exit
    free(symbolTable);
    
    printf("First pass completed successfully.\n");
    return 0; // Exit with success
}
