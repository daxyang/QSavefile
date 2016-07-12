#ifndef DEBUG_PRINT
#define DEBUG_PRINT

#include <stdio.h>
#include <string.h>
//#include <malloc.h>

#define ERRBUFLEN 1024

#define ERR_PRINT(str) \
    do \
    { \
        char errbuf[ERRBUFLEN] = {'\0'}; \
        snprintf(errbuf, ERRBUFLEN, "[file: %s line: %d] %s", \
                                    __FILE__, __LINE__, str); \
        fprintf(stderr, "\033[31m"); \
        perror(errbuf); \
        fprintf(stderr, "\033[0m"); \
    } while (0)
#define INFO_PRINT(str) \
    do \
    { \
        printf("\033[31m"); \
        printf("[file: %s line: %d] %s\n", __FILE__, __LINE__, str); \
        printf("\033[0m"); \
    } while(0)
#endif


#endif // DEBUG_PRINT

