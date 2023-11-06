#include <buffer.h>
#include <string.h>
#include <stdlib.h>

Buffer bufferCreate(const size_t size)
{
    Buffer buffer = malloc(sizeof(Buffer) + size);

    buffer->size = size;
    buffer->last = 0;

    return buffer;
}

void bufferPush(Buffer* dst, const uint8_t* src, const size_t size)
{
    if((*dst)->last + size >= (*dst)->size)
    {
        *dst = realloc(*dst, (*dst)->size * 2);
    }

    memcpy((*dst)->buffer + (*dst)->last, src, size);

    (*dst)->last += size;
}