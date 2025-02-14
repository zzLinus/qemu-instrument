#include "pin_state.h"

struct PIN_STATE PIN_state = {
    .read_symbol = false,
    .ins_cb = NULL,
    .ins_cb_val = NULL,
    .trace_cb = NULL,
    .trace_cb_val = NULL,
    .fini_cb = NULL,
    .fini_cb_val = NULL,
    .syscall_entry_cb = NULL,
    .syscall_entry_cb_val = NULL,
    .syscall_exit_cb = NULL,
    .syscall_exit_cb_val = NULL,
    .cpu_exec_enter_cb = NULL,
    .cpu_exec_enter_cb_val = NULL,
    .cpu_exec_exit_cb = NULL,
    .cpu_exec_exit_cb_val = NULL,
    .img_cb = NULL,
    .img_cb_val = NULL,
    .buffer_full_cb = NULL,
    .buffer_full_val = NULL,
    .thread_start_cb = NULL,
    .thread_start_val = NULL,
    .thread_finish_cb = NULL,
    .thread_finish_val = NULL,
    .thread_destruct_cb = NULL,
    .fork_before_cb = NULL,
    .fork_afterp_cb = NULL,
    .fork_afterc_cb = NULL,
    .follow_child_cb = NULL,
    .rtn_cb = NULL

};

struct PIN_INSTRU_CONTEXT PIN_instru_ctx = {
    .ins_if_call_valid = false,
};

struct PIN_BUFFER_TLS PIN_buffer_info = 
{
    .buffer_size = 0,
    .elenums = 0,
    .water_mark = 0,
};

void INS_instrument(INS ins)
{
    if (PIN_state.ins_cb != NULL) {
        PIN_state.ins_cb(ins, PIN_state.ins_cb_val);
    }
}

void TRACE_instrument(TRACE trace)
{
    if (PIN_state.trace_cb != NULL) {
        PIN_state.trace_cb(trace, PIN_state.trace_cb_val);
    }
    /* 检查是否需要函数插桩 */
    if (PIN_state.read_symbol) {
        RTN_instrument(trace);
    }
}

void exit_instrument(CPUArchState *env, int code)
{
    if (PIN_state.fini_cb != NULL) {
        PIN_state.fini_cb(code, PIN_state.fini_cb_val);
    }
}

// TODO LMJ move to target_mmap
// FIXME use static may cause multi-thread problem
static int need_check_load_so = 0;
static int fd;
static int64_t offset;
static void pin_instrument_load_shared_object_check(CPULoongArchState * env)
{
    /* void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset); */
    uint64_t syscall_nr = env->gpr[11];
    int prot = env->gpr[6];

    /* Only trace mmap a EXEC segment */
    if (syscall_nr != 222 || (prot & PROT_EXEC) == 0) {
        return;
    }
    
    /* save info used after mmap */
    fd = env->gpr[8];
    offset = env->gpr[9];
    need_check_load_so = 1;
}

#include "target/loongarch/instrument/elf/elf_parser.h"
static void pin_instrument_load_shared_object(CPULoongArchState * env)
{
    if (!need_check_load_so) {
        return;
    }
    need_check_load_so = 0;

    uint64_t ret = env->gpr[4];
    IMG img;
    parse_elf_symbol_with_fd(fd, ret - offset, &img);
    // FIXME I can't 100% sure it is an ELF?
    IMG_instrument(img);
}

void syscall_instrument(CPUState *cpu)
{
    if (PIN_state.read_symbol) {
        pin_instrument_load_shared_object_check(cpu->env_ptr);
    }

    CONTEXT ctxt;
    ctxt.env = cpu->env_ptr;
    if (PIN_state.syscall_entry_cb != NULL) {
        /* FIXME: some args are temporarily filled with NULL and 0 */
        PIN_state.syscall_entry_cb(PIN_ThreadId(), &ctxt, 0, PIN_state.syscall_entry_cb_val);
    }
}

void syscall_ret_instrument(CPUState *cpu, int num, abi_long ret)
{
    if (PIN_state.read_symbol) {
        pin_instrument_load_shared_object(cpu->env_ptr);
    }

    CONTEXT ctxt;
    ctxt.env = cpu->env_ptr;
    if (PIN_state.syscall_exit_cb != NULL) {
        /* FIXME: some args are temporarily filled with NULL and 0 */
        PIN_state.syscall_exit_cb(PIN_ThreadId(), &ctxt, 0, PIN_state.syscall_exit_cb_val);
    }
}

void cpu_exec_enter_instrument(CPUState *cpu, TranslationBlock *tb)
{
    if (PIN_state.cpu_exec_enter_cb) {
        PIN_state.cpu_exec_enter_cb(cpu, tb);
    }
}

void cpu_exec_exit_instrument(CPUState *cpu, TranslationBlock *last_tb, int tb_exit)
{
    /* FIEXME: when exec_tb exit for syscall/break, this cb will not be called */
    if (PIN_state.cpu_exec_exit_cb) {
        PIN_state.cpu_exec_exit_cb(cpu, last_tb, tb_exit);
    }
}

void IMG_instrument(IMG img)
{
    if (PIN_state.img_cb) {
        PIN_state.img_cb(img, PIN_state.img_cb_val);
    }
}


void THREAD_start_instrument(THREADID tid, CONTEXT* ctxt, INT32 flags, VOID* v)
{
    if(PIN_state.thread_start_cb){
        PIN_state.thread_start_cb(tid,ctxt,flags,v);
    }
}

void THREAD_finish_instrument(THREADID tid, const CONTEXT* ctxt, INT32 code, VOID* v)
{
    if(PIN_state.thread_finish_cb){
        PIN_state.thread_finish_cb(tid,ctxt,code,v);
    }
}

void FORK_before(THREADID tid,CONTEXT* ctxt,VOID* args)
{
    if(PIN_state.fork_before_cb){
        PIN_state.fork_before_cb(tid,ctxt,args);
    }
}

void FORK_afterp(THREADID tid,CONTEXT* ctxt,VOID* args)
{
    if(PIN_state.fork_afterp_cb){
        PIN_state.fork_afterp_cb(tid,ctxt,args);
    }
}

void FORK_afterc(THREADID tid,CONTEXT* ctxt,VOID* args)
{
    if(PIN_state.fork_afterc_cb){
        PIN_state.fork_afterc_cb(tid,ctxt,args);
    }
}

void FOLLOW_CHILD_callback(THREADID tid)
{
    if(PIN_state.follow_child_cb){
        PIN_state.follow_child_cb(&tid, NULL);
    }
}

