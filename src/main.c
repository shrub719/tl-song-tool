#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "attributes.h"

long getBytes(char *str, size_t start, size_t len) {
    char byteString[2];
    long attr = 0;

    for (int i = 0; i < len; i++) {
        char *byteStart = str + (start + i) * 2;
        memcpy(byteString, byteStart, 2);
        // printf("byte %d: %s (shifted %d)\n", i, byteString, (int)(i));
        attr += strtol(byteString, NULL, 16) << (i * 8);
    }

    // printf("attribute: %lx\n", attr);
    return attr;
}

void writeBytes(char *str, long data, size_t len) {
    char dataStr[50];
    uint8_t byte;
    
    for (int i = 0; i < len; i++) {
        byte = (data >> (i * 8)) % 0x100;
        sprintf(dataStr + (i * 2), "%02X", byte);
    }

    dataStr[len*2] = '\0';

    strcpy(str, dataStr);
}

void extract(char *msbpFilename, char *msbtFilename, char *outputFilename) {
    FILE *msbtPtr = fopen(msbtFilename, "r");
    FILE *outPtr = fopen(outputFilename, "w");

    AttrSet attrSet = getDefinitions(msbpFilename);
    char previousBuff[300];
    char buff[300];

    while (fgets(buff, 300, msbtPtr)) {
        if (strlen(buff) >= 10 && (strncmp("attribute:", buff, 10) == 0)) {
            char* label = previousBuff + 7;
            label[strlen(label) - 1] = '\0';
            char* attrStr = buff + 13;
            
            fprintf(outPtr, "[%s]\n", label);
            for (int i = 0; i < attrSet.len; i++) {
                Attr attr = attrSet.attributes[i];
                size_t len = getLength(attr.type);

                switch (attr.type) {
                    case UINT32:
                        fprintf(outPtr, "%s = %d\n", attr.name,
                            (uint32_t)getBytes(attrStr, attr.start, len)
                        );
                        break;
                    case INT32:
                        fprintf(outPtr, "%s = %d\n", attr.name,
                            (int32_t)getBytes(attrStr, attr.start, len)
                        );
                        break;
                    case UINT16:
                        fprintf(outPtr, "%s = %d\n", attr.name,
                            (uint16_t)getBytes(attrStr, attr.start, len)
                        );
                        break;
                    case INT16:
                        fprintf(outPtr, "%s = %d\n", attr.name,
                            (int16_t)getBytes(attrStr, attr.start, len)
                        );
                        break;
                    case BYTE:
                        fprintf(outPtr, "%s = %d\n", attr.name, 
                            (uint8_t)getBytes(attrStr, attr.start, len)
                        );
                        break;
                    case IDK4:
                        fprintf(outPtr, "%s = %d\n", attr.name,
                            (uint32_t)getBytes(attrStr, attr.start, len)
                        );
                        break;
                    case IDK2:
                        fprintf(outPtr, "%s = %d\n", attr.name, 
                            (uint16_t)getBytes(attrStr, attr.start, len)
                        );
                        break;
                    case IDK1:
                        fprintf(outPtr, "%s = %d\n", attr.name, 
                            (uint8_t)getBytes(attrStr, attr.start, len)
                        );
                        break;
                }
            }

            fprintf(outPtr, "\n");
        }
        strcpy(previousBuff, buff);
    }

    fclose(msbtPtr);
    fclose(outPtr);
}

void merge(char *msbpFilename, char *attrFilename, char *msbtFilename, char *outputFilename) {
    FILE *attrPtr = fopen(attrFilename, "r");
    FILE *msbtPtr = fopen(msbtFilename, "r");
    FILE *outPtr = fopen(outputFilename, "w");

    AttrSet attrSet = getDefinitions(msbpFilename);
    char previousBuff[300];
    char buff[300];
    char attrStr[300];
    char attrBuff[300];

    while (fgets(buff, 300, msbtPtr)) {
        if (strlen(buff) >= 10 && ( strncmp("attribute:", buff, 10) == 0 )) {
            /* check labels?
            char* label = previousBuff + 7;
            label[strlen(label) - 1] = '\0';
            */
            char *attrStrPtr = attrStr; // fuckk
            fgets(attrBuff, 300, attrPtr);  // skip label

            for (int i = 0; i < attrSet.len; i++) {
                fflush(stdout);
                Attr attr = attrSet.attributes[i];

                fgets(attrBuff, 300, attrPtr);
                char *dataStr = strchr(attrBuff, '=') + 1;

                size_t len = getLength(attr.type);
                writeBytes(attrStrPtr, atoi(dataStr), len);
                attrStrPtr += len * 2;
            }

            fgets(attrBuff, 300, attrPtr);  // skip newline

            fprintf(outPtr, "attribute: 0x%s\n", attrStr);
        } else {
            fprintf(outPtr, "%s", buff);
        }
        
        strcpy(previousBuff, buff);
    }

    fclose(attrPtr);
    fclose(msbtPtr);
    fclose(outPtr);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("\
tl-attr-tool [command]\n\
\n\
    extract [msbp] [msbt] [output]\n\
        Extract the attribute values from an MSBT file into a human readable and \n\
        editable TOML file.\n\
\n\
        msbp - decompressed .msbp.txt file which contains the attribute \n\
        definitions for the MSBT file\n\
        msbt - decompressed .msbt.txt file\n\
\n\
    merge [msbp] [attr] [msbt] [output]\n\
        Merge an edited attribute file with the MSBT file it was extracted from \n\
        to produce an edited MSBT file.\n\
\n\
        msbp - decompressed .msbp.txt file belonging to the original MSBT file\n\
        attr - extracted attribute file\n\
        msbt - original decompressed .msbt.txt file\n\
");
        return 1;
    }

    char *command = argv[1];

    if (strlen(command) >= 7 && (strncmp("extract", command, 7) == 0)) {
        char *msbpFilename = argv[2];
        char *msbtFilename = argv[3];
        char *outputFilename = argv[4];
        extract(msbpFilename, msbtFilename, outputFilename);
    } else if (strlen(command) >= 5 && (strncmp("merge", command, 5) == 0)) {
        char *msbpFilename = argv[2];
        char *attrFilename = argv[3];
        char *msbtFilename = argv[4];
        char *outputFilename = argv[5];
        merge(msbpFilename, attrFilename, msbtFilename, outputFilename);
    }

    return 0;
}

