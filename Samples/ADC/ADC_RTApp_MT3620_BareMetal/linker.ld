/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

MEMORY
{
    TCM (rwx) : ORIGIN = 0x00100000, LENGTH = 192K
    SYSRAM (rwx) : ORIGIN = 0x22000000, LENGTH = 64K
    FLASH (rx) : ORIGIN = 0x10000000, LENGTH = 1M
}

/* The data and BSS regions can be placed in TCM or SYSRAM. The code and read-only regions can
   be placed in TCM, SYSRAM, or FLASH. See
   https://docs.microsoft.com/en-us/azure-sphere/app-development/memory-latency for information
   about which types of memory which are available to real-time capable applications on the
   MT3620, and when they should be used. */
REGION_ALIAS("CODE_REGION", TCM);
REGION_ALIAS("RODATA_REGION", TCM);
REGION_ALIAS("DATA_REGION", TCM);
REGION_ALIAS("BSS_REGION", TCM);

ENTRY(ExceptionVectorTable)

SECTIONS
{
    /* The exception vector's virtual address must be aligned to a power of two,
       which is determined by its size and set via CODE_REGION.  See definition of
       ExceptionVectorTable in main.c.

       When the code is run from XIP flash, it must be loaded to virtual address
       0x10000000 and be aligned to a 32-byte offset within the ELF file. */
    .text : ALIGN(32) {
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

    StackTop = ORIGIN(TCM) + LENGTH(TCM);
}
