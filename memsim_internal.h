/************************************************************************
 * (c) 2021 Marek Klanica, DSA zadanie 1 - simulator pamatovych operacii
 *
 * The above copyright notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 ***********************************************************************/

#ifndef MEMSIM_INTERNAL_H
#define MEMSIM_INTERNAL_H

#define _ALIGN_TO_EVEN_ // zarovnanie velkosti alokovanych particii na parne hodnoty
#define _CLEAR_FREE_HEADER_ON_ALLOCATION_ // vycistenie zvysku volnej hlavicky po alokovani particie
#define _CLEAR_DELETED_HEADER_ // vycistenie hlavicky stratenej particii pri spajani particii

#define NONE 0

#define VALID 1
#define INVALID 0

typedef struct _MEMORY_META {
	unsigned int size;			// celkova velkost pamate
	unsigned int first_free;	// offset prveho volneho bloku
} MEMORY_META, *PMEMORY_META;

typedef struct _PARTITION_META {
	unsigned int prev;	// offset predchadzajuceho bloku
	unsigned int next;	// offset dalsieho bloku
	int size;			// dostupna velkost bloku
} PARTITION_META, *PPARTITION_META;

typedef struct _ALLOCATED_PARTITION_META {
	int size;			// alokovana velkost (negativna hodnota)
	char data[];		// alokovana oblast
} ALLOCATED_PARTITION_META, *PALLOCATED_PARTITION_META;

#define PARTITION(x) ((PPARTITION_META)(x))
#define ALLOCATED_PARTITION(x) ((PALLOCATED_PARTITION_META)(x))
#define INVERT(x) ((x) * (-1))

#define PARTITION_DIFF (sizeof(PARTITION_META) - sizeof(ALLOCATED_PARTITION_META))

#endif // MEMSIM_INTERNAL_H
