/************************************************************************
 * (c) 2021 Marek Klanica, DSA zadanie 1 - debugger simulatora
 *
 * The above copyright notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 ***********************************************************************/

#ifndef MEMSIMDBG_H
#define MEMSIMDBG_H

#define REPORT_NONE 0
#define REPORT_ERRORS 1
#define REPORT_WARNINGS 2
#define REPORT_ALL 3

typedef struct _MEMSIMDBG_META {
	void *region;
	void *managed_region;
	unsigned int managed_region_size;
	unsigned int region_size;
	unsigned int reporting_lvl;
} MEMSIMDBG_META, *PMEMSIMDBG_META;

typedef struct _ALLOCATEDBLOCK_META {
	void * ptr;
	unsigned int size;
} ALLOCATEDBLOCK_META, *PALLOCATEDBLOCK_META;

extern void dbg_init ( PMEMSIMDBG_META memsimdbg_metadata );

extern void checkMRegLimits ( PMEMSIMDBG_META memsimdbg_metadata );

extern void printMRegStatus ( PMEMSIMDBG_META memsimdbg_metadata, int expected_free_parts, bool expected_full_mem );

extern void printSuccessRate ( PALLOCATEDBLOCK_META blkInfo, unsigned int alloccount, unsigned int mem_size );

#endif // MEMSIMDBG_H
