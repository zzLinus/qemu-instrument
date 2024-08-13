- [x] buffer_linux.so          `INS_InsertFillBuffer()` 没写                     OK😆
- [ ] countreps.so    (s) x86 magic
- [ ] detach.so          🧐
- [ ] emudiv.so     🧐`PIN_AddInternalExceptionHandler()` 没写    
- [ ] follow_child_tool.so   `Instrumenting child processes` 没写
- [ ] fork_jit_tool.so  `Instrumenting Before and After Forks` 没写 
- [x] imageload.so              🧐                                               OK😆
- [x] inscount0.so                                                               小🐲人
- [x] inscount1.so       数ins per bbl bbl_count                                 小🐲人
- [ ] inscount2.so     🧐 Letting Pin Decide Where to Instrument
- [x] inscount_tls.so      Thread Local Storage 没写                             OK😆
- [ ] invocation.so       插桩位置 `IPOINT` 目前只支持 `IPOINT_BEFORE`
- [x] isampling.so         isampling                                             小🐲人
- [x] itrace.so            跟isampling基本是一个东西                             小🐲人
- [ ] malloc_mt.so           🥹 multi-thread stuff
- [x] malloctrace.so         routine                                             小🐲人
- [x] nonstatica.so      🧐                                                      OK😆
- [x] pinatrace.so           mem_trace                                           小🐲人
- [ ] proccount.so         🧐 `RTN_AddInstrumentFunction()`没写
- [ ] replacesigprobed.so     整个`probe mode`都没写呢 
- [ ] safecopy.so            🧐 `PIN_SafeCopy` 没写       
- [ ] stack-debugger.so   `Advanced Debugging Extensions on Linux` 没写
- [x] staticcount.so             🧐                                               OK😆
- [x] strace.soe                 strace                                           小🐲人


# Pin API 实现情况

打勾的为已实现的API。各API的具体效果请参考 Intel Pin 的用户手册。

{{TODO 这里只列举了INS相关的API，其他的后续也加进来。}}
{{TODO 可以另外写一个文档，说明一些有差异的部分，以及原创的API（INS_InsertInlineAdd） }}

## Instrumentation API
- [x] PIN_CALLBACK 	INS_AddInstrumentFunction (INS_INSTRUMENT_CALLBACK fun, VOID *val)
- [x] 80  VOID 	INS_InsertPredicatedCall (INS ins, IPOINT ipoint, AFUNPTR funptr,...)
- [x] 650 VOID 	INS_InsertCall (INS ins, IPOINT action, AFUNPTR funptr,...)
- [x] 150 VOID 	INS_InsertIfCall (INS ins, IPOINT action, AFUNPTR funptr,...)
- [x] 176 VOID 	INS_InsertThenCall (INS ins, IPOINT action, AFUNPTR funptr,...)
- [ ] 5   VOID 	INS_InsertIfPredicatedCall (INS ins, IPOINT action, AFUNPTR funptr,...)
- [ ] 5   VOID 	INS_InsertThenPredicatedCall (INS ins, IPOINT action, AFUNPTR funptr,...)
- [ ] 26  VOID 	INS_InsertFillBuffer (INS ins, IPOINT action, BUFFER_ID id,...)
- [ ] 4   VOID 	INS_InsertFillBufferPredicated (INS ins, IPOINT action, BUFFER_ID id,...)
- [ ] 4   VOID 	INS_InsertFillBufferThen (INS ins, IPOINeESeeT action, BUFFER_ID id,...)

## IARG_TYPE 实现情况

i: unimplemented, s: unsupported, r: reserved
eee
- [x] IARG_INVALID,
- [x] IARG_ADDRINT,
- [x] IARG_PTR,
- [x] IARG_BOOL,
- [x] IARG_UINT32,
- [x] IARG_UINT64,
- [x] IARG_INST_PTR,
- [x] IARG_REG_VALUE,
- [x] IARG_REG_REFERENCE,
- [ ] IARG_REG_CONST_REFERENCE,
- [x] IARG_MEMORYREAD_EA,
- [ ] IARG_MEMORYREAD2_EA,       (s)
- [x] IARG_MEMORYWRITE_EA,
- [x] IARG_MEMORYREAD_SIZE,
- [x] IARG_MEMORYWRITE_SIZE,
- [ ] IARG_MEMORYREAD_PTR,
- [ ] IARG_MEMORYREAD2_PTR, (s)
- [ ] IARG_MEMORYWRITE_PTR,
- [ ] IARG_MEMORYOP_PTR,
- [ ] IARG_MULTI_MEMORYACCESS_EA,
- [ ] IARG_MULTI_ELEMENT_OPERAND,
- [ ] IARG_REWRITE_SCATTERED_MEMOP,
- [ ] IARG_EXPLICIT_MEMORY_EA,
- [x] IARG_BRANCH_TAKEN,
- [x] IARG_BRANCH_TARGET_ADDR,
- [x] IARG_FALLTHROUGH_ADDR,
- [x] IARG_EXECUTING,
- [ ] IARG_FIRST_REP_ITERATION, (s)
- [ ] IARG_PREDICATE,   (r)
- [ ] IARG_STACK_VALUE, (r)
- [ ] IARG_STACK_REFERENCE, (r)
- [ ] IARG_MEMORY_VALUE, (r)
- [ ] IARG_MEMORY_REFERENCE, (r)
- [x] IARG_SYSCALL_NUMBER,
- [x] IARG_SYSARG_REFERENCE,
- [x] IARG_SYSARG_VALUE,
- [ ] IARG_SYSRET_VALUE,
- [ ] IARG_SYSRET_ERRNO,
- [ ] IARG_FUNCARG_CALLSITE_REFERENCE,
- [x] IARG_FUNCARG_CALLSITE_VALUE,
- [ ] IARG_FUNCARG_ENTRYPOINT_REFERENCE,
- [x] IARG_FUNCARG_ENTRYPOINT_VALUE,
- [ ] IARG_FUNCRET_EXITPOINT_REFERENCE,
- [x] IARG_FUNCRET_EXITPOINT_VALUE,
- [x] IARG_RETURN_IP,
- [ ] IARG_ORIG_FUNCPTR,
- [ ] IARG_PROTOTYPE,
- [ ] IARG_THREAD_ID,
- [ ] IARG_CONTEXT,
- [ ] IARG_CONST_CONTEXT,
- [ ] IARG_PARTIAL_CONTEXT,
- [ ] IARG_PRESERVE,
- [ ] IARG_RETURN_REGS,
- [ ] IARG_CALL_ORDER,
- [ ] IARG_IARGLIST,
- [ ] IARG_FAST_ANALYSIS_CALL, (s)
- [x] IARG_MEMORYOP_EA,
- [x] IARG_MEMORYOP_SIZE,
- [ ] IARG_MEMORYOP_MASKED_ON, (s)
- [x] IARG_TSC,
- [ ] IARG_FILE_NAME, (r)
- [ ] IARG_LINE_NO, (r)
- [ ] IARG_LAST, (r)
- [x] IARG_END

注：IARG_TSC

### 访存指令

{{FIXME 这段我也不知道是啥，之前实现Pin API时记录的信息}}

(addr = rj + si12)
ld.b rd, rj, si12
ld.h rd, rj, si12
ld.w rd, rj, si12
ld.d rd, rj, si12
ld.bu rd, rj, si12
ld.hu rd, rj, si12
ld.wu rd, rj, si12
st.b rd, rj, si12
st.h rd, rj, si12
st.w rd, rj, si12
st.d rd, rj, si12

(addr = rj + rk)
ldx.b rd, rj, rk
ldx.h rd, rj, rk
ldx.w rd, rj, rk
ldx.d rd, rj, rk
ldx.bu rd, rj, rk
ldx.hu rd, rj, rk
ldx.wu rd, rj, rk
stx.b rd, rj, rk
stx.h rd, rj, rk
stx.w rd, rj, rk
stx.d rd, rj, rk

(addr = rj + (si14 << 2))
ldptr.w rd, rj, si14
ldptr.d rd, rj, si14
stptr.w rd, rj, si14
stptr.d rd, rj, si14

(addr = rj + si12)
preld hint, rj, si12

(复杂，见文档)
preldx hint, rj, rk

边界访存 

(addr = rj, rk是边界检查)
ldgt.b rd, rj, rk
ldgt.h rd, rj, rk
ldgt.w rd, rj, rk
ldgt.d rd, rj, rk
ldle.b rd, rj, rk
ldle.h rd, rj, rk
ldle.w rd, rj, rk
ldle.d rd, rj, rk
stgt.b rd, rj, rk
stgt.h rd, rj, rk
stgt.w rd, rj, rk
stgt.d rd, rj, rk
stle.b rd, rj, rk
stle.h rd, rj, rk
stle.w rd, rj, rk
stle.d rd, rj, rk

原子访存 

(addr = rj)
amswap.w rd, rk, rj
amswap_db.w rd, rk, rj
amswap.d rd, rk, rj
amswap_db.d rd, rk, rj
amadd.w rd, rk, rj
amadd_db.w rd, rk, rj
amadd.d rd, rk, rj
amadd_db.d rd, rk, rj
amand.w rd, rk, rj
amand_db.w rd, rk, rj
amand.d rd, rk, rj
amand_db.d rd, rk, rj
amor.w rd, rk, rj
amor_db.w rd, rk, rj
amor.d rd, rk, rj
amor_db.d rd, rk, rj
amxor.w rd, rk, rj
amxor_db.w rd, rk, rj
amxor.d rd, rk, rj
amxor_db.d rd, rk, rj
ammax.w rd, rk, rj
ammax_db.w rd, rk, rj
ammax.d rd, rk, rj
ammax_db.d rd, rk, rj
ammin.w rd, rk, rj
ammin_db.w rd, rk, rj
ammin.d rd, rk, rj
ammin_db.d rd, rk, rj
ammax.wu rd, rk, rj
ammax_db.wu rd, rk, rj
ammax.du rd, rk, rj
ammax_db.du rd, rk, rj
ammin.wu rd, rk, rj
ammin_db.wu rd, rk, rj
ammin.du rd, rk, rj
ammin_db.du rd, rk, rj

LL/SC 

(addr = rj + si14 << 2)
ll.wrd, rj, si14
ll.drd, rj, si14
sc.wrd, rj, si14
sc.drd, rj, si14

浮点普通访存

(addr = rj + si12)
fld.s fd, rj, si12
fld.d fd, rj, si12
fst.s fd, rj, si12
fst.d fd, rj, si12

(addr = rj + rk)
fldx.s fd, rj, rk
fldx.d fd, rj, rk
fstx.s fd, rj, rk
fstx.d fd, rj, rk

浮点边界访存

(addr = rj)
fldgt.s fd, rj, rk
fldgt.d fd, rj, rk
fldle.s fd, rj, rk
fldle.d fd, rj, rk
fstgt.s fd, rj, rk
fstgt.d fd, rj, rk
fstle.s fd, rj, rk
fstle.d fd, rj, rk

向量扩展

(addr = rj + si12)
vld vd, rj, s12
xvld vd, rj, s12
vst vd, rj, si12
xvst vd, rj, si12

(addr = rj + rk)
vldx vd, rj, rk
xvldx vd, rj, rk
vstx vd, rj, rk
xvstx vd, rj, rk

vldrepl.b vd, rj, si12      (rj + si12)
xvldrepl.b xd, rj, si12     
vldrepl.h vd, rj, si11      (rj + si11 << 1)
xvldrepl.h xd, rj, si11
vldrepl.w vd, rj, si10      (rj + si10 << 2)
xvldrepl.w xd, rj, si10
vldrepl.d vd, rj, si9       (rj + si9 << 3)
xvldrepl.d xd, rj, si9
vstelm.b vd, rj, si8, idx   (rj + si8)
xvstelm.b xd, rj, si8, idx
vstelm.h vd, rj, si8, idx   (rj + si8 << 1)
xvstelm.h xd, rj, si8, idx
vstelm.w vd, rj, si8, idx   (rj + si8 << 2)
xvstelm.w xd, rj, si8, idx
vstelm.d vd, rj, si8, idx   (rj + si8 << 3)
xvstelm.d xd, rj, si8, idx

二进制扩展

(addr = rj + si12) 长度不定
ldl.w rd, rj, si12
ldl.d rd, rj, si12
ldr.w rd, rj, si12
ldr.d rd, rj, si12
stl.w rd, rj, si12
stl.d rd, rj, si12
str.w rd, rj, si12
str.d rd, rj, si12

## INS inspection API

i: unimplemented, s: unsupported, r: reserved, d: deprecated
- [ ] BOOL INS_HasRealRep (INS ins) (s)
- [ ] BOOL INS_IsStandardMemop (INS ins) (s)
- [ ] BOOL INS_IsCacheLineFlush (INS ins) (s)
- [ ] INT32 INS_Category (const INS ins) (s)
- [ ] INT32 INS_Extension (const INS ins) (s)
- [ ] UINT32 INS_EffectiveAddressWidth (INS ins)
- [x] PIN_DEPRECATED_API USIZE INS_MemoryWriteSize (INS ins) (d)
- [x] PIN_DEPRECATED_API USIZE INS_MemoryReadSize (INS ins) (d)
- [x] PREDICATE INS_GetPredicate (INS ins)
- [x] BOOL INS_IsMemoryRead (INS ins)
- [x] BOOL INS_IsMemoryWrite (INS ins)
- [ ] BOOL INS_HasMemoryRead2 (INS ins) (s)
- [x] BOOL INS_HasFallThrough (INS ins)
- [ ] BOOL INS_IsSysenter (INS ins) (s)
- [ ] BOOL INS_IsXbegin (INS ins) (s)
- [ ] BOOL INS_IsXend (INS ins) (s)
- [ ] BOOL INS_IsLea (INS ins) (s)
- [ ] BOOL INS_IsNop (INS ins) (s)
- [ ] std::string OPCODE_StringShort (UINT32 opcode) (s)
- [x] std::string INS_Mnemonic (INS ins)
- [x] BOOL INS_IsBranch (INS ins)
- [x] BOOL INS_IsDirectBranch (INS ins)
- [x] BOOL INS_IsDirectCall (INS ins)
- [x] PIN_DEPRECATED_API BOOL INS_IsDirectBranchOrCall (INS ins) (d)
- [x] BOOL INS_IsDirectControlFlow (INS ins)
- [ ] BOOL INS_IsHalt (INS ins) (s)
- [x] PIN_DEPRECATED_API BOOL INS_IsBranchOrCall (INS ins) (d)
- [ ] BOOL INS_Stutters (INS ins) (s)
- [ ] BOOL INS_IsPcMaterialization (INS ins)
- [x] BOOL INS_IsCall (INS ins)
- [x] BOOL INS_IsControlFlow (INS ins)
- [ ] BOOL INS_IsFarCall (INS ins) (s)
- [ ] BOOL INS_IsFarJump (INS ins) (s)
- [ ] BOOL INS_IsDirectFarJump (INS ins) (s)
- [ ] BOOL INS_IsValidForIpointAfter (INS ins)
- [ ] BOOL INS_IsValidForIpointTakenBranch (INS ins)
- [ ] BOOL INS_IsVgather (INS ins) (s)
- [ ] BOOL INS_IsVscatter (INS ins) (s)
- [ ] BOOL INS_HasMemoryVector (INS ins)
- [ ] BOOL INS_HasScatteredMemoryAccess (INS ins) (s)
- [ ] BOOL INS_IsAmx (INS ins) (s)
- [ ] VOID INS_GetFarPointer (INS ins, UINT16 &segment_selector, UINT32 &displacement) (s)
- [ ] BOOL INS_IsProcedureCall (INS ins)
- [x] BOOL INS_IsInterrupt (INS ins)
- [x] BOOL INS_IsRet (INS ins)
- [ ] BOOL INS_IsSysret (INS ins) (s)
- [ ] BOOL INS_IsFarRet (INS ins) (s)
- [x] BOOL INS_IsPrefetch (INS ins)
- [ ] BOOL INS_IsSub (const INS ins)
- [x] BOOL INS_IsMov (const INS ins)
- [ ] BOOL INS_IsMovdir64b (const INS ins) (s)
- [ ] BOOL INS_IsMovFullRegRegSame (const INS ins) (s)
- [x] BOOL INS_IsAtomicUpdate (const INS ins)
- [ ] BOOL INS_IsRDTSC (const INS ins) (s)
- [ ] BOOL INS_IsMaskMov (const INS ins) (s)
- [ ] REG INS_RepCountRegister (INS ins) (s)
- [ ] PIN_DEPRECATED_API BOOL INS_IsIndirectBranchOrCall (INS ins) (d)
- [x] BOOL INS_IsIndirectControlFlow (INS ins)
- [ ] REG INS_SegmentRegPrefix (INS ins) (s)
- [x] BOOL INS_HasExplicitMemoryReference (INS ins)
- [ ] VOID PIN_SetSyntaxIntel () (s)
- [ ] VOID PIN_SetSyntaxATT () (s)
- [ ] VOID PIN_SetSyntaxXED () (s)
- [ ] REG INS_RegR (INS x, UINT32 k) (s)
- [ ] REG INS_RegW (INS x, UINT32 k) (s)
- [x] OPCODE INS_Opcode (INS ins)
- [ ] std::string CATEGORY_StringShort (UINT32 num) (s)
- [ ] std::string EXTENSION_StringShort (UINT32 num) (s)
- [ ] BOOL INS_SegPrefixIsMemoryRead (INS ins) (s)
- [ ] BOOL INS_SegPrefixIsMemoryWrite (INS ins) (s)
- [ ] BOOL INS_AddressSizePrefix (INS ins) (s)
- [ ] BOOL INS_BranchNotTakenPrefix (INS ins) (s)
- [ ] BOOL INS_BranchTakenPrefix (INS ins) (s)
- [ ] BOOL INS_LockPrefix (INS ins) (s)
- [ ] BOOL INS_OperandSizePrefix (INS ins) (s)
- [ ] BOOL INS_RepPrefix (INS ins) (s)
- [ ] BOOL INS_RepnePrefix (INS ins) (s)
- [ ] BOOL INS_SegmentPrefix (INS ins) (s)
- [ ] BOOL INS_IsXchg (INS ins) (s)
- [ ] BOOL INS_IsStringop (INS ins) (s)
- [ ] BOOL INS_IsIRet (INS ins) (s)
- [ ] UINT32 INS_MaxNumRRegs (INS x) (s)
- [ ] UINT32 INS_MaxNumWRegs (INS x) (s)
- [ ] BOOL INS_RegRContain (const INS ins, const REG reg) (s)
- [ ] BOOL INS_RegWContain (const INS ins, const REG reg) (s)
- [ ] BOOL INS_FullRegRContain (const INS ins, const REG reg) (s)
- [ ] BOOL INS_FullRegWContain (const INS ins, const REG reg) (s)
- [ ] BOOL INS_IsStackRead (const INS ins) (s)
- [ ] BOOL INS_IsStackWrite (const INS ins) (s)
- [ ] BOOL INS_IsIpRelRead (const INS ins) (s)
- [ ] BOOL INS_IsIpRelWrite (const INS ins) (s)
- [ ] BOOL INS_IsPredicated (INS ins) (s)
- [ ] BOOL INS_IsOriginal (INS ins)
- [x] std::string INS_Disassemble (INS ins)
- [ ] ADDRDELTA INS_MemoryDisplacement (INS ins) (s)
- [ ] REG INS_MemoryBaseReg (INS ins) (s)
- [ ] REG INS_MemoryIndexReg (INS ins) (s)
- [ ] UINT32 INS_MemoryScale (INS ins) (s)
- [ ] BOOL INS_ChangeReg (const INS ins, const REG old_reg, const REG new_reg, const BOOL as_read)
- [ ] xed_decoded_inst_t * INS_XedDec (INS ins) (s)
- [ ] REG INS_XedExactMapToPinReg (xed_reg_enum_t xed_reg) (s)
- [ ] PIN_DEPRECATED_API REG INS_XedExactMapToPinReg (unsigned int r) (d)
- [ ] xed_reg_enum_t INS_XedExactMapFromPinReg (REG pin_reg) (s)
- [x] UINT32 INS_OperandCount (INS ins)
- [ ] UINT32 INS_OperandElementCount (INS ins, UINT32 opIdx) (s)
- [ ] BOOL INS_OperandHasElements (INS ins, UINT32 opIdx) (s)
- [ ] USIZE INS_OperandElementSize (INS ins, UINT32 opIdx) (s)
- [ ] UINT32 INS_OperandNameId (INS ins, UINT32 n) (s)
- [ ] BOOL INS_OperandIsMemory (INS ins, UINT32 n)
- [ ] REG INS_OperandMemoryBaseReg (INS ins, UINT32 n) (s)
- [ ] REG INS_OperandMemoryIndexReg (INS ins, UINT32 n) (s)
- [ ] REG INS_OperandMemorySegmentReg (INS ins, UINT32 n) (s)
- [ ] UINT32 INS_OperandMemoryScale (INS ins, UINT32 n) (s)
- [ ] ADDRDELTA INS_OperandMemoryDisplacement (INS ins, UINT32 n) (s)
- [ ] BOOL INS_OperandIsFixedMemop (INS ins, UINT32 n) (s)
- [ ] VOID GetNumberAndSizeOfMemAccesses (INS ins, int *numAccesses, int *accessSize, int *indexSize) (s)
- [x] UINT32 INS_MemoryOperandCount (INS ins)
- [ ] UINT32 INS_MemoryOperandElementCount (INS ins, UINT32 memoryOp) (s)
- [x] USIZE INS_MemoryOperandSize (INS ins, UINT32 memoryOp)
- [ ] USIZE INS_MemoryOperandElementSize (INS ins, UINT32 memoryOp) (s)
- [ ] BOOL INS_OperandIsAddressGenerator (INS ins, UINT32 n) (s)
- [ ] BOOL INS_OperandIsBranchDisplacement (INS ins, UINT32 n) (s)
- [x] BOOL INS_MemoryOperandIsRead (INS ins, UINT32 memopIdx)
- [x] BOOL INS_MemoryOperandIsWritten (INS ins, UINT32 memopIdx)
- [ ] UINT32 INS_MemoryOperandIndexToOperandIndex (INS ins, UINT32 memopIdx)
- [x] BOOL INS_OperandIsReg (INS ins, UINT32 n)
- [ ] BOOL INS_OperandIsSegmentReg (INS ins, UINT32 n) (s)
- [x] REG INS_OperandReg (INS ins, UINT32 n)
- [x] BOOL INS_OperandIsImmediate (INS ins, UINT32 n)
- [x] UINT64 INS_OperandImmediate (INS ins, UINT32 n)
- [ ] BOOL INS_OperandIsImplicit (INS ins, UINT32 n)
- [ ] BOOL INS_RegIsImplicit (INS ins, REG reg)
- [ ] UINT32 INS_OperandWidth (INS ins, UINT32 n) (s)
- [ ] UINT32 INS_OperandSize (INS ins, UINT32 n) (s)
- [x] BOOL INS_OperandRead (INS ins, UINT32 n)
- [x] BOOL INS_OperandWritten (INS ins, UINT32 n)
- [x] BOOL INS_OperandReadOnly (INS ins, UINT32 n)
- [x] BOOL INS_OperandWrittenOnly (INS ins, UINT32 n)
- [x] BOOL INS_OperandReadAndWritten (INS ins, UINT32 n)
- [ ] REG INS_MaskRegister (INS ins) (s)
- [x] BOOL INS_IsSyscall (INS ins)
- [ ] SYSCALL_STANDARD INS_SyscallStd (INS ins) (s)
- [ ] RTN INS_Rtn (INS x)
- [x] INS INS_Next (INS x)
- [x] INS INS_Prev (INS x)
- [x] INS INS_Invalid ()
- [x] BOOL INS_Valid (INS x)
- [x] ADDRINT INS_Address (INS ins)
- [x] USIZE INS_Size (INS ins)
- [ ] PIN_DEPRECATED_API ADDRINT INS_DirectBranchOrCallTargetAddress (INS ins) (d)
- [x] ADDRINT INS_DirectControlFlowTargetAddress (INS ins)
- [x] ADDRINT INS_NextAddress (INS ins)
- [ ] BOOL INS_IsAddedForFunctionReplacement (INS ins)
- [ ] BOOL INS_IsValidForIarg (INS ins, IARG_TYPE argType)
