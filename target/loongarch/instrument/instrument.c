#include "instrument.h"
#include "decoder/assemble.h"
#include "decoder/disasm.h"
#include "decoder/la_print.h"
#include "tr_data.h"
#include "regs.h"
#include "env.h"
#include "util/error.h"
#include "translate.h"
#include "../pin/pin_state.h"
#include "debug.h"


static inline uint32_t read_opcode(CPUState *cs, uint64_t pc)
{
    CPULoongArchState *env = cs->env_ptr;
    return cpu_ldl_code(env, pc);
}


/* Translate and instrument
 *   parse binary -> ins_list
 *   generate TRACE/BBL/INS
 */
#include "include/exec/translate-all.h"
int la_decode(CPUState *cs, TranslationBlock *tb, int max_insns)
{
    /* TODO tr_data中的一些数据和DisasContext差不多？ */
    /* See translator_loop(), loongarch_tr_init_disas_context 
     * 1. Make TB page non-writable
     * 2. Bound the max_insns to avoid TB cross page
     */
    page_protect(tb->pc);
    int64_t bound = -(tb->pc | TARGET_PAGE_MASK) / 4;
    max_insns = MIN(max_insns, bound);

    uint64_t start_pc = tb->pc;
    uint64_t pc = start_pc;
    Ins *origin_ins = NULL;
    int ins_nr = 0;

    TRACE trace = TRACE_alloc(pc);
    BBL bbl = BBL_alloc(pc);
    tr_data.trace = trace;

    while (1) {
        /* Disassemble */
        uint32_t opcode = read_opcode(cs, pc);
        origin_ins = ins_alloc();
        la_disasm(opcode, origin_ins);

        /* Translate */
        INS INS = INS_alloc(pc, opcode, origin_ins);
        INS_translate(cs, INS);
        INS_instrument(INS);
        ++ins_nr;

        /* Append exit */
        if (tr_data.is_jmp == TRANS_NEXT && ins_nr == max_insns) {
            tr_data.is_jmp = TRANS_TOO_MANY;
            INS_append_exit(INS, 0);
        } else if (op_is_condition_branch(origin_ins->op)) {
            INS_append_exit(INS, 1); /* 条件跳转也作为tb结束, tb_link第二个跳转出口 */
        }

#ifdef CONFIG_LMJ_DEBUG
        {
            /* check length(ins_list in INS) == INS->len  */
            int l = 0;
            for (Ins *i = INS->first_ins; i != NULL; i = i->next) {
                l++;
                if (i->next == NULL && i != INS->last_ins) {
                    fprintf(stderr, "assert fail\n");
                    char msg[128];
                    sprint_ins(INS->origin_ins, msg);
                    fprintf(stderr, "origin: %p, %s\n", INS->origin_ins, msg);
                    sprint_ins(i, msg);
                    fprintf(stderr, "i: %p, %s\n", i, msg);
                    sprint_ins(INS->last_ins, msg);
                    fprintf(stderr, "ins->last_ins: %p, %s\n", INS->last_ins, msg);
                    fprintf(stderr, "ins_real_nr: %d,\tc1: %d\n", INS->len, l);
                    /* move this to INS_dump() */
                    for (Ins *i = INS->first_ins; i != NULL; i = i->next) {
                        sprint_ins(i, msg);
                        fprintf(stderr, "%p: %08x\t%s\n", i, opcode, msg);
                    }
                    lsassert(0);
                }
            }
            lsassertm(l == INS->len, "c1: %d, read: %d", l, INS->len);
        }
#endif

        pc += 4;
        BBL_append_INS(bbl, INS);
        if (tr_data.is_jmp != TRANS_NEXT) {
            TRACE_append_BBL(trace, bbl);
            break;
        }
    }
    lsassertm(ins_nr <= max_insns, "tb ins_nr >= max_insns(%d)\n", max_insns);

    TRACE_instrument(trace);

    tr_data.first_ins = trace->bbl_head->ins_head->first_ins;
    tr_data.last_ins = trace->bbl_tail->ins_tail->last_ins;

    {
        /* check length(link_list) == SUM(INS->len) */
        int real_ins_nr = 0;
        for (BBL B = tr_data.trace->bbl_head; B != NULL; B = B->next) {
            for (INS I = B->ins_head; I != NULL; I = I->next) {
                real_ins_nr += I->len;
            }
        }
        lsassertm(tr_data.list_ins_nr == real_ins_nr, "tr_data.list_ins_nr(%d) != real_ins_nr(%d), origin_ins_nr(trace->ins_nr)=%d\n", tr_data.list_ins_nr, real_ins_nr, trace->nr_ins);
    }

    /* The disas_log hook may use these values rather than recompute.  */
    tb->size = pc - start_pc;
    tb->icount = ins_nr;

    return ins_nr;
}


/* 此时所有指令在code cache中位置已经确定，不会再添加新的指令 */
void la_relocation(CPUState *cs, const void *code_buf_rx)
{
    TranslationBlock *tb = tr_data.curr_tb;
    bool enable_tb_link = false;
    if (tb) {
        enable_tb_link = ((tb_cflags(tr_data.curr_tb) & CF_NO_GOTO_TB) == 0);
    }

    /* uintptr_t cur_ins_pos = (uintptr_t)tb->tc.ptr; */
    uintptr_t cur = (uintptr_t)code_buf_rx;

    for (Ins *ins = tr_data.first_ins; ins != NULL; ins = ins->next) {
        /* 跳转指令重定向 */
        /* FIXME：目前假设所有翻译出的 B 0 指令都是需要被重定位的
         * 1. 默认重定位到 context_switch_native_to_bt
         * */
        if (ins->op == LISA_B && ins->opnd[0].val == 0x0) {
            uintptr_t target = context_switch_native_to_bt;
            uintptr_t offset = target - cur;
            lsassert(offset == sextract64(offset, 0, 28));
            ins->opnd[0].val = offset >> 2;
        }

        /* tb_link: 记录要patch的nop指令（或BCC）的地址 */
        if (enable_tb_link) {
            if (tr_data.jmp_ins[0] == ins) {
                tb->jmp_target_arg[0] = cur - (uintptr_t)tb->tc.ptr;
                tb->jmp_reset_offset[0] = cur - (uintptr_t)tb->tc.ptr + 4;
            }
            if (tr_data.jmp_ins[1] == ins) {
                LA_OPCODE op = ins->op;
                if (op_is_condition_branch(op)) {
                    int bcc_jmp_over;
                    if (LISA_BEQZ <= op && op <= LISA_BCNEZ) {
                        bcc_jmp_over = ins->opnd[1].val;
                    } else if (LISA_BEQ <= op && op <= LISA_BGEU) {
                        bcc_jmp_over = ins->opnd[2].val;
                    } else {
                        lsassert(0);
                    }

                    tb->jmp_target_arg[1] = cur - (uintptr_t)tb->tc.ptr;
                    /* 恢复时让其跳转到nop的位置 */
                    tb->jmp_reset_offset[1] = cur - (uintptr_t)tb->tc.ptr + 4 * bcc_jmp_over;
                } else {
                    tb->jmp_target_arg[1] = cur - (uintptr_t)tb->tc.ptr;
                    tb->jmp_reset_offset[1] = cur - (uintptr_t)tb->tc.ptr + 4;
                }
            }
        }
        cur += 4;
    }

    /* TODO: handle segv scenario */
    /* 伪代码: */
    /* for (i = 0; i < ir1_nr; ++i) { */
    /*     tcg_ctx->gen_insn_data[i][0] = pir1->info->address; */
    /*     tcg_ctx->gen_insn_data[i][1] = 0; */
    /*     translate_one_ins(ins[i]); */
    /*     tcg_ctx->gen_insn_end_off[i] = (lsenv->tr_data->real_ir2_inst_num)<<2; */
    /* } */
}



extern int showtrans;

/* ins -> binary */
    /* TODO: 这些code-cache指针没搞清楚 */
    /* code_buf = tcg_ctx->code_gen_ptr; */
    /* if (unlikely((void *)s->code_ptr > s->code_gen_highwater)) { */
    /*     return -1; */
    /* } */
int la_encode(TCGContext *tcg_ctx, void *code_buf)
{
    uint64_t code_size = tr_data.list_ins_nr * 4;

    /* check code_cache overflow. */
    if (code_buf + code_size >
        tcg_ctx->code_gen_buffer + tcg_ctx->code_gen_buffer_size) {
        tr_data.curr_tb = NULL;
        return -1;
    }

    int ins_nr = 0;
    uint32_t *code_ptr = code_buf;
    for (Ins *ins = tr_data.first_ins; ins != NULL; ins = ins->next) {
        uint32_t opcode = la_assemble(ins);
        *code_ptr = opcode;
        ++code_ptr;
        ++ins_nr;
    }

#ifdef CONFIG_LMJ_DEBUG
    /* print translation result */
    if (showtrans == 1 && tr_data.trace != NULL) {
        fprintf(stderr, "\n==== TB_ENCODE ====\n");
        uint32_t *pc = code_buf;
        for (BBL bbl = tr_data.trace->bbl_head; bbl != NULL; bbl = bbl->next) {
            for (INS INS = bbl->ins_head; INS != NULL; INS = INS->next) {
                INS_dump(INS);
                pc += INS->len;
            }
        }
        lsassert(pc == code_ptr);
        fprintf(stderr, "===================\n");
    }
#endif

    lsassertm(tr_data.list_ins_nr == ins_nr, "tr_data.list_ins_nr(%d) != ins_nr(%d)\n", tr_data.list_ins_nr, ins_nr);
    return ins_nr;
}
