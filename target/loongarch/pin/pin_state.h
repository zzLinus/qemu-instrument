#ifndef PIN_STATE_H
#define PIN_STATE_H

#include "ins_instrumentation.h"
#include <stdlib.h>

struct PIN_STATE {
    /* TODO: wrap them into struct, make a list, then we can registers many ins_callbacks */
    INS_INSTRUMENT_CALLBACK ins_cb;
    VOID* ins_cb_val;
    TRACE_INSTRUMENT_CALLBACK trace_cb;
    VOID* trace_cb_val;
    FINI_CALLBACK fini_cb;
    VOID* fini_cb_val;
    SYSCALL_ENTRY_CALLBACK syscall_entry_cb;
    VOID* syscall_entry_cb_val;
    SYSCALL_EXIT_CALLBACK syscall_exit_cb;
    VOID* syscall_exit_cb_val;

    CPU_EXEC_ENTER_CALLBACK cpu_exec_enter_cb;
    VOID* cpu_exec_enter_cb_val;
    CPU_EXEC_EXIT_CALLBACK cpu_exec_exit_cb;
    VOID* cpu_exec_exit_cb_val;

    TRACE_BUFFER_CALLBACK buffer_full_cb;
    VOID* buffer_full_val;

    BOOL read_symbol;
    IMAGECALLBACK img_cb;
    VOID* img_cb_val;

    THREAD_START_CALLBACK thread_start_cb;
    VOID* thread_start_val;

    THREAD_FINI_CALLBACK thread_finish_cb;
    VOID* thread_finish_val;

    DESTRUCTFUN thread_destruct_cb;

    FORK_CALLBACK fork_before_cb;
    FORK_CALLBACK fork_afterp_cb;
    FORK_CALLBACK fork_afterc_cb;

    FOLLOW_CHILD_PROCESS_CALLBACK follow_child_cb;

    RTN_INSTRUMENT_CALLBACK rtn_cb;
};

struct PIN_INSTRU_CONTEXT {
    ANALYSIS_CALL ins_if_call_cb;
    bool ins_if_call_valid;
    ANALYSIS_CALL bbl_if_call_cb;
    bool bbl_if_call_valid;
};

struct PIN_BUFFER_TLS{
    uint64_t buffer_size;
    uint64_t elenums;
    uint64_t water_mark;
};

/* FIXME 线程安全吗？ */
extern struct PIN_STATE PIN_state;
extern struct PIN_INSTRU_CONTEXT PIN_instru_ctx;
extern struct PIN_BUFFER_TLS PIN_buffer_info;

/* === 以下接口只在 QEMU 内部调用 === */
void INS_instrument(INS ins);
void TRACE_instrument(TRACE trace);
void IMG_instrument(IMG img);
// NOTE: new added
// TODO:
void THREAD_start_instrument(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v);
void THREAD_finish_instrument(THREADID tid, const CONTEXT* ctxt, INT32 code, VOID* v);
void FOLLOW_CHILD_callback(THREADID tid);

void FORK_before(THREADID tid,CONTEXT* ctxt,VOID* args);
void FORK_afterp(THREADID tid,CONTEXT* ctxt,VOID* args);
void FORK_afterc(THREADID tid,CONTEXT* ctxt,VOID* args);

/* load pintool */
void qemu_pintool_opt_parse(const char *optarg, Error **errp);

#include "../instrument/env.h"
void exit_instrument(CPUArchState *env, int code);
void syscall_instrument(CPUState *cpu);
void syscall_ret_instrument(CPUState *cpu, int num, abi_long ret);
void cpu_exec_enter_instrument(CPUState *cpu, TranslationBlock *tb);
void cpu_exec_exit_instrument(CPUState *cpu, TranslationBlock *last_tb, int tb_exit);
#endif
