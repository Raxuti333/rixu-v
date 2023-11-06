#ifndef LINKER_H
#define LINKER_H

#include <buffer.h>
#include <stdbool.h>

typedef struct LinkerArgs 
{
    bool use_elf;
    char entry[32];
} LinkerArgs;

Buffer linker(Buffer* objects, size_t count, LinkerArgs* args);

#endif