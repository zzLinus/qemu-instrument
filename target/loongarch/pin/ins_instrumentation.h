#ifndef INS_TRUMENTATION_H
#define INS_TRUMENTATION_H

#include "types.h"
#include "instrumentation_arguements.h"
#include "thread.h"
#include "context.h"
#include "reg.h"
#include "../instrument/elf/symbol.h"
#include "qemu/typedefs.h" /* only for CPU_EXEC_ENTER(EXIT)_CALLBACK */


typedef VOID (*INS_INSTRUMENT_CALLBACK) (INS ins, VOID* v);
typedef VOID (*TRACE_INSTRUMENT_CALLBACK) (TRACE trace, VOID *v);
typedef VOID (*FINI_CALLBACK) (INT32 code, VOID *v);
typedef VOID (*FORK_CALLBACK)(THREADID threadid, const CONTEXT* ctxt, VOID* arg);
typedef VOID (*SYSCALL_ENTRY_CALLBACK) (THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);
typedef VOID (*SYSCALL_EXIT_CALLBACK) (THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);
typedef VOID (*CPU_EXEC_ENTER_CALLBACK) (CPUState *cpu, TranslationBlock *tb);
typedef VOID (*CPU_EXEC_EXIT_CALLBACK) (CPUState *cpu, TranslationBlock *last_tb, int tb_exit);
typedef VOID (*IMAGECALLBACK) (IMG img, VOID *v);
typedef const char* STR;

/* Buffering apis */
#define BUFFER_ID uint64_t
#define BUFFER_ID_INVALID -1
#define INVALID_TLS_KEY (-1)

// NOTE: new added
typedef VOID (*THREAD_START_CALLBACK)(THREADID threadIndex, CONTEXT* ctxt, INT32 flags, VOID* v);
typedef VOID (*THREAD_FINI_CALLBACK)(THREADID threadIndex, const CONTEXT* ctxt, INT32 code, VOID* v);
typedef VOID (*DESTRUCTFUN)(void*);

#ifdef __cplusplus
extern "C" {
#endif
BOOL PIN_Init(INT32 argc, CHAR** argv);
VOID PIN_InitSymbols(void);

/* FIXME: change the return type(PIN_CALLBACK) to (void) because I do not know the class of PIN_CALLBACK */
void INS_AddInstrumentFunction(INS_INSTRUMENT_CALLBACK fun, VOID* val);
void TRACE_AddInstrumentFunction(TRACE_INSTRUMENT_CALLBACK fun, VOID *val);
void PIN_AddFiniFunction(FINI_CALLBACK fun, VOID *val); 
void PIN_AddSyscallEntryFunction(SYSCALL_ENTRY_CALLBACK fun, VOID *val);
void PIN_AddSyscallExitFunction(SYSCALL_EXIT_CALLBACK fun, VOID *val);
void PIN_AddCpuExecEnterFunction(CPU_EXEC_ENTER_CALLBACK fun, VOID *val);
void PIN_AddCpuExecExitFunction(CPU_EXEC_EXIT_CALLBACK fun, VOID *val);
void IMG_AddInstrumentFunction(IMAGECALLBACK fun, VOID *val);
STR IMG_Name(IMG img);

VOID INS_InsertCall(INS INS, IPOINT action, AFUNPTR funptr, ...);
VOID INS_InsertPredicatedCall (INS ins, IPOINT ipoint, AFUNPTR funptr,...);
VOID INS_InsertIfCall (INS ins, IPOINT action, AFUNPTR funptr,...);
VOID INS_InsertThenCall (INS ins, IPOINT action, AFUNPTR funptr,...);
VOID BBL_InsertCall(BBL bbl, IPOINT action, AFUNPTR funptr, ...);
VOID BBL_InsertIfCall (BBL bbl, IPOINT action, AFUNPTR funptr,...);
VOID BBL_InsertThenCall (BBL bbl, IPOINT action, AFUNPTR funptr,...);
VOID TRACE_InsertCall(TRACE trace, IPOINT action, AFUNPTR funptr, ...);
VOID RTN_InsertCall(RTN *rtn, IPOINT action, AFUNPTR funptr, ...);
VOID RTN_instrument(TRACE trace);
// NOTE: new added
VOID INS_InsertFillBuffer(INS ins, IPOINT action, BUFFER_ID id, ...);

/* add imm to value in ptr */
VOID INS_InsertInlineAdd(INS ins, IPOINT action, VOID* ptr, UINT64 imm, BOOL atomic);
VOID BBL_InsertInlineAdd(BBL bbl, IPOINT action, VOID* ptr, UINT64 imm, BOOL atomic);

ADDRINT PIN_GetContextReg(const CONTEXT* ctxt, REG reg);
VOID PIN_SetContextReg (CONTEXT *ctxt, REG reg, ADDRINT val);

/* TODO PIN doc: In Linux systems with kernel 5.3 and above, calling this API while jumping to vsyscall area will return special number - VSYSCALL_NR. */
ADDRINT PIN_GetSyscallNumber (const CONTEXT *ctxt, SYSCALL_STANDARD std);
VOID PIN_SetSyscallNumber (CONTEXT *ctxt, SYSCALL_STANDARD std, ADDRINT val);
ADDRINT PIN_GetSyscallArgument (const CONTEXT *ctxt, SYSCALL_STANDARD std, UINT32 argNum);
VOID PIN_SetSyscallArgument (CONTEXT *ctxt, SYSCALL_STANDARD std, UINT32 argNum, ADDRINT val);
ADDRINT PIN_GetSyscallReturn (const CONTEXT *ctxt, SYSCALL_STANDARD std);
VOID PIN_SetSyscallReturn (CONTEXT *ctxt, SYSCALL_STANDARD std, ADDRINT val);
VOID PIN_DumpGuestMemory (const char* filename);
VOID PIN_DumpGuestReg (const char* filename);



typedef VOID* (*TRACE_BUFFER_CALLBACK)(BUFFER_ID id, THREADID tid, const CONTEXT* ctxt, VOID* buf, UINT64 numElements, VOID* v);
BUFFER_ID PIN_DefineTraceBuffer(size_t recordSize, UINT32 numPages, TRACE_BUFFER_CALLBACK fun, VOID* val);

/* NOTE: new added thread api */
typedef VOID (*THREAD_START_CALLBACK)(THREADID threadIndex, CONTEXT* ctxt, INT32 flags, VOID* v);
VOID PIN_AddThreadStartFunction(THREAD_START_CALLBACK fun, VOID* val);
typedef VOID (*THREAD_FINI_CALLBACK)(THREADID threadIndex, const CONTEXT* ctxt, INT32 code, VOID* v);
VOID PIN_AddThreadFiniFunction(THREAD_FINI_CALLBACK fun, VOID* val);
BOOL PIN_SetThreadData(TLS_KEY key, const VOID* data, THREADID threadId);
TLS_KEY PIN_CreateThreadDataKey(DESTRUCTFUN destruct_func);
VOID* PIN_GetThreadData(TLS_KEY key, THREADID threadId);

VOID PIN_AddForkFunction(FPOINT point, FORK_CALLBACK fun, VOID* val);
THREADID PIN_GetPid(void);
VOID PIN_InitLock(PIN_LOCK* lock);
VOID PIN_GetLock(PIN_LOCK* lock, INT32 val);
INT32 PIN_ReleaseLock(PIN_LOCK* lock);


#ifdef __cplusplus
}
#endif


#endif
