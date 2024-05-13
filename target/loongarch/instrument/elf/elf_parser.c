/* ref: https://stackoverflow.com/questions/34960383/how-read-elf-header-in-c */
#include "elf_parser.h"
#include <elf.h>

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "symbol.h"
#include "../util/error.h"

/* ELF format */
/* typedef struct */
/* { */
/*   unsigned char e_ident[EI_NIDENT];     /1* Magic number and other info *1/ */
/*   Elf64_Half    e_type;                 /1* Object file type *1/ */
/*   Elf64_Half    e_machine;              /1* Architecture *1/ */
/*   Elf64_Word    e_version;              /1* Object file version *1/ */
/*   Elf64_Addr    e_entry;                /1* Entry point virtual address *1/ */
/*   Elf64_Off     e_phoff;                /1* Program header table file offset *1/ */
/*   Elf64_Off     e_shoff;                /1* Section header table file offset *1/ */
/*   Elf64_Word    e_flags;                /1* Processor-specific flags *1/ */
/*   Elf64_Half    e_ehsize;               /1* ELF header size in bytes *1/ */
/*   Elf64_Half    e_phentsize;            /1* Program header table entry size *1/ */
/*   Elf64_Half    e_phnum;                /1* Program header table entry count *1/ */
/*   Elf64_Half    e_shentsize;            /1* Section header table entry size *1/ */
/*   Elf64_Half    e_shnum;                /1* Section header table entry count *1/ */
/*   Elf64_Half    e_shstrndx;             /1* Section header string table index *1/ */
/* } Elf64_Ehdr; */

/* typedef struct */
/* { */
/*   Elf64_Word	sh_name;		/1* Section name (string tbl index) *1/ */
/*   Elf64_Word	sh_type;		/1* Section type *1/ */
/*   Elf64_Xword	sh_flags;		/1* Section flags *1/ */
/*   Elf64_Addr	sh_addr;		/1* Section virtual addr at execution *1/ */
/*   Elf64_Off	sh_offset;		/1* Section file offset *1/ */
/*   Elf64_Xword	sh_size;		/1* Section size in bytes *1/ */
/*   Elf64_Word	sh_link;		/1* Link to another section *1/ */
/*   Elf64_Word	sh_info;		/1* Additional section information *1/ */
/*   Elf64_Xword	sh_addralign;		/1* Section alignment *1/ */
/*   Elf64_Xword	sh_entsize;		/1* Entry size if section holds table *1/ */
/* } Elf64_Shdr; */


#include <fcntl.h>
#include <unistd.h>
#include <alloca.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

/* lookup .symtab and .dynsym section */
static bool find_sym_table(Elf64_Shdr *shdr, int shnum, Elf64_Word type, int *sym_idx, int *str_idx)
{
    /* type can be SHT_SYMTAB or SHT_DYNSYM */
    for (int i = 0; i < shnum; ++i) {
        if (shdr[i].sh_type == type) {
            *sym_idx = i;
            *str_idx = shdr[i].sh_link;
            return true;
        }
    }
    return false;
}

void parse_elf_symbol_with_fd(int fd, uint64_t map_base, void **pp_img)
{
    char filepath[1024];
    char link[64];

    sprintf(link, "/proc/self/fd/%d", fd);
    int n = readlink(link, filepath, 1024);
    if (n < 0 || n >= 1024) {
        fprintf(stderr, "get file path failed\n");
        return;
    }
    filepath[n] = 0;    /* readlink不会给字符串结尾加\0 */

    lsdebug("read symbol in %s\n", filepath);
    parse_elf_symbol(filepath, map_base, pp_img);
}

/* parse ELF and print all symbols() */
void parse_elf_symbol(const char* pathname, uint64_t map_base, void **pp_img)
{
    Elf64_Ehdr *ehdr = NULL;
    int shnum;
    size_t shsz;
    Elf64_Shdr *shdr = NULL;
    uint64_t secsz;
    int sym_idx;
    int str_idx;
    Elf64_Sym *syms = NULL;
    char *strs = NULL;
    int nsyms;
    *pp_img = NULL;

    int fd = open(pathname, O_RDONLY, 0);
    if (fd < 0) {
        fprintf(stderr, "Error while loading %s\n", pathname);
        perror("open file failed\n");
        return;
    }

    /* 1. read ELF header */
    ehdr = (Elf64_Ehdr *)alloca(sizeof(Elf64_Ehdr));
    if (pread(fd, ehdr, sizeof(Elf64_Ehdr), 0) != sizeof(Elf64_Ehdr)) {
        fprintf(stderr, "read elf header failed\n");
        goto give_up;
    }

    /* 2. check if is an ELF shared object or exec */
    // TODO add more check
    if (!((memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0) && ((ehdr->e_type == ET_DYN) || (ehdr->e_type == ET_EXEC)))) {
        fprintf(stderr, "not a shared object\n");
        goto give_up;
    }

    /* 3. read section headers */
    shnum = ehdr->e_shnum;
    shsz = shnum * ehdr->e_shentsize;
    shdr = (Elf64_Shdr *)alloca(shsz);
    if (pread(fd, shdr, shsz, ehdr->e_shoff) != shsz) {
        fprintf(stderr, "read section headers failed\n");
        goto give_up;
    }


    /* 4. lookup .symtab and .dynsym section */
    /* only need one of both, because dynsym is included in symtab */
    if (!find_sym_table(shdr, shnum, SHT_SYMTAB, &sym_idx, &str_idx)
        && !find_sym_table(shdr, shnum, SHT_DYNSYM, &sym_idx, &str_idx)) {
        fprintf(stderr, "find no symbol table\n");
        goto give_up;
    }

    /* 4.1 read symbol string */
    /* TODO add assert strtab exist */
    secsz = shdr[str_idx].sh_size;
    strs = (char *)malloc(secsz);
    if (!strs || pread(fd, strs, secsz, shdr[str_idx].sh_offset) != secsz) {
        fprintf(stderr, "read .symstr section failed\n");
        goto give_up;
    }

    /* 4.2 read symbol table */
    secsz = shdr[sym_idx].sh_size;
    if (secsz / sizeof(Elf64_Sym) > INT_MAX) {
        fprintf(stderr, "Implausibly large symbol table, give up\n");
        goto give_up;
    }
    nsyms = secsz / sizeof(Elf64_Sym);
    syms = (Elf64_Sym *)malloc(secsz);
    if (!syms || pread(fd, syms, secsz, shdr[sym_idx].sh_offset) != secsz) {
        fprintf(stderr, "read .symtab section failed\n");
        goto give_up;
    }


    // alloc IMG
    *pp_img = image_alloc(pathname, map_base);

    // alloc SEC
    char sec_n[64];
    Elf64_Shdr *s = (void*)shdr + ehdr->e_shstrndx * sizeof(Elf64_Shdr); // get string table section pointer
    for(int i = 0;i < shnum;i++){
        pread(fd, sec_n, 64, s->sh_offset + shdr[i].sh_name);

        if(shdr[i].sh_type == SHT_PROGBITS && ((shdr[i].sh_flags & SHF_EXECINSTR) != 0)){
            SEC * sec = sec_alloc(*pp_img, sec_n, shdr[i].sh_addr, shdr[i].sh_size);
            uint64_t sec_start = map_base + shdr[i].sh_addr;
            uint64_t sec_end = sec_start + shdr[i].sh_size;
            /**fprintf(stderr,"sec name : %s            (%p) %d\n", sec_n, sec_start, shdr[i].sh_size);*/

            // alloc RTN and collect RTN if section has is
            for (int i = 0; i < nsyms; ++i) {
                /* Throw away entries which we do not need.  */
                if (syms[i].st_shndx == SHN_UNDEF
                    /**|| syms[i].st_shndx >= SHN_LORESERVE*/
                    || ELF64_ST_TYPE(syms[i].st_info) != STT_FUNC) {
                    continue;
                }

                if(map_base + syms[i].st_value < sec_end 
                        && map_base + syms[i].st_value >= sec_start) {
                    /**fprintf(stderr,"rtn name : %s    (%p) %d\n",  strs + syms[i].st_name,  map_base + syms[i].st_value, syms[i].st_size);*/
                    rtn_alloc(sec, strs + syms[i].st_name, map_base + syms[i].st_value, syms[i].st_size);
                }
            }
        }else{
        }
    }


give_up:
    free(strs);
    free(syms);
    close(fd);
}
