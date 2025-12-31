/*
    *Eilam Gazit , Eyal Hets Cohen.
    *assembler.c - the main file for the assembler project.
    *this file supervises the assembler process,
    *including the first and second passes, and handles command line arguments. 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"
#include "firstPass.h"
#include "util.h"
#include "binRep.h"
#include "secondPass.h"

int main(int argc, char *argv[]) {

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
        secondPass(argvAM[fileIdx]); // Perform second pass
        if (errorCode != 0) {
            printf("Error in second pass for file %s.\n", argv[fileIdx]);
            continue; // Skip to next file if error occurred
        }
        printf("Second pass completed for file %s.\n", argv[fileIdx]);
    }
    return 0;
}
