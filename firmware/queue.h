#ifndef _HAPR_QUEUE_H
#define _HAPR_QUEUE_H

struct queue
{
   struct queue_element *head; 
   struct queue_element *tail;
   struct queue_element *current; // used for walking the queue
};

struct queue_element
{
   struct filter *value;
   struct queue_element *next;
};

struct queue *new_queue();

struct filter *dequeue(struct queue *q);

void enqueue(struct queue* q, struct filter *filter);

inline void move_to_next(struct queue* q);
void move_to_start(struct queue* q);
inline struct filter* get_element(struct queue *q);

#endif