#include <riscv32.h>
#include <robj.h>
#include <linker.h>
#include <buffer.h>
#include <string.h>
#include <stdlib.h>
#include <elf.h>

static inline size_t getLabelAddress(const char* label, const Label* labels, const size_t count)
{
    for(size_t i = 0; i < count; ++i)
    {
        printf("%s\n", labels[i].label);
        if(!strcmp(label, labels[i].label))
        {
            return labels[i].line;
        }
    }

    return -1;
}

static inline uint32_t IResolveInstruction(uint32_t base, uint32_t imm)
{
    return (base & 0b11111111111111111111) + ((imm & 0b111111111111) << 20);
}

static inline uint32_t SResolveInstruction(uint32_t base, uint32_t imm)
{
    return (base & 0b1111111111111000001111111) + ((imm & 0b11111) << 7) + (((imm >> 5) & 0b1111111) << 25);
}

static inline uint32_t BResolveInstruction(uint32_t base, uint32_t imm)
{
    return (base & 0b1111111111111000001111111) + ((imm & 0b11111) << 7) + (((imm >> 5) & 0b1111111) << 25);
}

static inline uint32_t UResolveInstruction(uint32_t base, uint32_t imm)
{
    return (base & 0b111111111111) + ((imm & 0b11111111111111111111) << 12);
}

static inline uint32_t JResolveInstruction(uint32_t base, uint32_t imm)
{
    return (base & 0b111111111111) + ((imm & 0b11111111111111111111) << 12);
}

/* TODO generate elf header */
Buffer linker(Buffer* objects, size_t count, LinkerArgs* args)
{
    Buffer label = bufferCreate(sizeof(Label[16]));

    Buffer unresolved = bufferCreate(sizeof(UnresolvedAddress[16]));

    Buffer text = bufferCreate(sizeof(uint32_t[256]));

    Buffer data = bufferCreate(sizeof(uint32_t[256]));

    Buffer binary = bufferCreate(sizeof(uint32_t[512]));

    /* combine objects */
    for(size_t i = 0; i < count; ++i)
    {
        Header* header = (Header*)objects[i]->buffer;
        
        Label* labels = (Label*)(objects[i]->buffer + sizeof(Header));
        UnresolvedAddress* unresolveds = (UnresolvedAddress*)(objects[i]->buffer + header->unresolved_offset);

        for(size_t j = 0; j < header->label_count; ++j)
        {
            if(labels[j].in_data) 
            { 
                labels[j].line += data->last;
                continue;
            }
            
            labels[j].line += text->last;
        }

        for(size_t j = 0; j < header->unresolved_count; ++j)
        {
            unresolveds[j].line += text->last;
        }

        bufferPush(&label, (uint8_t*)labels, header->label_count * sizeof(Label));
        bufferPush(&unresolved, (uint8_t*)unresolveds, header->unresolved_count * sizeof(UnresolvedAddress));
        bufferPush(&text, (uint8_t*)(objects[i]->buffer + header->binary_offset), header->binary_length);
        bufferPush(&data, (uint8_t*)(objects[i]->buffer + header->data_offset), header->data_length);
    }

    /* resolve unresolved */
    for(size_t i = 0; i < unresolved->last / sizeof(UnresolvedAddress); ++i)
    {
        UnresolvedAddress* resolve = (UnresolvedAddress*)(unresolved->buffer + i * sizeof(UnresolvedAddress));

        uint32_t* binary = (uint32_t*)(text->buffer + resolve->line);

        size_t imm = getLabelAddress(resolve->label, (Label*)label->buffer, label->last / sizeof(Label));

        switch(resolve->type)
        {
        case I: *binary = IResolveInstruction(*binary, imm); break;

        case S: *binary = SResolveInstruction(*binary, imm); break;

        case B: *binary = SResolveInstruction(*binary, imm); break;

        case U: *binary = UResolveInstruction(*binary, imm); break;

        case J: *binary = JResolveInstruction(*binary, imm); break;

        default: break;
        }
    }

    if(args->use_elf)
    {
        Elf32_Ehdr elf_header = 
        {
            .e_ident = 
            {
                [EI_MAG0] = 0x7F, 

                [EI_MAG1] = 'E', 
                [EI_MAG2] = 'L', 
                [EI_MAG3] = 'F',

                [EI_CLASS] = ELFCLASS32,

                [EI_DATA] = 1,

                [EI_VERSION] = 1,

                [EI_OSABI] = ELFOSABI_SYSV,

                [EI_ABIVERSION] = 0,

                [EI_PAD] = 0,
            },

            .e_type = ET_EXEC,

            .e_machine = EM_RISCV,

            .e_version = 1,

            .e_entry = (Elf32_Addr)getLabelAddress(args->entry, (Label*)label->buffer, label->last / sizeof(Label)),

            .e_phoff = sizeof(Elf32_Ehdr),

            .e_shoff = 0,

            .e_ehsize = sizeof(Elf32_Ehdr),

            .e_flags = 0,

            .e_phentsize = sizeof(Elf32_Phdr),

            .e_phnum = 1,

            .e_shentsize = sizeof(Elf32_Section),

            .e_shnum = 0,

            .e_shstrndx = 0,
        };
        
        Elf32_Phdr elf_program_header = 
        {
            .p_type = PT_LOAD,

            .p_flags = PF_X,

            .p_offset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr),

            .p_vaddr = 0,

            .p_paddr = 0,

            .p_filesz = text->last,

            .p_memsz = text->last,

            .p_align = 0,
        };

        bufferPush(&binary,(uint8_t*)&elf_header, sizeof(Elf32_Ehdr));
        bufferPush(&binary, (uint8_t*)&elf_program_header, sizeof(Elf32_Phdr));
        /* TODO create elf header */
    }

    bufferPush(&binary, (uint8_t*)text->buffer, text->last);
    bufferPush(&binary, (uint8_t*)data->buffer, data->last);

    free(label);
    free(unresolved);
    free(text);
    free(data);

    return binary;
}