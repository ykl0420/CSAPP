/*
 * mm.c
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
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
// #define DEBUG
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

#define BLOCK_NUM 10

#define HEADER_BYTES 20

#define MIN_BLOCK 3


#define BYTE(size) ((size) << 3)
#define SIZE_IN_BYTE(b) ((b)->size << 3)


#define lBOUND(k) ((1u << ((k) + 1)))
#define uBOUND(k) ((k) == 9 ? UINT_MAX : lBOUND((k) + 1) - 1)


#define NEXT(p) ((block*)((void*)(p) + SIZE_IN_BYTE(p)))
#define PREV(p) ((block*)((void*)(p) - *((int*)(p) - 1)))

#define BEGIN(p) ((void*)(p) + HEADER_BYTES)
#define END(p) ((void*)((int*)(NEXT(p)) - 1))
#define HEAD(p) ((block*)((void*)(p) - HEADER_BYTES))

typedef struct __block{
	struct __block *prev;
	struct __block *next;
	unsigned size : 29;
	unsigned state : 1;
	unsigned prev_state : 1;
}block;

block **head,*first_block,*last_block;

static unsigned min(unsigned a,unsigned b){
	return a < b ? a : b;
}

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
	head = mem_sbrk(BLOCK_NUM * sizeof(block*));
	if(head == NULL) return -1;
	for(int i = 0; i < BLOCK_NUM; i ++){
		head[i] = mem_sbrk(ALIGN(HEADER_BYTES));
		head[i]->size = 0;
		head[i]->state = 0;
		head[i]->prev_state = 0;
		head[i]->prev = NULL;
		head[i]->next = NULL;
	}
	mem_sbrk(4);
	first_block = mem_heap_hi() + 1;
    return 0;
}

static block* alloc_new_block(unsigned size){
	block *b = mem_sbrk(BYTE(size));
	b->size = size;
	b->state = 1;
	if(b != first_block) b->prev_state = last_block->state;
	last_block = b;
	return b;
}

static void set_footer(block *b){
	*(int*)END(b) = SIZE_IN_BYTE(b);
}

static void set_alloc(block *b){
	b->state = 1;
	if(b != last_block) NEXT(b)->prev_state = 1;
}
static void set_unalloc(block* b){
	b->state = 0;
	set_footer(b);
	if(b != last_block) NEXT(b)->prev_state = 0;
}

static void insert_list(block *b){
	for(int i = 0; i <= 9; i ++){
		if(b->size > uBOUND(i)) continue;
		b->next = head[i]->next;
		if(b->next) b->next->prev = b;
		b->prev = head[i];
		head[i]->next = b;
		break;
	}
}
static void delete_list(block *b){
	b->prev->next = b->next;
	if(b->next) b->next->prev = b->prev;
}

/* Merge the block b and NEXT(b), keep the state of b */
static block* merge(block *b){
	block *next = NEXT(b);
	b->size += next->size;
	if(next == last_block) last_block = b;
	if(!b->state) set_footer(b);
	if(b != last_block) NEXT(b)->prev_state = b->state;
	return b;
}
/* split block b into two continuous blocks b and NEXT(b),
 * the first block b has a size of SIZE,
 * set the second block to be unallocated,
 * return the pointer of second block
 */
static block* split(block *b,unsigned size){
	int new_size = b->size - size;
	b->size = size;
	block *next = NEXT(b);
	next->size = new_size;
	next->state = 0;
	next->prev_state = b->state;
	set_footer(next);
	if(last_block == b) last_block = next;
	return next;
}

/*
 * malloc
 */
void *malloc (size_t bytes) {
	printf("malloc\n");
	if(bytes == 0) return NULL;
	size_t size = ALIGN(HEADER_BYTES + bytes) >> 3;
	block *b = NULL;
	for(int i = 0; i <= 9; i ++){
		if(size > uBOUND(i)) continue;
		block *p = head[i];
		int c = 0;
		while(p->next){
			p = p->next;
			if(p->size < size) continue;
			if(!b || p->size <= b->size) b = p;
			c ++;
			if(b && c >= 10) break;
		}
		if(b) break;
	}
	printf("%u\n",size);
	if(!b){
		b = alloc_new_block(size);
		printf("%p %p %p\n",b,last_block,BEGIN(b));
		return BEGIN(b);
	}
	delete_list(b);
	if(b->size - size >= MIN_BLOCK){
		block *p = split(b,size);
		insert_list(p);
	}
	set_alloc(b);
    return BEGIN(b);
}

/*
 * free
 */
void free (void *p) {
    if(!p) return;
	block *b = HEAD(p);
	set_unalloc(b);
	printf("free %p %p\n",b,last_block);
	if(b != last_block && !NEXT(b)->state){
		// printf("NMSL1\n");
		block *next = NEXT(b);
		delete_list(next);
		b = merge(b);
	}
	if(b != first_block && !b->prev_state){
		// printf("NMSL2\n");
		block *prev = PREV(b);
		delete_list(prev);
		b = merge(prev);
	}
	insert_list(b);
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t bytes) {
	int size = ALIGN(HEADER_BYTES + bytes) >> 3;

	printf("realloc %p %lu\n",oldptr,bytes);

	if(oldptr == NULL) return malloc(bytes);
	if(bytes == 0){
		free(oldptr);
		return NULL;
	}

	block *b = HEAD(oldptr);
	if(size == b->size) return oldptr;
	if(size < b->size){
		if(b->size - size >= MIN_BLOCK){
			block *new_block = split(b,size);
			insert_list(new_block);
		}
		return oldptr;
	}else{
		printf("HHH\n");
		if(b != last_block && !NEXT(b)->state && b->size + NEXT(b)->size >= size){
			delete_list(NEXT(b));
			merge(b);
			if(b->size - size >= MIN_BLOCK){
				block *p = split(b,size);
				insert_list(p);
			}
			return oldptr;
		}else if(b == last_block){
			mem_sbrk(BYTE(size - b->size));
			b->size = size;
			return oldptr;
		}else{
			void *newptr = malloc(bytes);
			memcpy(newptr,oldptr,min(SIZE_IN_BYTE(b) - HEADER_BYTES,bytes));
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
    return p <= mem_heap_hi() && p >= mem_heap_lo();
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
