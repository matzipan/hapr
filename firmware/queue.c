#ifndef _HAPR_QUEUE
#define _HAPR_QUEUE

#include "queue.h"

struct queue *new_queue()
{ 
	struct queue *queue = alloc_queue();
	queue->head = NULL;
	queue->tail = NULL;	
	queue->current = NULL;
	return queue;
}

struct filter *dequeue(struct queue *q)
{
	struct filter *filter;
	if (q->head == NULL)
	{
		return NULL;
	} else {
		if(q->current == q->head) {
			q->current = q->current->next;
		}

		filter = q->head->value;
		free_queue_element(q->head);
		q->head = q->head->next;
		return filter;
	}
}

void enqueue(struct queue* q, struct filter *filter)
{
	struct queue_element *qe = alloc_queue_element();
	qe->value = filter;
	qe->next = NULL;

	if (q->head == NULL)
	{
		q->head = qe;
		q->tail = qe;
		q->current = qe;
		return;
	} else {
		q->tail->next = qe;
		q->tail = qe;
	 	return;
	}
}

// moves the current pointer to next,
// marked as inline to allow compiler optimizations
inline void move_to_next(struct queue* q) {
	q->current = q->current->next;
}

// moves the current pointer to head of queue,
// marked as inline to allow compiler optimizations
void move_to_start(struct queue* q) {
	q->current = q->head;
}


inline struct filter* get_element(struct queue *q) {
	if(q->current == NULL)
		return NULL;

	return q->current->value;
}

#endif
