#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef MANUAL_ALLOC_H
#define MANUAL_ALLOC_H

#define MAX_MANUAL_ALLOC_NUMBER 32
#define MAX_MANUAL_ALLOC_SIZE 200000

typedef struct{
    unsigned char *address[MAX_MANUAL_ALLOC_NUMBER];
    unsigned int size[MAX_MANUAL_ALLOC_NUMBER];
    unsigned char buffer[MAX_MANUAL_ALLOC_SIZE];
}ManualAllocStruct;

void InitManualAlloc(ManualAllocStruct *ma);
void *ManualAlloc(ManualAllocStruct *ma, size_t size);
void ManualFree(ManualAllocStruct *ma, void * address);
#endif /* MANUAL_ALLOC_H */
