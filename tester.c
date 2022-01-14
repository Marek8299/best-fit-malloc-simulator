/************************************************************************
 * (c) 2021 Marek Klanica, DSA zadanie 1 - tester simulatora
 *
 * The above copyright notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 ***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "memsim.h"
#include "memsimdbg.h"
#include "concolors.h"


#define ALLOC_TESTS 100			// maximalny pocet alokacii
#define TEST_MAX_FAIL 3			// tolerovany pocet zlyhani

#define MEMORY_SIZE 103424		// celkova velkost pamate
#define PREFIX 512				// velkost oblasti pre test zapisu mimo spravovanej oblasti

#define TEST_REPEAT_COUNT 1		// pocet opakovani testov



#define NONE 0 // nula, ale ako makro

unsigned int getRandom( unsigned int value_min, unsigned int value_max ) {
	return ( ( rand() % ( value_max + 1 - value_min ) ) + value_min );
}

void randomFree( PALLOCATEDBLOCK_META blkList, unsigned int max_free_tgt, unsigned int max_frees ) {
	unsigned int randnum;
	for ( unsigned int i = NONE; i < max_frees; i++ ) {
		randnum = getRandom(NONE, max_free_tgt);
		memory_free(blkList[randnum].ptr);
		blkList[randnum].size = NONE;
	}
}

void fullFree( PALLOCATEDBLOCK_META blkList ) {
	for ( unsigned int i = NONE; i < ALLOC_TESTS; i++ ) {
		memory_free(blkList[i].ptr);
		blkList[i].size = NONE;
	}
}

void test ( void *region, unsigned int size, unsigned int usable_region_size, unsigned int allocator_min, unsigned int allocator_max, bool test_random_free, unsigned int reporting_lvl ) {
	// reporting_lvl = REPORT_ALL;
	unsigned int randnum = NONE;
	unsigned int failcount = NONE;
	int alloccount;
	ALLOCATEDBLOCK_META allocated_blks[ALLOC_TESTS] = {NONE};
	if( PREFIX + usable_region_size > size ) {
		printf ( ANSI_COLOR_RED "Tester: Wrong memory configuration. Adjust prefix, usable region size, or total memory size and recompile.\n" ANSI_COLOR_WHITE );
		return;
	}
	
	if( ! allocator_min ) {
		printf ( ANSI_COLOR_RED "Tester: Minimum memory size must be grater than 0. Recompile and try again.\n" ANSI_COLOR_WHITE );
		return;
	}
	
	if( failcount >= TEST_MAX_FAIL ) {
		printf ( ANSI_COLOR_RED "Tester: TEST_MAX_FAIL must be grater than 0. Recompile and try again.\n" ANSI_COLOR_WHITE );
		return;
	}
	
	if( allocator_min > allocator_max ) allocator_min = allocator_max;
	
	printf ( "Test info:\n" );
	printf ( (allocator_min == allocator_max) ? "Request size: fixed %u bytes\n" : "Request size: random %u-%u bytes\n", allocator_min, allocator_max );
	printf ( "Memory size: %u bytes\n", usable_region_size );
	
	
	
	MEMSIMDBG_META memsimdbg_metadata = { region, region + PREFIX, usable_region_size, size, reporting_lvl };
	dbg_init ( &memsimdbg_metadata );
	memory_init ( region + PREFIX, usable_region_size );
	
	if ( memsimdbg_metadata.reporting_lvl >= REPORT_ALL ) printf( "\nInit completed, status:\n" );
	checkMRegLimits ( &memsimdbg_metadata );
	printMRegStatus ( &memsimdbg_metadata, 1, true );
	putchar( '\n' );
	
	for ( alloccount = NONE; failcount < TEST_MAX_FAIL && alloccount < ALLOC_TESTS; alloccount++ ) {
		randnum = getRandom(allocator_min, allocator_max);
		if ( ! ( allocated_blks[alloccount].ptr = memory_alloc(randnum) ) ) {
			failcount++;
			alloccount--;
			if ( memsimdbg_metadata.reporting_lvl >= REPORT_ALL ) printf( ANSI_COLOR_YELLOW "%d: " ANSI_COLOR_CYAN "Partition of size %d byte(s) won't fit.\n" ANSI_COLOR_WHITE, failcount, randnum);
		}
		else {
			if ( failcount ) {
				if ( memsimdbg_metadata.reporting_lvl >= REPORT_ALL ) printf ( ANSI_COLOR_YELLOW "Failcount erased.\n" ANSI_COLOR_WHITE );
				failcount = NONE;
			}
			allocated_blks[alloccount].size = randnum;
		}
		if ( alloccount >= NONE && memsimdbg_metadata.reporting_lvl >= REPORT_WARNINGS && ! memory_check ( allocated_blks[alloccount].ptr ) ) printf ( ANSI_COLOR_YELLOW "Memcheck Failed for %p.\n" ANSI_COLOR_WHITE, allocated_blks[alloccount].ptr );
	}
	
	if ( memsimdbg_metadata.reporting_lvl >= REPORT_ALL ) printf( "\nAllocated.\n" );
	
	printSuccessRate ( allocated_blks, alloccount, usable_region_size );
	
	checkMRegLimits ( &memsimdbg_metadata );
	printMRegStatus ( &memsimdbg_metadata, -1, false );
	
	if ( test_random_free ) {
		randomFree( allocated_blks, 20, 10 );
		if ( memsimdbg_metadata.reporting_lvl >= REPORT_ALL ) printf( "\nMemory randomly freed.\n" );
		checkMRegLimits ( &memsimdbg_metadata );
		printMRegStatus ( &memsimdbg_metadata, -1, false );
		putchar( '\n' );
	}
	
	fullFree ( allocated_blks );
	
	if ( memsimdbg_metadata.reporting_lvl >= REPORT_ALL ) printf( "\nMemory fully freed.\n" );
	checkMRegLimits ( &memsimdbg_metadata );
	printMRegStatus ( &memsimdbg_metadata, 1, true );
}


int main() {
	char region[MEMORY_SIZE];
	
	srand(time(0));
	
	for (unsigned int i = 0; i < TEST_REPEAT_COUNT; i++){
		
		// oblast pre testy, yes it's fckin trash but no one got time 4 this
		
		printf ( ANSI_COLOR_BLUE "Test 1.i\n" ANSI_COLOR_WHITE );
		test ( region, MEMORY_SIZE, 50, 1, 8, true, REPORT_WARNINGS );
		test ( region, MEMORY_SIZE, 100, 8, 8, true, REPORT_WARNINGS );
		test ( region, MEMORY_SIZE, 200, 8, 8, true, REPORT_WARNINGS );
		printf ( ANSI_COLOR_BLUE "Test 1.ii\n" ANSI_COLOR_WHITE );
		test ( region, MEMORY_SIZE, 50, 15, 15, true, REPORT_WARNINGS );
		test ( region, MEMORY_SIZE, 100, 15, 15, true, REPORT_WARNINGS );
		test ( region, MEMORY_SIZE, 200, 15, 15, true, REPORT_WARNINGS );
		printf ( ANSI_COLOR_BLUE "Test 1.iii\n" ANSI_COLOR_WHITE );
		test ( region, MEMORY_SIZE, 50, 24, 24, true, REPORT_WARNINGS );
		test ( region, MEMORY_SIZE, 100, 24, 24, true, REPORT_WARNINGS );
		test ( region, MEMORY_SIZE, 200, 24, 24, true, REPORT_WARNINGS );
		printf ( ANSI_COLOR_BLUE "Test 2\n" ANSI_COLOR_WHITE );
		test ( region, MEMORY_SIZE, 50, 8, 24, true, REPORT_WARNINGS );
		test ( region, MEMORY_SIZE, 100, 8, 24, true, REPORT_WARNINGS );
		test ( region, MEMORY_SIZE, 200, 8, 24, true, REPORT_WARNINGS );
		printf ( ANSI_COLOR_BLUE "Test 3\n" ANSI_COLOR_WHITE );
		test ( region, MEMORY_SIZE, 10000, 500, 5000, true, REPORT_WARNINGS );
		printf ( ANSI_COLOR_BLUE "Test 4\n" ANSI_COLOR_WHITE );
		test ( region, MEMORY_SIZE, 100000, 8, 50000, true, REPORT_WARNINGS );
		
	}
	
	return 0;
}



