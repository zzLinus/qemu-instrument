/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

//
// This tool prints a trace of image load and unload events
//

#include <stdio.h>
#include "pintool.h"
#include "../../instrument/elf/symbol.h"
#include "../ins_inspection.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
using std::endl;
using std::ofstream;
using std::string;

string log_file = "imageload";
ofstream TraceFile;

// Pin calls this function every time a new img is loaded
// It can instrument the image, but this example does not
// Note that imgs (including shared libraries) are loaded lazily

VOID ImageLoad(IMG img, VOID* v) {
		fprintf(stderr, "Loading %s \n",IMG_Name(img));
}

// This function is called when the application exits
// It closes the output file.
VOID Fini(INT32 code, VOID* v)
{
    if (TraceFile.is_open())
    {
        TraceFile.close();
    }
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char* argv[])
{
		const string filename = log_file;
    // Initialize symbol processing
    PIN_InitSymbols();

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    TraceFile.open(filename.c_str());

    // Register ImageLoad to be called when an image is loaded
    IMG_AddInstrumentFunction(ImageLoad, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    return 0;
}
