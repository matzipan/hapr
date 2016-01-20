#ifndef _HAPR_ALLOC_H
#define _HAPR_ALLOC_H

// Struct used to contain an initialised filter.
struct filter
{
	uint16_t filter_id; // Integer ID of filter, used for reference and in UI.
	void (*filter_function)(struct filter *filter); // Function to use to apply filter.
	struct filter *next; // Pointer to first next filter struct in graph.
	uint16_t next_buf_n; // Buffer number of first next filter struct in graph.
	struct filter *next2; // Pointer to second next filter struct in graph.
	uint16_t next2_buf_n; // Buffer number of second next filter struct in graph.
	uint16_t param0; // First parameter for filter.
	uint16_t param1; // Second parameter for filter.
	uint16_t param2; // Third parameter for filter.
	uint16_t param3; // Fourth parameter for filter.
	uint16_t multi_input; // Does filter have a second buffer? 0/1
	uint16_t buf0_size; // Length of first sample circular buffer.
	uint16_t buf0_size_mask;
	uint16_t buf0_head_index; // Current index in first sample circular buffer.
	uint16_t *buf0; // Array of first sample circular buffer.
	uint16_t buf1_size; // Length of second sample circular buffer.
	uint16_t buf1_size_mask;
	uint16_t buf1_head_index; // Current index in second sample circular buffer.
	uint16_t *buf1; // Array of second sample circular buffer.
};

void alloc_init();
struct filter *alloc_filter();
void free_all_filters();
struct queue *alloc_queue();
void free_queue(struct queue *f);
struct queue_element *alloc_queue_element();
void free_queue_element(struct queue_element *f);
void free_all_buf();
void zero_buf(uint16_t *buf, uint16_t buf_size);
uint16_t *alloc_buf(uint32_t requested_buf);
void free_buf(uint16_t *b, uint32_t buf_size);
uint16_t filter_buf_read(struct filter *filter, uint8_t buf_n, uint16_t pos);
void alloc_init();
uint16_t filter_output(struct filter *filter, uint16_t v);

#endif
