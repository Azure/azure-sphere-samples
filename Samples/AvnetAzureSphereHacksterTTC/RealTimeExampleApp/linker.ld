/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

MEMORY
{
    TCM0 (rwx) : ORIGIN = 0x00100000, LENGTH = 64K
    FLASH (rx) : ORIGIN = 0x10000000, LENGTH = 1M
}

/* To run from XIP flash instead of TCM0, alias CODE_REGION AND RODATA_REGION to FLASH. */
REGION_ALIAS("CODE_REGION", TCM0);
REGION_ALIAS("RODATA_REGION", TCM0);
REGION_ALIAS("DATA_REGION", TCM0);
REGION_ALIAS("BSS_REGION", TCM0);

SECTIONS
{
    .text : ALIGN(16) {
        KEEP(*(.vector_table))
        *(.text)
    } >CODE_REGION

    .rodata : {
        *(.rodata)
    } >RODATA_REGION

    .data : {
        *(.data)
    } >DATA_REGION

    .bss : {
        *(.bss)
    } >BSS_REGION

    StackTop = ORIGIN(TCM0) + LENGTH(TCM0);
}
