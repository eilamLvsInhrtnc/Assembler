#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preAsm.h"
#include "firstPass.h"
#include "util.h"

int main(int argc, char *argv[]) { // this main only executes macroSpread now.
    

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
        char **macroTbl = NULL;
        int status = spreadMacros(argvAM[fileIdx] , argv[fileIdx] , &macroTbl);
        if (status == 1) {
            fprintf(stderr, "Error: in file: %s macro proccesing failed.\n" , argv[fileIdx]);
            continue;
        }
        printf("File %s : macro processing completed.\n", argv[fileIdx]);
    }
    return 0; // remove LATER!!!!!!!!!!!!!!!
}
