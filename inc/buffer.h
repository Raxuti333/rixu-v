#ifndef BUFFER_H
#define BUFFER_H

/*
    Utility header for dynamic buffer
*/

#include <stddef.h>
#include <stdint.h>

typedef struct Buffer 
{
    size_t last, size;

    uint8_t buffer[];
} *Buffer;

Buffer bufferCreate(const size_t size);

void bufferPush(Buffer* dst, const uint8_t* src, const size_t size);

#endif