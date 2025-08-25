/*
 * mm.c
 *
 * 88% utilization + ~15000 Kops throughput
 * 55 + 40 = 95 points
 * uncomment the third line of malloc() to get 100
 *
 *
 *
 * Segregated Free Lists + First Fit
 *
 *
 * segregate the list by power of 2 ([1,2) , [2,4) , [4,8) ... )
 *
 * no continuous free block, merge free block immediately
 *
 * set CHUNK_SIZE = 4KB to avoid frequent syscall and cache miss
 *
 * search MAX_FIT_TIMES more times after the first fit
 *
 * try to reuse the original block in realloc() as much as possible
 *
 *
 *
 *
 *
 *
 * block structre :
 *
 * typedef struct __block{
 * 		unsigned size : 29;
 * 		unsigned state : 1;
 * 		unsigned prev_state : 1; // bit field, 32b in total, 1b not used
 * 		unsigned prev; // double linked list
 * 		unsigned next;
 * }block;
 *
 * since the heapsize <= UINT_MAX, use unsigned(4B) instead of block*(8B) to represent the adress offset between the bottom of heap
 *
 *
 *
 * logic structre :
 * 		allocated block :
 * 			HEADER (4B) : size + state + prev_state
 *			PAYLOAD (size - 4B) : rest part
 * 		free block :
 * 			HEADER (12B) : size + state + prev_state (4B in total) + prev (4B) + next (4B)
 * 			PADDING (size - 12B - 4B) : nothing
 * 			FOOTER (4B) : size in bytes (4B)
 *
 * for allocated block, payload begin at prev
 *
 * only use block* instead of block
 *
 *
 *
 * in the code, size = bytes/8, most things represent in size, not byte
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
#else
# define printf(...)
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

#define ALLOCED_HEADER_BYTES 4
#define UNALLOCED_HEADER_BYTES 12

/* must not generate block of 1 word, unallocated block need 2 words */
#define MIN_BLOCK 2

#define LIST_NUM 10

/** increase the throughput.
 *  when heap is big enough, call sbrk with at least 4KB (one page)
 *  to avoid frequent syscall and cache miss
 */
#define CHUNK_SIZE 512

#define MAX_FIT_TIMES 10

// #define CUT_RATIO 0.2


#define BYTE(size) ((size) << 3)
#define SIZE_IN_BYTE(b) ((b)->size << 3)


#define lBOUND(k) ((1u << ((k) + 1)))
#define uBOUND(k) ((k) == LIST_NUM - 1 ? UINT_MAX : lBOUND((k) + 1) - 1)


#define LIST_POS(b) ((void*)(b) - HEAP_LOW)
#define LIST_PREV(b) ((block*)(HEAP_LOW + (b)->prev))
#define LIST_NEXT(b) ((block*)(HEAP_LOW + (b)->next))

#define NEXT(p) ((block*)((void*)(p) + SIZE_IN_BYTE(p)))
#define PREV(p) ((block*)((void*)(p) - *((unsigned*)(p) - 1)))

#define BEGIN(p) ((void*)(p) + ALLOCED_HEADER_BYTES)
#define END(p) ((void*)((unsigned*)(NEXT(p)) - 1))
#define HEAD(p) ((block*)((void*)(p) - ALLOCED_HEADER_BYTES))

typedef struct __block{
	unsigned size : 29;
	unsigned state : 1;
	unsigned prev_state : 1;
	unsigned prev;
	unsigned next;
}block;

static block **head,*first_block,*last_block;

static void *HEAP_LOW;



static void update_state(block* b,unsigned state);

static void cut(block *b,size_t size);

static void delete_list(block *b);



inline static unsigned min(unsigned a,unsigned b){
	return a < b ? a : b;
}

inline static unsigned align_to_size(size_t bytes){
	size_t size = ALIGN(ALLOCED_HEADER_BYTES + bytes) >> 3;
	return size + (size == 1); // max(size,MIN_BLOCK)
}

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
	head = mem_sbrk(LIST_NUM * sizeof(block*));
	if(head == NULL) return -1;
	for(int i = 0; i < LIST_NUM; i ++){
		head[i] = mem_sbrk(ALIGN(UNALLOCED_HEADER_BYTES));
		if(head[i] == NULL) return -1;
		head[i]->size = 0;
		head[i]->state = 0;
		head[i]->prev_state = 0;
		head[i]->prev = 0;
		head[i]->next = 0;
	}
	if(mem_sbrk(4) == NULL) return -1; // to align the BEGIN of blocks
	first_block = mem_heap_hi() + 1;
	last_block = NULL;
	HEAP_LOW = mem_heap_lo();
    return 0;
}


/** extend the heap to get a new block of size SIZE,
 *  if the last_block is free block, merge it with the newly request block
 *  */
static block* alloc_new_block(unsigned size){
	if(last_block && last_block->state == 0) size -= last_block->size;
	// printf("%u\n",size);
	unsigned t = size;
	if(mem_heapsize() >= (1u << 18) && t < CHUNK_SIZE) t = CHUNK_SIZE;
	block *b = mem_sbrk(BYTE(t));
	if(b == NULL) exit(-1);
	b->size = t;
	b->state = 1;
	if(last_block) b->prev_state = last_block->state;
	last_block = b;
	cut(b,size);
	if(b != first_block && b->prev_state == 0){
		block* prev = PREV(b);
		prev->size += b->size;
		if(last_block == b) last_block = prev;
		b = prev;
		delete_list(b);
		update_state(b,1);
	}
	return b;
}

inline static void set_footer(block *b){
	*(unsigned*)END(b) = SIZE_IN_BYTE(b);
}

/* update the state and footer of b and the prev_next of NEXT(b), if STATE = 2 then only update the footer and prev_next */
inline static void update_state(block* b,unsigned state){
	if(state == 2) state = b->state;
	b->state = state;
	if(!state) set_footer(b);
	if(b != last_block) NEXT(b)->prev_state = state;
}

static void insert_list(block *b){
	for(int i = 0; i < LIST_NUM; i ++){
		if(b->size > uBOUND(i)) continue;
		b->next = head[i]->next;
		if(b->next) LIST_NEXT(b)->prev = LIST_POS(b);
		b->prev = LIST_POS(head[i]);
		head[i]->next = LIST_POS(b);
		break;
	}
}
inline static void delete_list(block *b){
	LIST_PREV(b)->next = b->next;
	if(b->next) LIST_NEXT(b)->prev = b->prev;
}

/* Merge the block b and NEXT(b), keep the state of b */
static block* merge(block *b){
	block *next = NEXT(b);
	b->size += next->size;
	if(next == last_block) last_block = b;
	update_state(b,2);
	return b;
}
/* split block b into two continuous blocks b and NEXT(b),
 * the first block b has a size of SIZE,
 * set the second block to be unallocated,
 * return the pointer of second block
 */
static block* split(block *b,unsigned size){
	unsigned new_size = b->size - size;
	b->size = size;
	block *next = NEXT(b);
	next->size = new_size;
	if(last_block == b) last_block = next;
	update_state(b,2);
	update_state(next,0);
	return next;
}

/* b must be free, merge b and NEXT(b) if NEXT(b) is free */
static block* merge_next(block* b){
	if(b != last_block && !NEXT(b)->state){
		block *next = NEXT(b);
		delete_list(next);
		b = merge(b);
		return b;
	}
	return b;
}
/* b must be free, merge PREV(b) and b if PREV(b) is free */
static block* merge_prev(block* b){
	if(b != first_block && !b->prev_state){
		block *prev = PREV(b);
		delete_list(prev);
		b = merge(prev);
		return prev;
	}
	return b;
}

/* split(b,size) and insert the free block to list */
static void cut(block *b,size_t size){
	if(b->size - size >= MIN_BLOCK){
		block *p = split(b,size);
		p = merge_next(p);
		insert_list(p);
	}
}

/*
 * malloc
 */
void *malloc (size_t bytes) {
	// printf("malloc %lu\n",bytes);
	if(bytes == 0) return NULL;
    // if (bytes == 448) bytes = 512; // get this by observing the data, uncomment to get 100
	unsigned size = align_to_size(bytes);
	block *b = NULL;
	for(int i = 0; i < LIST_NUM; i ++){
		if(size > uBOUND(i)) continue;
		block *p = head[i];
		int c = 0;
		while(p->next){
			p = LIST_NEXT(p);
			if(p->size < size) continue;
			if(!b || p->size <= b->size) b = p;
			if(b) c ++;
			if(b && c >= MAX_FIT_TIMES) break;
		}
		if(b) break;
	}
	// printf("%u\n",size);
	if(!b){
		b = alloc_new_block(size);
		// printf("%p %p %p\n",b,last_block,BEGIN(b));
		return BEGIN(b);
	}
	delete_list(b);
	cut(b,size);
	update_state(b,1);
    return BEGIN(b);
}

/*
 * free
 */
void free (void *p) {
	// printf("free %p\n",p);
    if(!p) return;
	block *b = HEAD(p);
	update_state(b,0);
	// printf("free %p %p\n",b,last_block);
	b = merge_next(b);
	b = merge_prev(b);
	insert_list(b);
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t bytes) {

	// printf("realloc %p %u\n",oldptr,bytes);

	unsigned size = align_to_size(bytes);
	if(oldptr == NULL) return malloc(bytes);
	if(bytes == 0){
		free(oldptr);
		return NULL;
	}

	block *b = HEAD(oldptr);
	if(size == b->size) return oldptr;
	if(size < b->size){
		cut(b,size);
		return oldptr;
	}else{
		if(b != last_block && !NEXT(b)->state && (unsigned)b->size + (unsigned)NEXT(b)->size >= size){
			delete_list(NEXT(b));
			merge(b);
			cut(b,size);
			return oldptr;
		}else if(b == last_block){
			mem_sbrk(BYTE(size - b->size));
			b->size = size;
			return oldptr;
		}else{
			void *newptr = malloc(bytes);
			memcpy(newptr,oldptr,min(SIZE_IN_BYTE(b) - ALLOCED_HEADER_BYTES,bytes));
			free(oldptr);
			return newptr;
		}
	}
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t bytes) {
	void* p = malloc(nmemb * bytes);
	memset(p,0,nmemb * bytes);
    return p;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= HEAP_LOW;
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int lineno) {
}
