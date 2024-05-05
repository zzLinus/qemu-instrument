#ifndef _ELF_SYMBOL_H_
#define _ELF_SYMBOL_H_
#include <stdint.h>
#include <stdbool.h>
#include "../../pin/types.h"
#include "../../pin/instrumentation_arguements.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct image image;
    typedef void *IMG;

    typedef struct symbol_info {
        struct image *image;
        const char *name;
        uint64_t addr;
        uint64_t size;
    } symbol_info;

    image *image_alloc(const char *path, uintptr_t load_base);
    void image_add_symbol(image *image, const char * name, uint64_t addr, uint64_t size);

    symbol_info *image_get_symbol_by_name(image *image, const char *name);
    /* use this function get symbol info */
    symbol_info *get_symbol_by_pc(uint64_t pc);
    const char *get_symbol_name_by_pc(uint64_t pc);
    void is_sym_in_sec(image* img, uint64_t start, uint64_t size);
    const char *get_img_name(image *image);

    bool is_symbol_name_dupcalited(const char *name);
    void print_collected_symbols(void);

#ifdef __cplusplus
}
#endif

typedef struct pin_rtn {
    const char *name;
    uint64_t addr;
    uint64_t size;
} RTN;

#ifdef __cplusplus
extern "C" {
#endif
    /* In Pin, RTN_FindNameByAddress return std::string */
    const CHAR *RTN_FindNameByAddress(ADDRINT address);
    RTN RTN_FindByName(IMG img, const CHAR *name);
    BOOL RTN_Valid(RTN x);
    VOID RTN_Open(RTN rtn);
    VOID RTN_Close(RTN rtn);

#ifdef __cplusplus
    extern "C" {
#endif
    RTN RTN_alloc(const char *name, uint64_t addr, uint64_t size);
#ifdef __cplusplus
    }
#endif

    /* === 下面为内部实现所需接口 === */
    VOID RTN_add_entry_cb(RTN rtn, ANALYSIS_CALL *cb);
    ANALYSIS_CALL *RTN_get_entry_cbs(uintptr_t pc, int *cnt);
    VOID RTN_add_exit_cb(RTN rtn, ANALYSIS_CALL *cb);
    ANALYSIS_CALL *RTN_get_exit_cbs(uintptr_t pc, int *cnt);
#ifdef __cplusplus
}
#endif

#endif
