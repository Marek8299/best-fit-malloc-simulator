/************************************************************************
 * (c) 2021 Marek Klanica, DSA zadanie 1 - simulator pamatovych operacii
 *
 * The above copyright notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 ***********************************************************************/

#include <stddef.h> // NULL
#include <stdbool.h>
#include <stdio.h> // fprintf(), stderr
#include <string.h> // memset
#include "memsim.h"
#include "memsim_internal.h"

void *memory = NULL;

// partition_create zapise hlavicku volnej particie na miesto zadane offsetom
void partition_create(unsigned int offset, int size, unsigned int prev, unsigned int next) {
	PPARTITION_META partition_start = memory + offset;
	partition_start->prev = prev;
	partition_start->next = next;
	partition_start->size = size - sizeof(ALLOCATED_PARTITION_META); // zapis alokovatelnej velkosti
}

// partition_split rozdeli particiu na 2, ak je v danej particii dostatok miesta.
void partition_split(unsigned int src_offset, unsigned int tgt_size) {
	PPARTITION_META src_partition = memory + src_offset;
	if ( tgt_size < PARTITION_DIFF ) tgt_size = PARTITION_DIFF;
	unsigned int req_space = sizeof(ALLOCATED_PARTITION_META) + tgt_size; // novy alokovany header + pozadovane miesto
	if ( req_space < sizeof(PARTITION_META) )
		req_space = sizeof(PARTITION_META); // rozdiel medzi alokovanou a nealokovanou hlavickou particie su 2 inty, takze najmensia mozna velkost particie je hlavicka volnej particie, inak by sa nedala particia spravne uvolnit.

#if defined _ALIGN_TO_EVEN_
	if ( req_space % 2 ) {
		req_space++;
		tgt_size++;
	}
#endif
	
	if(src_partition->size < req_space + PARTITION_DIFF) return; // particia musi mat miesto pre particiu pozadovanej velkosti a novu particiu minimalnej velkosti, inak sa nerozdeli
	partition_create(src_offset + req_space, src_partition->size - tgt_size, src_offset, src_partition->next);
	src_partition->size = tgt_size;
	src_partition->next = src_offset + req_space;
}

// partition_allocate alokuje particiu zmenou hlavicky a oznacenim velkosti negativnym znamienkom. Ak je nastaveny _CLEAR_FREE_HEADER_ON_ALLOCATION_, vynuluje stopy po hlavicke volnej particie.
void partition_allocate(unsigned int tgt_offset) {
	PPARTITION_META tgt_part = memory + tgt_offset;
	if( tgt_part->next ) {
		PARTITION(memory + tgt_part->next)->prev = tgt_part->prev; // ak existuje dalsia neobsadena particia, dostane aktualizovany pointer na predchadzajucu
	}
	if( tgt_part->prev ) {
		PARTITION(memory + tgt_part->prev)->next = tgt_part->next; // predchadzajuca volna particia prevezme pointer na nasledujucu volnu
	}
	else {
		((PMEMORY_META)memory)->first_free = tgt_part->next; // ak predchadzajuca volna neexistuje, nasledujuca je prva volna
	}
	ALLOCATED_PARTITION(tgt_part)->size = INVERT(tgt_part->size); // oznacenie, ze particia je alokovana, ale s pouzitim inej hlavicky
#if defined _CLEAR_FREE_HEADER_ON_ALLOCATION_
	memset ( ((void *)tgt_part) + sizeof(ALLOCATED_PARTITION_META), NONE, PARTITION_DIFF ); // tento krok vvynuluje zvysok hlavicky volnej particie. Na funkcnost programu nema vplyv a je ho mozne zakazat.
#endif
}

// partition_erase zmaze hlavicku particie. Tato akcia nema vplyv na funkcnost programu.
void partition_erase(PPARTITION_META del_tgt) {
#if defined _CLEAR_DELETED_HEADER_
	del_tgt->size = NONE;
	del_tgt->prev = NONE;
	del_tgt->next = NONE;
#else
	return;
#endif // _CLEAR_DELETED_HEADER_
}

// plug_in zapoji uvolnovanu particiu do zoznamu
void plug_in(PPARTITION_META free_part) {
	unsigned int hook_offset = ((PMEMORY_META)memory)->first_free;
	unsigned int free_offset = ((void *)free_part) - memory;
	while(PARTITION(memory + hook_offset)->next < free_offset && PARTITION(memory + hook_offset)->next != NONE) {  // hlada offset na predchadzajucu particiu
		hook_offset = PARTITION(memory + hook_offset)->next;
	}
	if ( hook_offset > free_offset ) {
		free_part->next = hook_offset;
		free_part->prev = PARTITION(memory + hook_offset)->prev;
		PARTITION(memory + hook_offset)->prev = free_offset;
		((PMEMORY_META)memory)->first_free = free_offset;
	} // ak je prva volna particia az za prave zapajanou particiou, uloz zapajanu particiu na zaciatok zoznamu
	else {
		free_part->prev = hook_offset;
		free_part->next = PARTITION(memory + hook_offset)->next;
		PARTITION(memory + free_part->prev)->next = free_offset;
		if ( free_part->next ) PARTITION(memory + free_part->next)->prev = free_offset;
	}
}

// fragmented zisti, ci su particie dane offsetmi vedla seba. 
bool fragmented( unsigned int first_part_off, unsigned int second_part_off ) {
	if ( first_part_off > second_part_off ) {
		unsigned int tmp = first_part_off;
		first_part_off = second_part_off;
		second_part_off = tmp;
	}
	return ( PARTITION(memory + first_part_off)->size - PARTITION_DIFF + sizeof(PARTITION_META) + first_part_off == second_part_off );
}

// defrag zavola na fragmented a spoji susedne particie
void defrag(PPARTITION_META free_part) {
	unsigned int tmp_part_offset;
	if ( free_part->prev && fragmented(free_part->prev, ((void *)free_part) - memory) ) {
		tmp_part_offset = free_part->prev;
		partition_create ( tmp_part_offset, ( PARTITION(memory + tmp_part_offset)->size + (2 * sizeof(ALLOCATED_PARTITION_META)) + free_part->size ), PARTITION(memory + tmp_part_offset)->prev, free_part->next );
		partition_erase( free_part );
		free_part = memory + tmp_part_offset;
	}
	if ( free_part->next && fragmented( ((void *)free_part) - memory, free_part->next ) ) {
		tmp_part_offset = free_part->next;
		partition_create ( ((void *)free_part) - memory, ( PARTITION(memory + tmp_part_offset)->size + (2 * sizeof(ALLOCATED_PARTITION_META)) + free_part->size ), free_part->prev, PARTITION(memory + tmp_part_offset)->next );
		partition_erase( PARTITION(memory + tmp_part_offset) );
	}
}

// findBestFit najde najvhodnejsie umiestnenie v pamati pre pozadovanu velkost
unsigned int findBestFit(unsigned int size) {
	unsigned int part = ((PMEMORY_META)memory)->first_free;
	unsigned int best = NONE;
	if ( part ) {
		if(PARTITION(memory + part) == PARTITION(memory + PARTITION(memory + part)->next)) {
			fprintf(stderr, "Internal partition list corrupted. Re-init the memory and try again.\n");
			return NONE;
		}
		do {
			if ( PARTITION(memory + part)->size >= size && ( PARTITION(memory + part)->size < PARTITION(memory + best)->size || ! best ) )
				best = part;
		} while ( ( part = PARTITION(memory + part)->next ) );
		return best;
	}
	return NONE;
}

void memory_init(void *ptr, unsigned int size) {

#if defined _ALIGN_TO_EVEN_
	if ( size % 2 ) size--; // ak je aktivne zarovnavanie na parny pocet blokov, pamat musi mat parnu velkost
#endif
	
	if( size <= sizeof(MEMORY_META) + sizeof(PARTITION_META) ) return; // init nebude v pripade, ze pamat je mizerne mala
	memory = ptr;
	((PMEMORY_META)memory)->size = size;
	((PMEMORY_META)memory)->first_free = sizeof(MEMORY_META);
	partition_create(((PMEMORY_META)memory)->first_free, size - sizeof(MEMORY_META), NONE, NONE);
}

void * memory_alloc(unsigned int size) {
	if ( memory && size ) {
		unsigned int tgt = findBestFit( size );
		if ( tgt ) {
			partition_split( tgt, size );
			partition_allocate( tgt );
			return ALLOCATED_PARTITION(memory + tgt)->data;
		}
	}
	return NULL;
}

int memory_check(void *ptr) {
	void *header_ptr = ptr - sizeof(ALLOCATED_PARTITION_META);
	if ( memory && ptr && header_ptr >= memory + sizeof(MEMORY_META) && ptr <= memory + ((PMEMORY_META)memory)->size - PARTITION_DIFF ) { // check, ci pointer pointuje do pamate pre particie
		if(ALLOCATED_PARTITION(header_ptr)->size < 0) return VALID;
	}
	return INVALID;
}

int memory_free(void *ptr) {
	if ( ! memory_check ( ptr ) ) return 0;
	PPARTITION_META free_tgt = ptr - sizeof(ALLOCATED_PARTITION_META);
	free_tgt->size = INVERT(ALLOCATED_PARTITION(free_tgt)->size);
	plug_in(free_tgt);
	defrag(free_tgt);
	return 1;
}

