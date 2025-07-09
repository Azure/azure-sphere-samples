/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "main.h"

// Persistent storage takes up two 128-byte pages of flash at 0x0800_4000, which is
// 16KB after the start of flash. If the application code extends into this page, the
// data page must be moved.
//
// The first page contains the magic words <"MSAS", "SODA">, followed by a sequence of
// <stocked, issued> pairs.
//
// When the application starts, it reads the first two words of the first page.  If they
// are not contain the magic values, it erases both pages, which sets all bytes to 0x00.
// The application then writes the magic words to the start of the first page.
//
// When the application stores a new <stocked, issued> pair, it overwrites the first pair
// of <0x00000000, 0x00000000> words that it finds. If all pages are full, it erases them,
// re-writes the magic header, followed by the state, to the start of the first page.
//
// This is intentionally simple.  In a full application, the number of erase cycles could
// be reduced by only writing the machine state when it powers down.

static void EraseAllPages(void);
static void ReadStateFromExistingPage(void);

#define DATA_AREA_ADDR		(FLASH_BASE + (128 * FLASH_PAGE_SIZE))
#define DATA_AREA_LENGTH	(2 * FLASH_PAGE_SIZE)
#define DATA_AREA_END		(DATA_AREA_ADDR + DATA_AREA_LENGTH)

static const uint32_t DATA_AREA_SECTORS = OB_WRP_Pages128to159;

static const uint32_t MAGIC_WORD_0 = ('M' << 24) | ('S' << 16) | ('A' << 8) | 'S';
static const uint32_t MAGIC_WORD_1 = ('S' << 24) | ('O' << 16) | ('D' << 8) | 'A';
static const uint32_t MAGIC_HEADER_SIZE = 2 * sizeof(uint32_t);

// { uint32_t stocked; uint32_t issued; }
static const uint32_t DATA_ENTRY_SIZE = 2 * sizeof(uint32_t);

// Address in flash where last valid entry was written.
static uint32_t lastEntryAddr;

void RestoreStateFromFlash(void)
{
	HAL_FLASH_Unlock();
	HAL_FLASH_OB_Unlock();

	FLASH_OBProgramInitTypeDef ob = { };
	HAL_FLASHEx_OBGetConfig(&ob);

	// Ensure page is not write-protected.
	// (If it is, write-protection can be disabled with HAL_FLASHEx_OBProgram.)
	if ((ob.WRPSector & DATA_AREA_SECTORS) == DATA_AREA_SECTORS) {
		Error_Handler();
	}

	uint32_t magic0 = *(__IO uint32_t*) DATA_AREA_ADDR;
	uint32_t magic1 = *(__IO uint32_t*) (DATA_AREA_ADDR + sizeof(uint32_t));

	// If this is the first time that the device has been used, erase the data page.
	bool hasBeenFormatted = (magic0 == MAGIC_WORD_0 && magic1 == MAGIC_WORD_1);
	if (! hasBeenFormatted) {
		EraseAllPages();
	} else {
		ReadStateFromExistingPage();
	}
}

// Erase all pages which are used to store the machine state and write
// the magic header followed by the current machine state to the start
// of the first page.
static void EraseAllPages(void)
{
	FLASH_EraseInitTypeDef ei = { };
	ei.TypeErase = FLASH_TYPEERASE_PAGES;
	ei.PageAddress = DATA_AREA_ADDR;
	ei.NbPages = DATA_AREA_LENGTH / FLASH_PAGE_SIZE;

	uint32_t pageError;

	if (HAL_FLASHEx_Erase(&ei, &pageError) != HAL_OK) {
		Error_Handler();
	}

	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, DATA_AREA_ADDR + 0 * sizeof(uint32_t), MAGIC_WORD_0);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, DATA_AREA_ADDR + 1 * sizeof(uint32_t), MAGIC_WORD_1);

	lastEntryAddr = DATA_AREA_ADDR + MAGIC_HEADER_SIZE;
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, lastEntryAddr, ~state.stockedDispenses);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, lastEntryAddr + sizeof(uint32_t), ~state.issuedDispenses);
}

// Populate the global state variable with the most recently-written data.
// This assumes the data pages have already been initialized with EraseAllPages.
static void ReadStateFromExistingPage(void)
{
	// Find the most recently-written data entry.
	uint32_t searchAddr = DATA_AREA_ADDR + MAGIC_HEADER_SIZE;
	while (searchAddr < DATA_AREA_END) {
		uint32_t searchStockCmpl = *(__IO uint32_t*) searchAddr;
		if (searchStockCmpl == 0x0) {
			break;
		}
		searchAddr += DATA_ENTRY_SIZE;
	}

	lastEntryAddr = searchAddr - DATA_ENTRY_SIZE;
	uint32_t stockCmpl = *(__IO uint32_t*) lastEntryAddr;
	state.stockedDispenses = ~stockCmpl;
	uint32_t dispCmpl = *(__IO uint32_t*) (lastEntryAddr + sizeof(uint32_t));
	state.issuedDispenses = ~dispCmpl;
}

// Append the current machine state to the flash memory. If the flash memory has been
// exhausted, erase it and write the state to the start of the first page.
void WriteLatestMachineState(void)
{
	lastEntryAddr += DATA_ENTRY_SIZE;

	if (lastEntryAddr >= DATA_AREA_END) {
		EraseAllPages();
	} else {
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, lastEntryAddr, ~state.stockedDispenses);
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, lastEntryAddr + sizeof(uint32_t), ~state.issuedDispenses);
	}
}


