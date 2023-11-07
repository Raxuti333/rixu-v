#include <stdio.h>
#include <stdlib.h>
#include <compiler.h>
#include <linker.h>
#include <string.h>

void linkArg(const char* arg , LinkerArgs* linker_arg)
{
    if(!strcmp(arg + 2, "no-elf"))
    {
        linker_arg->use_elf = false;
    }
}

void getArgs(int argc, char** argv, char*** input, size_t* input_count, char** output, LinkerArgs* linker_args)
{
    size_t last = 0;
    *input = malloc(sizeof(char*) * argc);

    bool is_output = false;

    for(size_t i = 0; i < argc; ++i)
    {
        size_t size = strlen(argv[i]);

        if(size >= 2 && argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'o': is_output = true; break;
                
                case 'f': linkArg(argv[i], linker_args); break;

                default: break;
            }

            continue;
        }

        if(is_output)
        {
            is_output = false;

            *output = argv[i];
        }
        else
        {
            (*input)[last++] = argv[i];
        }
    }

    *input_count = last;
}

int main(int argc, char** argv)
{
    LinkerArgs linker_args = {.use_elf = true, .entry = "main"};
    CompilerArgs compiler_args;

    char* output;
    size_t input_count;
    char** input;

    getArgs(argc, argv, &input, &input_count, &output, &linker_args);

    if(output == NULL) { return 1; }

    Buffer* buffers = malloc(sizeof(Buffer*) * input_count);

    /* TODO paralelize */
    for(size_t i = 0; i < input_count; ++i)
    {
        FILE* fd = fopen(input[i], "rb");

        fseek(fd, 0, SEEK_END);
        size_t size = ftell(fd);
        fseek(fd, 0, SEEK_SET);

        char* source = malloc(size);

        size_t tmp = fread(source, 1, size, fd);

        fclose(fd);

        buffers[i] = obj_compile(source, compiler_args);

        free(source);
    }

    Buffer out = linker(buffers, input_count, &linker_args);

    FILE* fdout = fopen(output, "wb");

    fwrite(out->buffer, 1, out->last, fdout);

    fclose(fdout);

    /* clean up */
    for(size_t i = 0; i < input_count; ++i) { free(buffers[i]); }

    free(input);
    free(buffers);
    free(out);
}