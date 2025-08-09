char* dataToBinary(char *line) {
    char *binaryString = malloc((BINARY_WIDTH * (MAX_IN_LINE) * sizeof(char))+1); // Adjust size as needed
    if (!binaryString) return NULL;

    char *lineCopy = strdup(line);
    if (!lineCopy) {
        free(binaryString);
        return NULL;
    }

    lineCopy[strcspn(lineCopy, "\n")] = 0;  // Remove newline char

    char *dataOrlabel = strtok(lineCopy, " \t");

    if (!dataOrlabel) {
        free(lineCopy);
        free(binaryString);
        return NULL;
    }

    int isData = strcmp(dataOrlabel, ".data") == 0;
    int isString = strcmp(dataOrlabel, ".string") == 0;
    int isMat = strcmp(dataOrlabel, ".mat") == 0;

    if (!isData && !isString && !isMat) {
        dataOrlabel = strtok(NULL, " \t");
        if (!dataOrlabel) {
            free(lineCopy);
            free(binaryString);
            return NULL;
        }
        isData = strcmp(dataOrlabel, ".data") == 0;
        isString = strcmp(dataOrlabel, ".string") == 0;
        isMat = strcmp(dataOrlabel, ".mat") == 0;
    }

    if (isData) {
        char *currentNumber = strtok(NULL, ",");
        char *ptrBinaryString = binaryString;
        char *lineCopyData = strdup(lineCopy);

        while (currentNumber != NULL) {
            char *value = removeStartEndSpaces(currentNumber);
            int number = atoi(value);
            char *binaryNumber = decToBinary10Bit(number);

            for (int i = 0; i < 10; i++) {
                *ptrBinaryString++ = binaryNumber[i];
            }
            *ptrBinaryString++ = '\n';

            free(binaryNumber);
            currentNumber = strtok(NULL, ",");
        }
        *ptrBinaryString = '\0';
    } 
    else if(isString){
        printf("String detected\n");
        char *ptrBinaryString = binaryString;
        char *lineCopyString = strdup(lineCopy);
        char *stringStart = lineCopyString;

        while (*stringStart && *stringStart != '"') {
            stringStart++; // Move to the first quote
        }
        if (stringStart != NULL) {
            stringStart+=2; // Move past the opening quote
            char *stringEnd = strchr(stringStart, '"'); // Find closing quote
            if (stringEnd) {
                *stringEnd = '\0'; // Terminate string at closing quote
                for (char *pointer = stringStart; *pointer != '\0'; pointer++) {
                
                    int asciiValue = (int)(*pointer);
                    char *binaryChar = decToBinary10Bit(asciiValue);
                    for (int i = 0; i < 10; i++) {
                        *ptrBinaryString++ = binaryChar[i];
                    }
                    *ptrBinaryString++ = '\n';
                    free(binaryChar);
                }

                // Add binary for null terminator character
                char *nullBinary = decToBinary10Bit(0);
                for (int i = 0; i < 10; i++) {
                    *ptrBinaryString++ = nullBinary[i];
                }
                *ptrBinaryString++ = '\n';
                free(nullBinary);

                *ptrBinaryString = '\0'; // Null terminate the full string
            }
            else {
                printf("No closing quote found!\n");
                return NULL;
            }
        }
        else {
            printf("No opening quote found!\n");
            return NULL;
        }
    }
    else if (isMat) { // Handle matrix directive
        printf("Matrix detected\n");

        char *lineCopyMat = strdup(line);
        if (!lineCopyMat) {
            free(binaryString);
            return NULL;
        }

        // Skip the ".mat" part to start parsing from after it
        char *afterMat = strstr(lineCopyMat, ".mat");
        if (!afterMat) {
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }
        afterMat += 4; // move pointer past ".mat"

        char *ptrBinaryString = binaryString;
        int rows = 0, cols = 0;

        // Find rows count
        char *openBracketRows = strchr(afterMat, '[');
        if (!openBracketRows) {
            printf("Error: Missing opening bracket for rows\n");
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }
        rows = atoi(openBracketRows + 1);

        char *closeBracketRows = strchr(openBracketRows, ']');
        if (!closeBracketRows) {
            printf("Error: Missing closing bracket for rows\n");
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }

        // Find columns count
        char *openBracketCols = strchr(closeBracketRows + 1, '[');
        if (!openBracketCols) {
            printf("Error: Missing opening bracket for cols\n");
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }
        cols = atoi(openBracketCols + 1);

        char *closeBracketCols = strchr(openBracketCols, ']');
        if (!closeBracketCols) {
            printf("Error: Missing closing bracket for cols\n");
            free(lineCopyMat);
            free(binaryString);
            return NULL;
        }

        // Start of matrix values
        char *valuesStart = closeBracketCols + 1;
        int expectedCount = rows * cols;
        int count = 0;

        char *value = strtok(valuesStart, ", \t");
        while(value != NULL) {
            value = removeStartEndSpaces(value);
            int number = atoi(value);
            char *binaryNumber = decToBinary10Bit(number);
            for (int i = 0; i < 10; i++) {
                *ptrBinaryString++ = binaryNumber[i];
            }
            *ptrBinaryString++ = '\n';
            count++;
            free(binaryNumber);
            value = strtok(NULL, ", \t");
        }

        // Fill remaining cells with zeros if needed
        while (count < expectedCount) {
            char *binaryNumber = decToBinary10Bit(0);
            for (int i = 0; i < 10; i++) {
                *ptrBinaryString++ = binaryNumber[i];
            }
            *ptrBinaryString++ = '\n';
            count++;
            free(binaryNumber);
        }

        *ptrBinaryString = '\0'; // Null-terminate final string
        free(lineCopyMat);
    }

    else {
        free(lineCopy);
        free(binaryString);
        return NULL;
    }

    free(lineCopy);
    return binaryString;
}
