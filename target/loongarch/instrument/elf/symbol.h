#ifndef _ELF_SYMBOL_H_
#define _ELF_SYMBOL_H_
#include <stdint.h>
#include <stdbool.h>
#include "../../pin/types.h"
#include "../../pin/instrumentation_arguements.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct pin_rtn RTN;

    typedef struct sec SEC;

    // FIXME: why use void* instead of using IMG directly
    typedef struct image image;
    typedef void* IMG;

    // NOTE: new api
    SEC* IMG_SecHead(IMG img);
    SEC* SEC_Next(SEC* sec);
    bool SEC_Valid(SEC* sec);
    const char* SEC_Name(SEC* sec);
    RTN* SEC_RtnHead(SEC* sec);
    IMG SEC_Img(SEC* sec);

    RTN* RTN_Next(RTN* rtn);
    bool RTN_Valid(RTN* rtn);
    const char* RTN_Name(RTN* rtn);
    uint64_t RTN_Address(RTN* rtn);
    INS RTN_InsHead(RTN* rtn);
    SEC* RTN_Sec(RTN* rtn);

    image* image_alloc(const char* path, uintptr_t load_base);

    SEC* sec_alloc(IMG img, const char* name, uint64_t addr,uint64_t size);
    RTN* sec_get_symbol_by_name(SEC *sec, const char* name);

    /* use this function get symbol info */
    RTN* get_symbol_by_pc(uint64_t pc);
    const char* get_symbol_name_by_pc(uint64_t pc);
    const char* get_img_name(image* image);

    bool is_symbol_name_dupcalited(const char* name);
    void print_collected_symbols(void);

    /* In Pin, RTN_FindNameByAddress return std::string */
    const CHAR* RTN_FindNameByAddress(ADDRINT address);
    VOID RTN_Open(RTN *rtn);
    VOID RTN_Close(RTN *rtn);
    RTN* rtn_alloc(SEC* sec, const char* name, uint64_t addr, uint64_t size);
    RTN* RTN_FindByName(IMG img, const CHAR* name);


    /* === 下面为内部实现所需接口 === */
    VOID RTN_add_entry_cb(RTN* rtn, ANALYSIS_CALL* cb);
    ANALYSIS_CALL* RTN_get_entry_cbs(uintptr_t pc, int *cnt);
    VOID RTN_add_exit_cb(RTN* rtn, ANALYSIS_CALL* cb);
    ANALYSIS_CALL* RTN_get_exit_cbs(uintptr_t pc, int *cnt);
#ifdef __cplusplus
}
#endif

#endif
