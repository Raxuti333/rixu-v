#ifndef COMPILE_H
#define COMPILE_H

#include <buffer.h>

typedef struct CompilerArgs {} CompilerArgs;

Buffer obj_compile(char* source, CompilerArgs args);

#endif