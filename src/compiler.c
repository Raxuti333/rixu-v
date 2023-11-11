#include "riscv32.h"

/* temporary until i figure out ELF */
#include "robj.h"

#include <buffer.h>
#include <compiler.h>

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define IMPERATIVE_NONE UINT32_MAX
#define IMPERATIVE_TEXT 0
#define IMPERATIVE_DATA 1

#define LABEL           UINT32_MAX - 1
#define NO_ERROR        0
#define LABEL_ARG       1
#define WRONG_ARGS      2
#define TOO_LARGE       3
#define INVALID_INST    4
#define WRONG_TOKENS    5

/*
    get next line or NULL if last line
*/
static inline char* next(char* src)
{
    size_t i = 0;

    while (src[i] != '\0' && src[i] != '\n') { ++i; }
    
    if(src[i] == '\0') { return NULL; }

    return src + i + 1;
}


static inline size_t length(const char* src)
{
    size_t i = 0;

    while (src[i] != '\0' && src[i] != '\n' && src[i] != '#') { ++i; }

    while (i != 0 && src[i - 1] == ' ') { --i; }; 
    
    return i;
}

/*
    formats src to dst and returns is src a label
*/
static inline bool formater(const char* src, const size_t size, char* dst)
{
    bool is_label = false;

    for(size_t i = 0, j = 0; i < size; ++i)
    {
        /* skip for checking if label later */
        if(src[i] == ':') { is_label = true; }

        else if(src[i] == ' ')
        {
            if(i != 0 && src[i - 1] != ',' && src[i - 1] != ' ')
            {
                dst[j++] = ',';
            }
        }

        else { dst[j++] = src[i]; }
    }

    return is_label;
}

/*
    tokenizes src to and puts in tokens
*/
static inline size_t tokenizer(char* src, char** tokens, const size_t max_tokens)
{
    size_t token_count = 1;

    size_t size = strlen(src);

    for(size_t i = 0; i < size; ++i)
    {
        if(src[i] == ',') 
        {
            src[i] = '\0';

            if(token_count < max_tokens)
            {
                tokens[token_count++] = &src[i + 1];
            }

            /* too many tokens exit and handle in caller */
            else { token_count = SIZE_MAX; break; }
        }
    }

    return token_count;
}

static inline const Instruction* getInstruction(const char* inst)
{
    for(size_t i = 0; i < (sizeof(instructions) / sizeof(*instructions)); ++i)
    {
        if(!strcmp(inst, instructions[i].inst))
        {
            return &instructions[i];
        }
    }

    return NULL;
}

static inline const uint8_t getRegisterId(const char* reg)
{
    for(size_t i = 0; i < (sizeof(registers) / sizeof(*registers)); ++i)
    {
        if(!strcmp(reg, registers[i].name))
        {
            return registers[i].number;
        }
    }

    return UINT8_MAX;
}

static inline uint32_t getNumber(const char* imm)
{
    char* terminator;
    uint32_t v = strtol(imm, &terminator, 0);

    if(*terminator == '\0') { return v; }

    /* invalid immedate force line to error */
    if(v != 0) { return (1<<20); }

    return LABEL;
}

/* 
    added binary, hex and ascii support
*/
static inline uint32_t getImmediateValue(const char* imm)
{
    size_t size = strlen(imm);

    if(size == 0) { return 0; }

    if(imm[0] == '\'' && size > 2)
    {
        return imm[1];
    }
    else 
    {
        return getNumber(imm);
    }
}

static inline uint32_t RbuildInstruction(uint32_t base, uint32_t rd, uint32_t rs1, uint32_t rs2, uint32_t* error)
{
    if(rd > 31 || rs1 > 31 || rs2 > 31)
    {
        *error = WRONG_ARGS;

        return 0;
    }

    return base + ((rd & 0b11111) << 7) + ((rs1 & 0b11111) << 15) + ((rs2 & 0b11111) << 20);
}

static inline uint32_t IbuildInstruction(uint32_t base, uint32_t rd, uint32_t rs1, uint32_t imm, uint32_t* error)
{
    if(imm == LABEL) 
    {
        *error = LABEL_ARG;
    }
    else if(imm > 4095)
    {
        *error = TOO_LARGE;
    }
    else if(rd > 31)
    {
        *error = WRONG_ARGS;

        return 0;
    }

    /* check for special immediate types */
    switch((base >> 12) & 0b111)
    {
    
    /* slli instruction */
    case 0x1:
    /* srli and srai instructions */
    case 0x5: 
    
    /* special immediate type handling as imm can only be 5 bits */
    return base + ((rd & 0b11111) << 7) + ((rs1 & 0b11111) << 15) + ((imm & 0b11111) << 20);

    /* default immediate type handling where imm can be 12 bits */
    default: return base + ((rd & 0b11111) << 7) + ((rs1 & 0b11111) << 15) + ((imm & 0b111111111111) << 20);
    }
}

static inline uint32_t SbuildInstruction(uint32_t base, uint32_t rs1, uint32_t rs2, uint32_t imm, uint32_t* error)
{
    if(rs1 > 31 || rs2 > 31)
    {
        *error = WRONG_ARGS;

        return 0;
    }

    else if(imm == LABEL)
    {
        *error = LABEL_ARG;
    }

    else if(imm > 4095)
    {
        *error = TOO_LARGE;

        return 0;
    }

    return base + ((imm & 0b11111) << 7) + ((rs1 & 0b11111) << 15) + ((rs2 & 0b11111) << 20) + (((imm >> 5) & 0b1111111) << 25);
}

static inline uint32_t BbuildInstruction(uint32_t base, uint32_t rs1, uint32_t rs2, uint32_t imm, uint32_t* error)
{
    if(rs1 > 31 || rs2 > 31)
    {
        *error = WRONG_ARGS;

        return 0;
    }

    else if(imm == LABEL)
    {
        *error = LABEL_ARG;
    }

    else if(imm > (1<<13))
    {
        *error = TOO_LARGE;

        return 0;
    }

    return base + ((imm & 0b11111) << 7) + ((rs1 & 0b11111) << 15) + ((rs2 & 0b11111) << 20) + (((imm >> 5) & 0b1111111) << 25);
}

static inline uint32_t UbuildInstruction(uint32_t base, uint32_t rd, uint32_t imm, uint32_t* error)
{
    if(rd > 31)
    {
        *error = WRONG_ARGS;

        return 0;
    }

    else if(imm == LABEL)
    {
        *error = LABEL_ARG;
    }
    
    else if(imm > 1048575)
    {
        *error = TOO_LARGE;

        return 0;
    }

    return base + ((rd & 0b11111) << 7) + ((imm & 0b11111111111111111111) << 12);
}

static inline uint32_t JbuildInstruction(uint32_t base, uint32_t rd, uint32_t imm, uint32_t* error)
{
    if(rd > 31)
    {
        *error = WRONG_ARGS;

        return 0;
    }

    else if(imm == LABEL)
    {
        *error = LABEL_ARG;
    }
    
    else if(imm > 1048575)
    {
        *error = TOO_LARGE;

        return 0;
    }

    return base + ((rd & 0b11111) << 7) + ((imm & 0b11111111111111111111) << 12);
}

static inline void compile_text(const char* line, Buffer* label, Buffer* unresolved, Buffer* code)
{
    char formated[46];
    memset(formated, 0, sizeof(formated));

    size_t token_count;
    char* tokens[4] = {formated, NULL, NULL, NULL};
        
    /* length of line excluding comments */
    size_t size = length(line);

    /* line is too short thus skipped */
    if(size <= 1) { return; }

    /* format and detect for label */
    bool is_label = formater(line, size, formated);

    /* tokenizer */
    token_count = tokenizer(formated, tokens, (sizeof(tokens) / sizeof(*tokens)));

    if(token_count == SIZE_MAX) { printf("too many arguments"); return; }

    else if(is_label) 
    {
        Label tmp;
        memset(&tmp, 0, sizeof(Label));
        
        tmp.in_data = false;
        tmp.line = (*code)->last;
        memcpy(tmp.label, tokens[0], strlen(tokens[0]));

        bufferPush(label, (uint8_t*)&tmp, sizeof(Label));
    }

    else 
    {
        const Instruction* instruction = getInstruction(tokens[0]);

        if(instruction == NULL) { return; }

        uint32_t error = NO_ERROR, binary = 0;

        switch(instruction->type)
        {

        case R:
            if(token_count != 4) { error = WRONG_TOKENS; break; }
            binary = RbuildInstruction(instruction->base, getRegisterId(tokens[1]), getRegisterId(tokens[2]), getRegisterId(tokens[3]), &error);
        break;

        case I:
            if(!strcmp(instruction->inst, "ecall"))
            {
                binary = IbuildInstruction(instruction->base, 0, 0, 0, &error);
                break;
            }
            else if(token_count != 4) { error = WRONG_TOKENS; break; }
            binary = IbuildInstruction(instruction->base, getRegisterId(tokens[1]), getRegisterId(tokens[2]), getImmediateValue(tokens[3]), &error);
        break;

        case S:
            if(token_count != 4) { error = WRONG_TOKENS; break; }
            binary = SbuildInstruction(instruction->base, getRegisterId(tokens[1]), getRegisterId(tokens[2]), getImmediateValue(tokens[3]), &error);
        break;

        case B:
            if(token_count != 4) { error = WRONG_TOKENS; break; }
            binary = BbuildInstruction(instruction->base, getRegisterId(tokens[1]), getRegisterId(tokens[2]), getImmediateValue(tokens[3]), &error);
        break;

        case U:
            if(token_count != 3) { error = WRONG_TOKENS; break; }
            binary = UbuildInstruction(instruction->base, getRegisterId(tokens[1]), getImmediateValue(tokens[2]), &error);
        break;

        case J:
            if(token_count != 3) { error = WRONG_TOKENS; break; }
            binary = JbuildInstruction(instruction->base, getRegisterId(tokens[1]), getImmediateValue(tokens[2]), &error);
        break;

        default: error = INVALID_INST; break;
        }

        switch(error)
        {
        case LABEL_ARG:
            
        UnresolvedAddress tmp;
        memset(&tmp, 0, sizeof(tmp));

        tmp.type = instruction->type;
        tmp.line = (*code)->last;
        memcpy(tmp.label, tokens[3], strlen(tokens[3]));
        bufferPush(unresolved, (uint8_t*)&tmp, sizeof(UnresolvedAddress));

        /* fix to allow negative numbers */
        case TOO_LARGE:

        case NO_ERROR:

        bufferPush(code, (uint8_t*)&binary, sizeof(uint32_t));
        break;

        default: break;
        }
    }

}

static inline void compile_data(const char* line, Buffer* label, Buffer* unresolved, Buffer* data)
{
    /* TODO create data compiler */
    size_t size = length(line);

    if(line[0] == '.')
    {
        if(size > 8 && !strncmp(&line[1], "string", 6))
        {
            bool copying = false;
            for(size_t i = 9; i < size; ++i)
            {
                if(line[i] == '"') 
                { 
                    copying = copying ? false : true;
                    if(!copying) {bufferPush(data, &(char){'\0'}, 1);}
                }

                else if(copying) { bufferPush(data, &line[i], 1); }
            }
        }
    }

    else
    {
        char formated[32];
        memset(formated, 0, sizeof(formated));

        bool is_label = formater(line, size, formated);

        if(!is_label) { return; }

        Label tmp;
        memset(&tmp, 0, sizeof(Label));
        
        tmp.in_data = true;
        tmp.line = (*data)->last;
        memcpy(tmp.label, formated, strlen(formated));

        bufferPush(label, (uint8_t*)&tmp, sizeof(Label));
    }
}

/*
    object compiler
*/
Buffer obj_compile(char* source, CompilerArgs args)
{
    Buffer label = bufferCreate(sizeof(Label[16]));

    Buffer unresolved = bufferCreate(sizeof(UnresolvedAddress[16]));

    Buffer code = bufferCreate(sizeof(uint32_t[256]));

    Buffer data = bufferCreate(sizeof(uint32_t[256]));

    Buffer object = bufferCreate(sizeof(uint32_t[512]));

    uint32_t mode = IMPERATIVE_NONE;

    /* compile loop */
    for(char* line = source; line != NULL; line = next(line))
    {
        if(line[0] == '\0') { break; }

        size_t size = length(line);

        if(size > 1 && line[0] == '.')
        {
            if(size < 5);

            else
            {
                if(!strncmp(line, ".text", 5)) { mode = IMPERATIVE_TEXT; }
                else if(!strncmp(line, ".data", 5)) { mode = IMPERATIVE_DATA; }
            }
        }

        switch(mode)
        {
            case IMPERATIVE_TEXT: compile_text(line, &label, &unresolved, &code); break;

            case IMPERATIVE_DATA: compile_data(line, &label, &unresolved, &data); break;

            default: break;
        }
    }

    Header header = 
    {
        .label_count = label->last / sizeof(Label),
        .unresolved_offset = sizeof(Header) + label->last,
        .unresolved_count = unresolved->last / sizeof(Label),
        .binary_offset = sizeof(Header) + label->last + unresolved->last,
        .binary_length = code->last,
        .data_offset = sizeof(Header) + label->last + unresolved->last + code->last,
        .data_length = data->last,
    };

    bufferPush(&object, (uint8_t*)&header, sizeof(Header));
    bufferPush(&object, label->buffer, label->last);
    bufferPush(&object, unresolved->buffer, unresolved->last);
    bufferPush(&object, code->buffer, code->last);
    bufferPush(&object, data->buffer, data->last);

    free(label);
    free(unresolved);
    free(code);
    free(data);

    return object;
}