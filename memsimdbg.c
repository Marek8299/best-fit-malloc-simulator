/************************************************************************
 * (c) 2021 Marek Klanica, DSA zadanie 1 - debugger simulatora
 *
 * The above copyright notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 ***********************************************************************/

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "memsim.h"
#include "memsim_internal.h"
#include "memsimdbg.h"
#include "concolors.h"

#define SUCCESS 0
#define ERROR -1


typedef struct _MREG_STATUS {
	unsigned int mreg_size;
	unsigned int free_part_count;
	unsigned int free_part_minsize;
	unsigned int free_part_maxsize;
	unsigned int free_mem_total;
} MREG_STATUS, *PMREG_STATUS;

void checkForZeros ( char *start, char *end, bool reverse_offset, bool *invalid_write ) {
	if ( invalid_write ) *invalid_write = false;
	for ( char *ptr = start; ptr < end; ptr++ ) {
		if( *ptr != NONE ) {
			printf( ANSI_COLOR_RED "ERROR: Detected write outside of managed area, offset: %ld\n" ANSI_COLOR_WHITE , (reverse_offset) ? (ptr - end) : (ptr - start) );
			if ( invalid_write ) *invalid_write = true;
		}
	}
}

int updateMRegStatus( PMEMSIMDBG_META memsimdbg_metadata, PMREG_STATUS status ) {
	status->mreg_size = ((PMEMORY_META)memsimdbg_metadata->managed_region)->size;
	if ( status->mreg_size != memsimdbg_metadata->managed_region_size ) {
		if( memsimdbg_metadata->reporting_lvl >= REPORT_ERRORS ) printf ( ANSI_COLOR_RED "FATAL: Incorrect managed region size: defined %d, region header value: %d\n" ANSI_COLOR_WHITE, memsimdbg_metadata->managed_region_size, status->mreg_size );
		return ERROR;
	}		
	PPARTITION_META partition = ( ((PMEMORY_META)memsimdbg_metadata->managed_region)->first_free ) ? PARTITION(memsimdbg_metadata->managed_region + ((PMEMORY_META)memsimdbg_metadata->managed_region)->first_free) : NULL;
	PPARTITION_META partition_bak = partition;
	if ( partition && partition == PARTITION(memsimdbg_metadata->managed_region + partition->next) ) {
		if( memsimdbg_metadata->reporting_lvl >= REPORT_ERRORS ) printf( ANSI_COLOR_RED "FATAL: Partition list corruption detected. Re-init the memory and try again.\n" ANSI_COLOR_WHITE );
		return ERROR;
	}
	while ( partition ) {
		status->free_part_count++;
		status->free_mem_total += partition->size;
		if ( ! status->free_part_minsize || status->free_part_minsize > partition->size ) status->free_part_minsize = partition->size;
		if ( status->free_part_maxsize < partition->size ) status->free_part_maxsize = partition->size;
		partition = ( partition->next ) ? PARTITION(memsimdbg_metadata->managed_region + partition->next) : NULL;
		if( partition_bak == partition && memsimdbg_metadata->reporting_lvl >= REPORT_ERRORS ) {
			printf( ANSI_COLOR_RED "FATAL: Partition linst is a loop.\n" ANSI_COLOR_WHITE );
			return ERROR;
		}
	}
	return SUCCESS;
}

void dbg_init ( PMEMSIMDBG_META memsimdbg_metadata ) {
#if defined _ALIGN_TO_EVEN_
	unsigned int mreg_size_old = memsimdbg_metadata->managed_region_size;
	if ( memsimdbg_metadata->managed_region_size % 2 ) memsimdbg_metadata->managed_region_size--; // ak je aktivne zarovnavanie na parny pocet blokov, pamat musi mat parnu velkost
	printf ( "Memory aligned to even. Managed size: %d of %d provided\n", memsimdbg_metadata->managed_region_size, mreg_size_old );
#endif
	if ( memsimdbg_metadata && memsimdbg_metadata->region ) memset( memsimdbg_metadata->region, NONE, memsimdbg_metadata->region_size );
}

void checkMRegLimits ( PMEMSIMDBG_META memsimdbg_metadata ) {
	bool iwdetector;
	checkForZeros ( memsimdbg_metadata->region, memsimdbg_metadata->managed_region, true, &iwdetector );
	if ( memsimdbg_metadata->reporting_lvl >= REPORT_ALL && ! iwdetector ) printf ( ANSI_COLOR_GREEN "No invalid write(s) detected before managed region.\n" ANSI_COLOR_WHITE );
	checkForZeros ( memsimdbg_metadata->managed_region + memsimdbg_metadata->managed_region_size, memsimdbg_metadata->region + memsimdbg_metadata->region_size, false, &iwdetector );
	if ( memsimdbg_metadata->reporting_lvl >= REPORT_ALL && ! iwdetector ) printf ( ANSI_COLOR_GREEN "No invalid write(s) detected after managed region.\n" ANSI_COLOR_WHITE );
}

void printMRegStatus ( PMEMSIMDBG_META memsimdbg_metadata, int expected_free_parts, bool expected_full_mem ) {
	MREG_STATUS status = {NONE};
	if ( updateMRegStatus ( memsimdbg_metadata, &status ) == SUCCESS ) {
		if ( memsimdbg_metadata->reporting_lvl >= REPORT_ALL ) {
			printf ( "Managed region starts at %p, with size of %d byte(s).\n", memsimdbg_metadata->managed_region, status.mreg_size );
			printf ( "%d partition(s) free in %d byte(s) total with minimum size of %d byte(s) and maximum of %d byte(s).\n", status.free_part_count, status.free_mem_total, status.free_part_minsize, status.free_part_maxsize );
		}
		if ( memsimdbg_metadata->reporting_lvl >= REPORT_WARNINGS ) 
			if ( expected_free_parts >= NONE && expected_free_parts != status.free_part_count ) printf ( ANSI_COLOR_YELLOW "WARNING: free partition count doesn't match the expected value. free: %d, expected: %d\n" ANSI_COLOR_WHITE, status.free_part_count, expected_free_parts );
			if ( expected_full_mem ) {
				unsigned int expected_available_size = memsimdbg_metadata->managed_region_size - sizeof(MEMORY_META) - sizeof(PARTITION_META) + PARTITION_DIFF;
				if ( expected_available_size != status.free_mem_total ) printf ( ANSI_COLOR_YELLOW "WARNING: %d byte(s) of memory forever lost.\n" ANSI_COLOR_WHITE, expected_available_size - status.free_mem_total );
			}
	}
}

void printSuccessRate ( PALLOCATEDBLOCK_META blkInfo, unsigned int alloccount, unsigned int mem_size ) {
	unsigned int total_mem_requested = NONE;
	unsigned int total_mem_allocated = NONE;
	double success_rate = NONE;
	for ( unsigned int i = NONE; i < alloccount; i++ ) {
		total_mem_requested += blkInfo[i].size;
		total_mem_allocated += INVERT(ALLOCATED_PARTITION(((void *)blkInfo[i].ptr) - sizeof(ALLOCATED_PARTITION_META))->size);
	}
	if( total_mem_allocated ) {
		success_rate = ((double)total_mem_requested / (double)total_mem_allocated) * 100;
		printf ( "Requested %u partition(s) in %u byte(s). Allocated: %u byte(s).\n%.2lf%% (%u byte(s)) of allocated memory wasted in internal fragmentation.\n", alloccount, total_mem_requested, total_mem_allocated, 100 - success_rate, total_mem_allocated - total_mem_requested );
		printf ( "%.2lf%% of memory requested, %.2lf%% of memory allocated. Total memory: %u bytes.\n", ((double)total_mem_requested / (double)mem_size) * 100, ((double)total_mem_allocated / (double)mem_size) * 100, mem_size );
	}
}

