#ifndef ROBJ_H
#define ROBJ_H

/*
    Rixu-Object format
    simpe internal object format
*/

#include <stdint.h>
#include <stddef.h>
#include <riscv32.h>
#include <stdbool.h>

typedef struct Header
{
    size_t label_count;
    size_t unresolved_offset;
    size_t unresolved_count;
    size_t binary_offset;
    size_t binary_length;
    size_t data_offset;
    size_t data_length;
} Header;

typedef struct Label 
{
    bool in_data;
    
    char label[32];

    size_t line;
} Label;

typedef struct UnresolvedAddress
{
    FMT type;
    
    size_t line;

    char label[32];
} UnresolvedAddress;

#endif