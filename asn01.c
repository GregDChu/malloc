#include "malloc.h"
#include <stdio.h>

int main()
{
    void *ptrs[330];
    int i = 0;
    int bytes = 0;
    char *buf[100];
    while(i < 330)
    {
        void *mem = malloc(i);
        ptrs[i] = mem;
        bytes += i;
        snprintf(&buf, 100, 
        "Bytes - %d\n", bytes);
        fputs(&buf, stderr);
        i++;
    }
    i = 0;
    while(i < 330)
    {
        free(ptrs[i]);
        i++;
    }
    return 0;   
}
