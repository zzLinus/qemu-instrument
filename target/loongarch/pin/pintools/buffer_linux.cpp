#include "../ins_inspection.h"
#include "pintool.h"
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>
using std::cerr;
using std::endl;
using std::hex;
using std::ofstream;
using std::string;

uint64_t bufId;

string log_file = "mlog";

TLS_KEY mlog_key;
/*
 * Number of OS pages for the buffer
 */
#define NUM_BUF_PAGES 1024

/*
 * Record of memory references.  Rather than having two separate
 * buffers for reads and writes, we just use one struct that includes a
 * flag for type.
 */
struct MEMREF {
  ADDRINT pc;
  ADDRINT ea;
  UINT32 size;
  BOOL read;
};

/*
 * MLOG - thread specific data that is not handled by the buffering API.
 */
class MLOG {
public:
  MLOG(THREADID tid);
  ~MLOG();

  VOID DumpBufferToFile(struct MEMREF *reference, UINT64 numElements,
                        THREADID tid);

private:
  ofstream _ofile;
};

MLOG::MLOG(THREADID tid) {
  const string filename = log_file;

  _ofile.open(filename.c_str());

  if (!_ofile) {
    cerr << "Error: could not open output file." << endl;
    exit(1);
  }

  _ofile << hex;
}

MLOG::~MLOG() { _ofile.close(); }

VOID MLOG::DumpBufferToFile(struct MEMREF *reference, UINT64 numElements,
                            THREADID tid) {
  for (UINT64 i = 0; i < numElements; i++, reference++) {
    if (reference->ea != 0)
      _ofile << reference->pc << "   " << reference->ea << endl;
  }
}

/**************************************************************************
 *
 *  Instrumentation routines
 *
 **************************************************************************/

/*
 * Insert code to write data to a thread-specific buffer for instructions
 * that access memory.
 */
VOID Trace(TRACE trace, VOID *v) {
  for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
      // if (!INS_IsStandardMemop(ins) && !INS_HasMemoryVector(ins))
      // {
      //     // We don't know how to treat these instructions
      //     continue;
      // }

      UINT32 memoryOperands = INS_MemoryOperandCount(ins);

      for (UINT32 memOp = 0; memOp < memoryOperands; memOp++) {
        UINT32 refSize = INS_MemoryOperandSize(ins, memOp);

        // Note that if the operand is both read and written we log it once
        // for each.
        if (INS_MemoryOperandIsRead(ins, memOp)) {
          // INS_InsertFillBuffer(ins, IPOINT_BEFORE, bufId, IARG_INST_PTR,
          // offsetof(struct MEMREF, pc), IARG_MEMORYOP_EA,
          //                      memOp, offsetof(struct MEMREF, ea),
          //                      IARG_UINT32, refSize, offsetof(struct MEMREF,
          //                      size),
          //                  IARG_BOOL, TRUE, offsetof(struct MEMREF, read),
          //                  IARG_END);
        }

        if (INS_MemoryOperandIsWritten(ins, memOp)) {
          // INS_InsertFillBuffer(ins, IPOINT_BEFORE, bufId, IARG_INST_PTR,
          // offsetof(struct MEMREF, pc), IARG_MEMORYOP_EA,
          //                      memOp, offsetof(struct MEMREF, ea),
          //                      IARG_UINT32, refSize, offsetof(struct MEMREF,
          //                      size), IARG_BOOL, FALSE, offsetof(struct
          //                      MEMREF, read), IARG_END);
        }
      }
    }
  }
}

/**************************************************************************
 *
 *  Callback Routines
 *
 **************************************************************************/

/*!
 * Called when a buffer fills up, or the thread exits, so we can process it or
 * pass it off as we see fit.
 * @param[in] id		buffer handle
 * @param[in] tid		id of owning thread
 * @param[in] ctxt		application context
 * @param[in] buf		actual pointer to buffer
 * @param[in] numElements	number of records
 * @param[in] v			callback value
 * @return  A pointer to the buffer to resume filling.
 */
VOID *BufferFull(uint64_t id, THREADID tid, const CONTEXT *ctxt, VOID *buf,
                 UINT64 numElements, VOID *v) {
  struct MEMREF *reference = (struct MEMREF *)buf;

  MLOG *mlog = static_cast<MLOG *>(PIN_GetThreadData(mlog_key, tid));

  mlog->DumpBufferToFile(reference, numElements, tid);

  return buf;
}

/*
 * Note that opening a file in a callback is only supported on Linux systems.
 * See buffer-win.cpp for how to work around this issue on Windows.
 */
VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v) {
  // There is a new MLOG for every thread.  Opens the output file.
  MLOG *mlog = new MLOG(tid);

  // Initialize thread-specific data not handled by buffering api.
	mlog_key = PIN_CreateThreadDataKey(0);
  fprintf(stderr, "Thread start tid : %d key :%ld\n",tid, mlog_key);

  PIN_SetThreadData(mlog_key, mlog, tid);
}

VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v) {
	// WARN: don't wark when multi thread,because only one mlog_key get store globally
	MLOG *mlog = static_cast<MLOG *>(PIN_GetThreadData(mlog_key, tid));
	delete mlog;

  fprintf(stderr, "Thread finish\n");
  PIN_SetThreadData(mlog_key, 0, tid);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage() {
  cerr << "This tool demonstrates the basic use of the buffering API." << endl;
  return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet
 * started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments,
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[]) {
  // Initialize PIN library. Print help message if -h(elp) is specified
  // in the command line or the command line is invalid
  if (PIN_Init(argc, argv)) {
    return Usage();
  }

  // Initialize the memory reference buffer;
  // set up the callback to process the buffer.
  //  FIXME:
  bufId = PIN_DefineTraceBuffer(sizeof(struct MEMREF), NUM_BUF_PAGES,
                                BufferFull, 0);
  if (bufId == BUFFER_ID_INVALID) {
    cerr << "Error: could not allocate initial buffer" << endl;
    return 1;
  }


  // add an instrumentation function
  TRACE_AddInstrumentFunction(Trace, 0);

  // add callbacks
  PIN_AddThreadStartFunction(ThreadStart, 0);
  PIN_AddThreadFiniFunction(ThreadFini, 0);

  return 0;
}
