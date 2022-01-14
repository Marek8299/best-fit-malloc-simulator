/************************************************************************
 * (c) 2021 Marek Klanica, DSA zadanie 1 - simulator pamatovych operacii
 *
 * The above copyright notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 ***********************************************************************/

#ifndef MEMSIM_H
#define MEMSIM_H


extern void memory_init(void *ptr, unsigned int size);

extern void *memory_alloc(unsigned int size);

extern int memory_check(void *ptr);

extern int memory_free(void *ptr);

#endif // MEMSIM_H
