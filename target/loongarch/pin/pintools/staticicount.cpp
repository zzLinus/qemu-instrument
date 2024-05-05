/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

//
// This tool prints a trace of image load and unload events
//

#include <stdio.h>
#include <iostream>
#include "pintool.h"
#include "../../instrument/elf/symbol.h"

// Pin calls this function every time a new img is loaded
// It can instrument the image, but this example merely
// counts the number of static instructions in the image

VOID ImageLoad(IMG img, VOID* v)
{
	UINT32 count = 0;

	//for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
	//{
	//    for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
	//    {
	//        // Prepare for processing of RTN, an  RTN is not broken up into BBLs,
	//        // it is merely a sequence of INSs
	//        RTN_Open(rtn);

	//        for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
	//        {
	//            count++;
	//        }

	//        // to preserve space, release data associated with RTN after we have processed it
	//        RTN_Close(rtn);
	//    }
	//}
	fprintf(stderr, "Image %s has  %d instructions\n", IMG_Name(img), count);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
	return -1;
}

static VOID Fini(INT32 code, VOID* v) {
	 //print_collected_symbols();
}
 

int main(int argc, char* argv[])
{
    // Initialize pin & symbol manager
    PIN_InitSymbols();

    if (PIN_Init(argc, argv)) return Usage();
 
    // Register Image to be called to instrument functions.
    IMG_AddInstrumentFunction(ImageLoad, 0);
    PIN_AddFiniFunction(Fini, 0);
 
 
    return 0;
}
