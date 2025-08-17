#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"
#include "firstPass.h"
#include "util.h"
#include "binRep.h"
#include "secondPass.h"

int main(int argc, char *argv[]) { // remove ALL debug prints!!!!!!!!!!!!

    if (argc < 2) {  // No input files provided
        fprintf(stderr, "%s: No input files.\n", argv[0]);
        return 1;
    }
    char *argvAM[argc];
    argvAM[0] = argv[0]; // Keep the program name
    for (int i = 1; i < argc; i++) {
        argvAM[i] = preAsmFileName(argv[i]); // Convert each argument to .am file name
    }
    for (int fileIdx = 1; fileIdx < argc; fileIdx++) {
        errorCode = 0; // Reset error code for each file
        symbolIdx = 0;
        binRepIdx = 0;
        firstPass(argvAM[fileIdx], argv[fileIdx]); // Perform first pass
        if (errorCode != 0) {
            printf("Error in first pass for file %s.\n", argv[fileIdx]);
            continue; // Skip to next file if error occurred
        }
        if (symbolTable == NULL) {
            printf("No symbols found in file %s.\n", argv[fileIdx]);
            return 1;
        }
        for (int i = 0; i < symbolIdx; i++) {
            printf("Symbol: %s, Address: %d, Type: %s\n", symbolTable[i].label, symbolTable[i].adress, symbolTable[i].labelType);
        }
        for (int j = 0; j < binRepIdx; j++) {
            printf("%s", binRep[j].binaryString);
        }
        secondPass(argvAM[fileIdx]); // Perform second pass
        if (errorCode != 0) {
            printf("Error in second pass for file %s.\n", argv[fileIdx]);
            continue; // Skip to next file if error occurred
        }
        for (int j = 0; j < binRepIdx; j++) {
            printf("%s", binRep[j].binaryString);
        }
        printf("ICF = %d, DCF = %d\n", ICF, DCF);
        secondPass(argvAM[fileIdx]); // Load in files after second pass
        printf("Second pass completed for file %s.\n", argv[fileIdx]);
    
        // free all allocated memory.
    }
    return 0;
}
