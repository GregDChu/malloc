#include "malloc.h"
#include <stdio.h>

#define BUF_SIZE 100
#define PTRS_TO_MAKE 330
int main()
{
    // Allocate memory
    void *ptrs[PTRS_TO_MAKE];
    int i = 0;
    int bytes = 0;
    char *buf[BUF_SIZE];
    while(i < PTRS_TO_MAKE)
    {
        void *mem = malloc(i);
        ptrs[i] = mem;
        bytes += i;
        snprintf(&buf, BUF_SIZE, 
        "Bytes allocated - %d\n", bytes);
        fputs(&buf, stderr);
        i++;
    }
    // Free memory
    i = 0;
    while(i < PTRS_TO_MAKE)
    {
        snprintf(&buf, BUF_SIZE,
        "Freeing pointer %p\n", ptrs[i]);
        fputs(&buf, stderr);
        free(ptrs[i]);
        i++;
    }
    return 0;   
}
