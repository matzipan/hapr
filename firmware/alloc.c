#ifndef _HAPR_ALLOC
#define _HAPR_ALLOC

#include "math.h"
#include "stdio.h"
#include "stdlib.h"

#include "alloc.h"
#include "serial.h"
#include "queue.h"

// Originally we made use of malloc, but with no ability to control how memory
// was being used and without the aforementioned upper 32KB of RAM, we ran into
// issues with it unpredictably returning NULL due to insufficient contiguous
// memory.

// These allocation routines are for various structs (filters, queues) and sample
// buffers. We store some samples in the RAM normally reserved for Ethernet and USB,
// as neither peripheral is used in this solution. We presently can store over 22000
// samples at any given time, versus only 6000 or so using the previous solution.

// The buffer allocation functions for samples still return only contiguous memory,
// however in even experimental usage we have almost never found this an issue. Since
// the system recreates all filters when the graph is changed, this is impossible to
// encounter when using even 100 of the currently used filters (which CPU constraints
// would make impossible). Issues like wasting memory when less than BUF_BLOCK_LENGTH
// samples are wanted in a buffer also would need addressing if memory issues became
// a constraint again.

// 2014-02-22 Created by Michael Mokrysz to allocate sample buffers YYYY-MM-DD
// 2014-02-25 Modified by Michael Mokrysz to store extra samples in Ethernet/USB reserved 32KB of RAM
// 2014-03-04 Modified by Alex Oyston & Andrei Zisu to address duplicate-allocation issues
// 2014-03-04 Modified by Michael Mokrysz in major refactoring and bug elimination
// 2014-03-06 Modified by Michael Mokrysz to allocate various structs

#define FILTER_ALLOC_SIZE 100 // How many filter structs to allocate.
#define QUEUE_ALLOC_SIZE 100 // How many queue structs to allocate.
#define QUEUE_ELEMENT_ALLOC_SIZE 100 // How many queue_element structs to allocate.
#define BUF_BLOCK_LENGTH (1<<7) // How many samples in one allocation block.
#define BUF_BLOCK_SIZE (BUF_BLOCK_LENGTH * sizeof(uint16_t)) // Size of a sample allocation block in bytes.
#define BUF1_LENGTH (10240-5608) // Size of first sample array, must be multiple of BUF_BLOCK_LENGTH.
#define BUF2_LENGTH (1<<14) // Size of second sample array, stored in Ethernet memory. Must be multiple of BUF_BLOCK_LENGTH
#define BUF1_BLOCK_LENGTH (BUF1_LENGTH / BUF_BLOCK_LENGTH) // Length of first sample array.
#define BUF2_BLOCK_LENGTH (BUF2_LENGTH / BUF_BLOCK_LENGTH) // Length of second sample array.

// A blank filter struct used to blank deallocating filters.
static const struct filter EMPTY_FILTER;
// Pool of filter structs to use for allocation.
struct filter filter_alloc_pool[FILTER_ALLOC_SIZE];
// Allocation booleans of filter structs.
uint8_t filter_alloc_mask[FILTER_ALLOC_SIZE];

// A blank queue struct used to blank deallocating queues.
static const struct queue EMPTY_QUEUE;
// Pool of queue structs to use for allocation.
struct queue queue_alloc_pool[QUEUE_ALLOC_SIZE];
// Allocation booleans of queue structs.
uint8_t queue_alloc_mask[QUEUE_ALLOC_SIZE];

// A blank queue_element used to blank deallocating filters.
static const struct queue_element EMPTY_QUEUE_ELEMENT;
// Pool of queue_element structs to use for allocation.
struct queue_element queue_element_alloc_pool[QUEUE_ELEMENT_ALLOC_SIZE];
// Allocation booleans of filter queue_elements.
uint8_t queue_element_alloc_mask[QUEUE_ELEMENT_ALLOC_SIZE];

// First pool of audio samples, stored in spare space in the ordinary 32KB of memory.
uint16_t buf1_pool[BUF1_LENGTH];
// This second buf alloc pool utilises the 32K of memory normally reserved for the
// Ethernet and USB. Need to disable/alter this if we use either.
uint16_t *buf2_pool = (uint16_t *)(0x2007C000);

// Allocation booleans for each of the sample pools. Samples are allocated in blocks
// of 100, in order to minimise overhead in the masks.
uint8_t buf1_mask[BUF1_BLOCK_LENGTH];
uint8_t buf2_mask[BUF2_BLOCK_LENGTH];

// Initialise filter allocation routines
void filter_alloc_init()
{
	int i;
	for (i = 0; i < FILTER_ALLOC_SIZE; i++) {
		filter_alloc_mask[i] = 0;
	}
}

// Allocate a filter struct from the pool.
struct filter *alloc_filter()
{
	int i;
	for (i = 0; i < FILTER_ALLOC_SIZE; i++) {
		if (filter_alloc_mask[i] == 0) {
			filter_alloc_mask[i] = 1;

			return &filter_alloc_pool[i];
		}
	}
	return NULL;
}

// Free a filter struct in the pool.
void free_filter(struct filter *f)
{
	int filter_index = (f - &filter_alloc_pool[0]) / sizeof(filter_alloc_pool[0]);
	filter_alloc_pool[filter_index] = EMPTY_FILTER;
	filter_alloc_mask[filter_index] = 0;
}

// Free all filter structs. Generally used when soft-resetting firmware.
void free_all_filters()
{
	uint32_t i;
	for (i = 0; i < FILTER_ALLOC_SIZE; i++) {
		if (filter_alloc_mask[i]) {
			free_filter(&filter_alloc_pool[i]);
		}
	}
}

// Count allocated filter structs.
uint32_t filter_alloced()
{
	uint32_t i, used_count = 0;
	for (i = 0; i < FILTER_ALLOC_SIZE; i++) {
		used_count += filter_alloc_mask[i];
	}
	return used_count;
}

// Queue allocation routines
// Initialise queue struct allocation.
void queue_alloc_init()
{
	int i;
	for (i = 0; i < QUEUE_ALLOC_SIZE; i++) {
		queue_alloc_mask[i] = 0;
	}
}

// Allocate an unused queue struct.
struct queue *alloc_queue()
{
	int i;
	for (i = 0; i < QUEUE_ALLOC_SIZE; i++) {
		if (queue_alloc_mask[i] == 0) {
			queue_alloc_mask[i] = 1;
			return &queue_alloc_pool[i];
		}
	}
	return NULL;
}

// Free provided pointer to queue struct.
void free_queue(struct queue *f)
{
	int queue_index = (f - &queue_alloc_pool[0]) / sizeof(queue_alloc_pool[0]);
	queue_alloc_pool[queue_index] = EMPTY_QUEUE;
	queue_alloc_mask[queue_index] = 0;
}

// Free all queue structs.
void free_all_queues()
{
	uint32_t i;
	for (i = 0; i < QUEUE_ALLOC_SIZE; i++) {
		if (queue_alloc_mask[i]) {
			free_queue(&queue_alloc_pool[i]);
		}
	}
}

// Count allocated queue structs.
uint32_t queue_alloced()
{
	uint32_t i, used_count = 0;
	for (i = 0; i < QUEUE_ALLOC_SIZE; i++) {
		used_count += queue_alloc_mask[i];
	}
	return used_count;
}

// Queue element allocation routines
// Initialise queue element allocation.
void queue_element_alloc_init()
{
	int i;
	for (i = 0; i < QUEUE_ELEMENT_ALLOC_SIZE; i++) {
		queue_element_alloc_mask[i] = 0;
	}
}

// Allocate an unused queue_element struct from the pool.
struct queue_element *alloc_queue_element()
{
	int i;
	for (i = 0; i < QUEUE_ELEMENT_ALLOC_SIZE; i++) {
		if (queue_element_alloc_mask[i] == 0) {
			queue_element_alloc_mask[i] = 1;
			return &queue_element_alloc_pool[i];
		}
	}
	return NULL;
}

// Free specified queue element.
void free_queue_element(struct queue_element *f)
{
	// Get block index of
	int queue_element_index = (f - &queue_element_alloc_pool[0]) / sizeof(queue_element_alloc_pool[0]);
	queue_element_alloc_pool[queue_element_index] = EMPTY_QUEUE_ELEMENT;
	queue_element_alloc_mask[queue_element_index] = 0;
}

// Mark every queue_element in pool as unused.
void free_all_queue_elements()
{
	uint32_t i;
	for (i = 0; i < QUEUE_ELEMENT_ALLOC_SIZE; i++) {
		if (queue_element_alloc_mask[i]) {
			free_queue_element(&queue_element_alloc_pool[i]);
		}
	}
}

// Count how many queue_elements are allocated. Could be used to monitor memory usage.
uint32_t queue_element_alloced()
{
	uint32_t i, used_count = 0;
	for (i = 0; i < QUEUE_ELEMENT_ALLOC_SIZE; i++) {
		used_count += queue_element_alloc_mask[i];
	}
	return used_count;
}

// Initialise sample buffer allocation.
void buf_alloc_init()
{
	free_all_buf();
}

// Attempt to allocate a number of blocks of samples in provided sample buffer.
uint16_t *alloc_buf_pointed(uint8_t *buf_mask, uint16_t *buf_pool, uint32_t buf_length, uint32_t requested_blocks)
{
	uint32_t i, current_blocks_length = 0, current_blocks_start = 0;
	// Iterate over length of buffer mask, looking for enough free blocks.
	for (i = 0; i < buf_length; i++) {
		// If block is used, keep iterating.
		if (buf_mask[i] != 0) {
			current_blocks_length = 0;
			continue;
		}

		// If this is the first unused block, set start index of contiguous blocks.
		if (current_blocks_length == 0) {
			current_blocks_start = i;
		}
		// Increment how many contiguous blocks are unused.
		current_blocks_length++;

		// If we have enough contiguous free blocks, mark them as allocated and return a pointer.
		if (current_blocks_length >= requested_blocks) {
			uint32_t j;
			for (j = current_blocks_start; j < current_blocks_start + requested_blocks; j++) {
				buf_mask[j] = 1;
			}
			return &buf_pool[current_blocks_start * BUF_BLOCK_LENGTH];
		}
	}
	return NULL;
}

// Allocate a certain length of sample buffer.
uint16_t *alloc_buf(uint32_t requested_buf)
{
	uint16_t *buf;
	// How many blocks (each corresponding to an entry in bufN_mask) are requested.
	// @improvement: fix requesting half a block of memory getting 0 blocks in neater way
	// than always allocating an extra block.
	uint32_t requested_blocks = (requested_buf / BUF_BLOCK_LENGTH) + 1;
	// Try to allocate enough blocks in the first sample buffer.
	buf = alloc_buf_pointed(&buf1_mask[0], &buf1_pool[0], BUF1_BLOCK_LENGTH, requested_blocks);
	if (buf == NULL) {
		// If couldn't allocate in first sample buffer, try allocating in the second.
		buf = alloc_buf_pointed(&buf2_mask[0], &buf2_pool[0], BUF2_BLOCK_LENGTH, requested_blocks);
	}
	#if DEBUG==1
	// If compiled in debug mode, print to serial if allocation fails.
	if (buf == NULL) {
		tty_writeln("ERROR: alloc_buf failed for this much memory and blocks:");
		tty_writeln_int(requested_buf);
		tty_writeln_int(requested_blocks);
	}
	#endif
	return buf;
}

// Frees a certain section of the second sample buffer.
void free_buf2(uint16_t *b, uint32_t buf_size)
{
	int i, buf_index = (b - &buf2_pool[0]) / BUF_BLOCK_SIZE;
	for (i = 0; i < buf_size; i++) {
		buf2_mask[buf_index + i] = 0;
	}
}

// Free a certain section of the sample buffers. Assumes provided array is all of
// the underlying buffer blocks, valid in our solution since we never subdivide the
// arrays returned by alloc_buf.
// If a sample is in the second buffer (as determined by the pointer being after the
// end of buf0, which is in the lower half of RAM) continue with a function for that.
void free_buf(uint16_t *b, uint32_t buf_size)
{
	int i, buf_index = (b - &buf1_pool[0]) / BUF_BLOCK_SIZE;
	if (buf_index > BUF1_BLOCK_LENGTH) {
		return free_buf2(b, buf_size);
	}
	for (i = 0; i < buf_size; i++) {
		buf1_mask[buf_index + i] = 0;
	}
}

// Free all buffers (non-recursively). Used when performing a soft system reset.
// The use of this instead of recursively finding and freeing all buffers is
// non-ideal when only a small number of buffers are allocated, but it helps reduce
// the state to keep track of.
void free_all_buf()
{
	free_buf(&buf1_pool[0], BUF1_LENGTH);
	free_buf(&buf2_pool[0], BUF2_LENGTH);
}

// Write zero to provided array of samples. Used when initialising new sample buffer.
// Buffer element allocation routines
void zero_buf(uint16_t *buf, uint16_t buf_size)
{
	uint16_t i;
	for (i = 0; i < buf_size; i++) {
		buf[i] = 0;
	}
}

// Count how many samples are allocated in the buffers.
uint32_t buf_alloced()
{
	uint16_t i, used_count = 0;
	for (i = 0; i < BUF1_BLOCK_LENGTH; i++) {
		used_count += buf1_mask[i];
	}
	for (i = 0; i < BUF2_BLOCK_LENGTH; i++) {
		used_count += buf2_mask[i];
	}
	return used_count;
}

// Write a value to a specified sample buffer of a given filter struct.
void filter_buf_write(struct filter *filter, uint8_t buf_n, uint16_t val)
{
	if (buf_n == 0) {
		filter->buf0[(filter->buf0_head_index++) & filter->buf0_size_mask] = val;
		return;
	}
	if (buf_n == 1) {
		filter->buf1[(filter->buf1_head_index++) & filter->buf1_size_mask] = val;
		return;
	}
	#if DEBUG==1
	tty_writeln("ERROR: Trying to read a buf_n that isn't 0 or 1.");
	#endif
}

// Read the value of a specified sample buffer of a given filter struct.
uint16_t filter_buf_read(struct filter *filter, uint8_t buf_n, uint16_t pos)
{
	if (buf_n == 0) {
		return filter->buf0[(filter->buf0_head_index + (~pos)) & filter->buf0_size_mask];
	} else if (buf_n == 1) {
		return filter->buf1[(filter->buf1_head_index + (~pos)) & filter->buf1_size_mask];
	}
	#if DEBUG==1
	tty_writeln("ERROR: Trying to read a buf_n that isn't 0 or 1.");
	#endif
	return 0;
}

// Initialise allocation routines. For use on system setup in main().
void alloc_init() {
	filter_alloc_init();
	queue_alloc_init();
	queue_element_alloc_init();
	buf_alloc_init();
}

// Handles output from filter functions. Simple call just to prevent duplicating code
// in dozens of functions with common behaviour.
uint16_t filter_output(struct filter *filter, uint16_t v) {
	if (filter->next) {
		filter_buf_write(filter->next, filter->next_buf_n, v);
	}
	if (filter->next2) {
		filter_buf_write(filter->next2, filter->next2_buf_n, v);
	}
	return v;
}

#endif
