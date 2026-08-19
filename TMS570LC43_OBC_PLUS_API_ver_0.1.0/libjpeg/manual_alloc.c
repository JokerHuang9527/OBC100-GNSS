#include <stdlib.h>
#include <string.h>

#include "manual_alloc.h"

/*
 * manual_alloc.c
 *
 *  Created on: unknown
 *      Author: smith
 */
void InitManualAlloc(ManualAllocStruct *ma)
{
    memset(ma, 0, sizeof(ManualAllocStruct));
}

unsigned char *GetNextAddress(ManualAllocStruct *ma, int index)
{
    int i;
    for(i=index+1; i<MAX_MANUAL_ALLOC_NUMBER; i++)
        if(ma->address[i] != 0)
            return ma->address[i];
    return (ma->buffer+MAX_MANUAL_ALLOC_SIZE);
}

void *ManualAlloc(ManualAllocStruct *ma, size_t size)
{
    int i;
    unsigned char *address = ma->buffer;
    unsigned char *nextAddress = ma->buffer+MAX_MANUAL_ALLOC_SIZE;
    for (i=0; i<MAX_MANUAL_ALLOC_NUMBER; i++)
    {
        nextAddress = GetNextAddress(ma, i);
        if((ma->address[i] == 0) && (size <= (nextAddress-address)))
        {
            ma->size[i] = size;
            ma->address[i] = address;
            return (void *)address;
        }
        address += ma->size[i];
    }
    return 0;
}

void ManualFree(ManualAllocStruct *ma, void * address)
{
    int i;
    for (i=0; i<MAX_MANUAL_ALLOC_NUMBER; i++)
    {
        if(ma->address[i] == address)
        {
            memset(ma->address[i], 0, ma->size[i]);
            ma->size[i] = 0;
            ma->address[i] = 0;
        }
    }
}
