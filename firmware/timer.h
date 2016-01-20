#ifndef _HAPR_TIMER_H
#define _HAPR_TIMER_H

uint16_t cycle;
uint16_t frequency;

void timer_init(int frequency);
void timer_start();
void timer_stop();

#endif