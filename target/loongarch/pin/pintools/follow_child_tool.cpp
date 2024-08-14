/*
 * Copyright (C) 2009-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

#include "pintool.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>

/* ===================================================================== */
/* Command line Switches */
/* ===================================================================== */

BOOL FollowChild(CHILD_PROCESS cProcess, VOID* userData)
{
    fprintf(stderr, "before child:%u\n", *(THREADID*)cProcess);
    return true;
}

/* ===================================================================== */

int main(INT32 argc, CHAR** argv)
{
    PIN_Init(argc, argv);

    PIN_AddFollowChildProcessFunction(FollowChild, 0);

    return 0;
}

