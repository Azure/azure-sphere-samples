/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "logical-dpc.h"

static CallbackNode *volatile callbacks = NULL;

void EnqueueDeferredProc(CallbackNode *node)
{
    uint32_t prevBasePri = BlockIrqs();
    if (!node->enqueued) {
        CallbackNode *prevHead = callbacks;
        node->enqueued = true;
        callbacks = node;
        node->next = prevHead;
    }
    RestoreIrqs(prevBasePri);
}

void InvokeDeferredProcs(void)
{
    CallbackNode *node;
    do {
        uint32_t prevBasePri = BlockIrqs();
        node = callbacks;
        if (node) {
            node->enqueued = false;
            callbacks = node->next;
        }
        RestoreIrqs(prevBasePri);

        if (node) {
            (*node->cb)();
        }
    } while (node);
}
