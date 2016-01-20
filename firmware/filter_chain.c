// Modified 2014-01-28 by Alex Oyston
// 	- Created circular buffer
// Modified 2014-01-28 by Andrei Zisu
//	- Restructure, cleanup
// Modified 2014-01-28 by Michael Mokrysz
//	- Fix for filter chain
// Modified 2014-01-30 by Michael Mokrysz
//	- Debugging and memory allocation stuff
// Modified 2014-02-04 by Michael Mokrysz
//  - Included serial and added debugging
//	- More memory allocation stuff
// Modified 2014-02-25 by Michael Mokrysz
//	- Alloc stuff
// Modified 2014-02-25 by Alex Oyston, Michael Mokrysz
//	- Added input, output, zero, max and min filters
// Modified 2014-02-28 by Alex Oyston
//	- Added mixer filter and parameters to filters
//	- Added delay, reverb, sine filters
// Modified 2014-03-03 by Michael Mokrysz
//	- Added prototype high pass and low pass filters
// Modified 2014-03-03 by Andrei Zisu
//	- Merge and cleanup
// Modified 2014-03-04 by Michael Mokrysz
//	- Added flange filter
//	- Added alloc stuff for filters
// Modified 2014-03-04 by Alex Oyston
//	- Added multiple outputs feature for filters
//	- Created filter_init
// Modified 2014-03-05 by Andrei Zisu
//	- Refactoring of input and output
//	- Fix for circular buffer
// Modified 2014-03-06 by Michael Mokrysz
//	- Improve filter_init
//	- Tidied filter_init a lot
// 	- Added fix for filter order
// Modified 2014-03-07 by Alex Oyston
//	- Added compressor filters
// Modified 2014-03-07 by Andrei Zisu
// 	- Fixes, debugs + refactor
//	- Improve filter_init
//	- Fix issue with non consecutive ids
// Modified 2014-03-08 by Andrei Zisu
//	- Add error trap
//	- Add filter function sizes
//	- Adjust filters
// Modified 2014-03-08 by Alex Oyston
//	- Improve filters
//	- Added distortion
// Modified 2014-03-08 by Michael Mokrysz
//	- Final alloc changes
//	- More fixes for filter_init
// Modified 2014-03-10 by Michael Mokrysz
//	- More fixes for filter_init
//	- Low pass filter
// Modified 2014-03-10 by Alex Oyston
//	- Fix filter_init bug
// Modified 2014-03-10 by Andrei Zisu
//	- Improve queue
//	- Add catches for filter_id errors in filter_init
// Modified 2014-03-12 by Andrei Zisu
//	- Remove useless code, cleanup etc.
// Modified 2014-03-12 by Andrei Zisu
//  - Final fix for filter_init


#ifndef _HAPR_FC
#define _HAPR_FC

#include "math.h"
#include "stdio.h"
#include "stdlib.h"

#include "alloc.h"
#include "dac.h"
#include "adc.c"
#include "queue.h"
#include "filter_chain.h"
#include "timer.h"

#define VALUE_RANGE 3300 // maximum peak value
#define VALUE_ZERO 2000
#define PI 3.14159265359
#define TWO_PI 6.28318530718

#define NUMBER_OF_STEPS 100 // maximum value allowed for the parameters
#define ADC_STEP VALUE_RANGE/NUMBER_OF_STEPS // a step that is used to scale parameters to a certain ADC value

// input from ADC
void input_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("input_function");
	#endif
	uint16_t v = adc_get_data();

	filter_output(filter, v);
}

// output to DAC
void output_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("output_function");
	#endif

	uint16_t v = filter_buf_read(filter, 0, 0);

	#if DEBUG==1 && TRACE==1
	tty_write("Output = ");
	tty_writeln_int(v);
	#endif

	dac_set_value(v);
}

// passthrough, no change, takes one input and gives maximum of two, so it is used for splitting
void passthrough_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("passthrough_function");
	#endif

	uint16_t v = filter_buf_read(filter, 0, 0);

	filter_output(filter, v);
}

// zeroes out input, takes no input
void zero_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("zero_function");
	#endif
	uint16_t v = VALUE_ZERO;

	filter_output(filter, v);
}

// limit output amplitude
// takes param0: a step value that gets multiplied with ADC_STEP to compute the maximum value
void max_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("max_function");
	#endif
	uint16_t max = filter->param0*ADC_STEP;
	uint16_t v = filter_buf_read(filter, 0, 0);

	if (v > max)
		v = max;

	filter_output(filter, v);
}

// limit output amplitude
// takes param0: a step value that gets multiplied with ADC_STEP to compute the maximum value
void min_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("min_function");
	#endif
	uint16_t min = filter->param0*ADC_STEP;
	uint16_t v = filter_buf_read(filter, 0, 0);

	if (v < min)
		v = min;

	filter_output(filter, v);
}

// creates and outputs a sine wave, takes no input
// param0: 0 to NUMBER_OF_STEPS, amplitude, 0
// param1: frequency of sine wave, gets multiplied by 100
// param2: 0 to NUMBER_OF_STEPS, phase shift, 0 means 0 phase, NUMBER_OF_STEPS means TWO_PI phase difference
void sine_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("sine_function");
	#endif
	uint16_t amp = (((float)filter->param0)* (VALUE_RANGE-VALUE_ZERO)) /NUMBER_OF_STEPS;
	uint16_t freq = filter->param1 * 100;
	uint16_t phase = (((float)filter->param2)*TWO_PI)/NUMBER_OF_STEPS;

	float t = ((float)cycle) / frequency;

	float x = sin(TWO_PI * freq * t + phase);

	uint16_t v = (amp * x) + VALUE_ZERO;

	filter_output(filter, v);
}

// tremolo filter
// param0: depth, percentage: 100% - amplitude goes between 0 and 1; 30% - amplitude goes between 0.7 and 1
// param1: 0-NUMBER_OF_STEPS, frequency
// param2: 0 to NUMBER_OF_STEPS, phase shift, 0 means 0 phase, NUMBER_OF_STEPS means TWO_PI phase difference
// param3: 0-NUMBER_OF_STEPS, type, 1- triangle, 2 - square, 3 - upward saw, 4 - downward saw, otherwise - sine
void tremolo_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("tremolo_function");
	#endif

	uint16_t depth = filter->param0;
	uint16_t freq = filter->param1;
	uint16_t phase = (((float)filter->param2)*TWO_PI)/NUMBER_OF_STEPS;
	uint16_t type  = filter->param3;

	if(depth > 100){
		filter->param0 = depth = 100;
	}

	int16_t v = filter_buf_read(filter, 0, 0);

	float d = (depth / 100.0);

	float x;
	if(type == 1){
		//tri
		x = abs((cycle % (freq*2)) - freq) / (float)freq;
	}else if(type == 2){
		//sqr
		x = (cycle % freq) < (freq/2) ? 0 : 1;
	}else if(type == 3){
		//upward saw
		x = (cycle % freq) / (float)freq;
	}else if(type == 4){
		//downward saw
		x = (freq - (cycle % freq)) / (float)freq;
	}else{
		//sin
		float t = (float)cycle / (float)frequency;
		x = (sin(TWO_PI * freq * t + phase)+1) * 0.5;
	}

	float inter = x * d;

	v-=VALUE_ZERO;
	v = (v * inter);
	v+=VALUE_ZERO;

	filter_output(filter, v);
}

// reverb filter
// param0: 0-NUMBER_OF_STEPS, decay 0- no delay, NUMBER_OF_STEPS - maximum delay
// param1: 0-100, decay
// param2: 0-NUMBER_OF_STEPS, frequency
void reverb_function(struct filter *filter) //@TODO test
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("delay_function");
	#endif
	uint16_t delay = (((float)filter->param0)*DELAY_BUFFER_SIZE)/NUMBER_OF_STEPS;
	uint16_t decay = filter->param1;
	uint16_t freq = filter->param2;

	if (decay > 100){
		filter->param1 = decay = 100;
	}

	float t = (float)cycle / (float)frequency;
	float x = ((sin(TWO_PI * freq * t)+1) * 0.25) + 0.5;

	delay = delay * x;

	uint16_t v = filter_buf_read(filter, 0, 1);
	int16_t delay_v = (int16_t)filter_buf_read(filter, 0, delay);

	delay_v -= VALUE_ZERO;
	delay_v *= decay;
	delay_v /= 100;

	v = v + delay_v;

	filter_output(filter, v);
}

// delay function
// param0: 0-NUMBER_OF_STEPS, decay 0- no delay, NUMBER_OF_STEPS - maximum delay
void delay_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("delay_function");
	#endif
	uint16_t delay = (((float)filter->param0)*DELAY_BUFFER_SIZE)/NUMBER_OF_STEPS;
	uint16_t v = filter_buf_read(filter, 0, delay);

	filter_output(filter, v);
}

// mix function, it takes two input buffer and multiplexes it using a ratio
// param0: 0-NUMBER_OF_STEPS, mix ratio, 0 - only buffer 1; NUMBER_OF_STEPS/2 - half and half; NUMBER_OF_STEPS - only buffer 2
void mix_function(struct filter *filter) //@TODO test
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("mix_function");
	#endif
	uint16_t p0 = filter->param0;

	uint16_t v1 = filter_buf_read(filter, 0, 0);
	uint16_t v2 = filter_buf_read(filter, 1, 0);

	v1 = (p0*v1)/NUMBER_OF_STEPS + ((NUMBER_OF_STEPS-p0)*v2)/NUMBER_OF_STEPS;

	filter_output(filter, v1);
}

// flange filter
// param0: 0-NUMBER_OF_STEPS, frequency
// param1: 0 to NUMBER_OF_STEPS, phase shift, 0 means 0 phase, NUMBER_OF_STEPS means TWO_PI phase difference
// param2: 0 to NUMBER_OF_STEPS, max delay, 0 - no delay, NUMBER_OF_STEPS means FLANGE_BUFFER_SIZE maxdelay
// param3: 0-NUMBER_OF_STEPS, type, 1- triangle, 2 - square, 3 - upward saw, 4 - downward saw, otherwise - sine
void flange_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("flange_function");
	#endif

	uint16_t freq = filter->param0;
	uint16_t phase = (((float)filter->param1)*TWO_PI)/NUMBER_OF_STEPS;
	uint16_t maxdelay = (((float)filter->param2)*FLANGE_BUFFER_SIZE)/NUMBER_OF_STEPS;
	uint16_t type  = filter->param3;

	float x;
	if(type == 1){
		//tri
		x = abs((cycle % (freq*2)) - freq) / (float)freq;
	}else if(type == 2){
		//sqr
		x = (cycle % freq) < (freq/2) ? 0 : 1;
	}else if(type == 3){
		//upward saw
		x = (cycle % freq) / (float)freq;
	}else if(type == 4){
		//downward saw
		x = (freq - (cycle % freq)) / (float)freq;
	}else{
		//sin
		float t = (float)cycle / (float)frequency;
		x = (sin(TWO_PI * freq * t + phase)+1) * 0.5;
	}

	uint16_t delay = x * maxdelay;

	uint16_t v1 = filter_buf_read(filter, 0, 0);
	uint16_t v2 = filter_buf_read(filter, 0, delay);

	v1 = (v1 + v2) / 2;
	filter_output(filter, v1);
}

// upward compressor filter
// param0: 0-NUMBER_OF_STEPS, threshold, 0 - VALUE_ZERO threshold, NUMBER_OF_STEPS - VALUE_RANGE threshold
// param1:  0-NUMBER_OF_STEPS, controls the steepness of the compression
void upward_compressor_function(struct filter *filter) { //@TODO test
	#if DEBUG==1 && TRACE==1
	tty_writeln("upward_compressor_function");
	#endif

	uint16_t threshold = (((float)filter->param0)*(VALUE_RANGE-VALUE_ZERO))/NUMBER_OF_STEPS;
	uint16_t ratio = filter->param1;

	threshold = VALUE_ZERO + threshold;

	uint16_t v = filter_buf_read(filter, 0, 0);

	if(ratio == 0){
		ratio = 1;
	}

	if(v > threshold){
		uint16_t diff = v - threshold;
		v = threshold + (diff / ratio);
	}

	filter_output(filter, v);
}

// upward compressor filter
// param0: 0-NUMBER_OF_STEPS, threshold, 0 - VALUE_ZERO threshold, NUMBER_OF_STEPS - VALUE_RANGE threshold
// param1:  0-NUMBER_OF_STEPS, controls the steepness of the compression
void downward_compressor_function(struct filter *filter) //@TODO test
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("downward_compressor_function");
	#endif
	uint16_t threshold = (((float)filter->param0)*(VALUE_RANGE-VALUE_ZERO))/NUMBER_OF_STEPS;
	uint16_t ratio = filter->param1; // ratio of ratio:1

	threshold = VALUE_ZERO - threshold;

	uint16_t v = filter_buf_read(filter, 0, 0);

	if(ratio == 0){
		ratio = 1;
	}

	if(v < threshold){
		uint16_t diff = threshold - v;
		v = threshold - (diff / ratio);
	}

	filter_output(filter, v);
}

// n bits filter
// param0: 0-12, quantization level
void n_bits_function(struct filter *filter) //@TODO Improve
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("n_bits_function");
	#endif
	uint16_t n = filter->param0;

	uint16_t v = filter_buf_read(filter, 0, 0);

	if(n > 12){
		n = 12;
	}
	v = (v >> (12-n)) << (12-n);

	filter_output(filter, v);
}

// distortion filter
// param0: 0 - NUMBER_OF_STEPS, distortion level, 0 - no distortion, NUMBER_OF_STEPS - maximum distortion
void distortion_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("distortion_function");
	#endif

	uint16_t distortion = (((float)filter->param0)*(VALUE_RANGE-VALUE_ZERO))/NUMBER_OF_STEPS;

	uint16_t v = filter_buf_read(filter, 0, 0);
	if(v > VALUE_ZERO + distortion){
		v = VALUE_ZERO + distortion;
	}
	if(v < VALUE_ZERO - distortion){
		v = VALUE_ZERO - distortion;
	}

	filter_output(filter, v);
}

// triangle generator function, does not take any input
// param0: 0-NUMBER_OF_STEPS, frequency
// param1: 0 to NUMBER_OF_STEPS, amplitude
void triangle_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("triangle_function");
	#endif
	uint16_t freq = filter->param0;
	uint16_t amp = (((float)filter->param1)* (VALUE_RANGE-VALUE_ZERO)) /NUMBER_OF_STEPS;

	float x;

	x = 2 * (abs((cycle % (freq*2)) - freq) / (float)freq) - 1;

	uint16_t v = VALUE_ZERO + (x*amp);

	filter_output(filter, v);
}

// We proved unable to make the low/high/all-pass filters fully functional, despite
// considerable research and implementation attempts.
// These based upon http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
float aec_lowpass_a[3] = {0, 0, 0};
float aec_lowpass_b[3] = {0, 0, 0};

// generates constants required for the lowpass filter
void aec_lowpass_constants(fs, f0, q)
{
	#if DEBUG==1
	tty_writeln("aec_lowpass_constants");
	#endif
	float w0 = 2 * PI * f0/fs;
	float cw0 = cos(w0);
	float sw0 = sin(w0);
	float alpha = sw0 / (2*q);
	float omcw0 = 1 - cw0;

	aec_lowpass_b[0] = omcw0 / 2;
	aec_lowpass_b[1] = omcw0;
	aec_lowpass_b[2] = aec_lowpass_b[0];
	aec_lowpass_a[0] = 1 + alpha;
	aec_lowpass_a[1] = -2*cw0;
	aec_lowpass_a[2] = 1 - alpha;
}

// lowpass filter, takes one input, no params
void aec_lowpass_function(struct filter *filter) //@TODO
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("aec_lowpass_function");
	#endif

	float x0 = filter_buf_read(filter, 0, 0) - VALUE_ZERO;
	float x1 = filter_buf_read(filter, 0, 1) - VALUE_ZERO;
	float x2 = filter_buf_read(filter, 0, 2) - VALUE_ZERO;
	float y1 = filter_buf_read(filter->next, 0, 1) - VALUE_ZERO;
	float y2 = filter_buf_read(filter->next, 0, 2) - VALUE_ZERO;

	float vf = (aec_lowpass_b[0] / aec_lowpass_a[0]) * x0;
	vf += (aec_lowpass_b[1] / aec_lowpass_a[0]) * x1;
	vf += (aec_lowpass_b[2] / aec_lowpass_a[0]) * x2;
	vf -= (aec_lowpass_a[1] / aec_lowpass_a[0]) * y1;
	vf -= (aec_lowpass_b[2] / aec_lowpass_a[0]) * y2;

	uint16_t v = vf + VALUE_ZERO;

	filter_output(filter, v);
}

float aec_highpass_a[3] = {0, 0, 0};
float aec_highpass_b[3] = {0, 0, 0};

// generates constants required for the highpass filter
void aec_highpass_constants(fs, f0, q)
{
	#if DEBUG==1
	tty_writeln("aec_highpass_constants");
	#endif
	float w0 = 2 * PI * f0/fs;
	float cw0 = cos(w0);
	float sw0 = sin(w0);
	float alpha = sw0 / (2*q);
	float omcw0 = 1 + cw0;

	aec_highpass_b[0] = omcw0 / 2;
	aec_highpass_b[1] = -omcw0;
	aec_highpass_b[2] = aec_highpass_b[0];
	aec_highpass_a[0] = 1 + alpha;
	aec_highpass_a[1] = -2*cw0;
	aec_highpass_a[2] = 1 - alpha;
}

// highpass filter, takes one input, no params
void aec_highpass_function(struct filter *filter) //@TODO
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("aec_highpass_function");
	#endif

	float x0 = filter_buf_read(filter, 0, 0);
	float x1 = filter_buf_read(filter, 0, 1);
	float x2 = filter_buf_read(filter, 0, 2);
	float y1 = filter_buf_read(filter->next, 0, 1);
	float y2 = filter_buf_read(filter->next, 0, 2);

	float vf = (aec_highpass_b[0] / aec_highpass_a[0]) * x0;
	vf += (aec_highpass_b[1] / aec_highpass_a[0]) * x1;
	vf += (aec_highpass_b[2] / aec_highpass_a[0]) * x2;
	vf -= (aec_highpass_a[1] / aec_highpass_a[0]) * y1;
	vf -= (aec_highpass_b[2] / aec_highpass_a[0]) * y2;

	uint16_t v = vf;

	filter_output(filter, v);
}

float aec_allpass_a[3] = {0, 0, 0};
float aec_allpass_b[3] = {0, 0, 0};

// generates constants required for the allpass filter
void aec_allpass_constants(fs, f0, q)
{
	#if DEBUG==1
	tty_writeln("aec_lowpass_constants");
	#endif
	float w0 = 2 * PI * f0/fs;
	float cw0 = cos(w0);
	float sw0 = sin(w0);
	float alpha = sw0 / (2*q);

	aec_allpass_b[0] = 1 - alpha;
	aec_allpass_b[1] = -2*cw0;
	aec_allpass_b[2] = 1 + alpha;
	aec_allpass_a[0] = 1 + alpha;
	aec_allpass_a[1] = -2*cw0;
	aec_allpass_a[2] = 1 - alpha;
}

// allpass filter, takes one input, no params
void aec_allpass_function(struct filter *filter) //@TODO
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("aec_lowpass_function");
	#endif

	float x0 = filter_buf_read(filter, 0, 0);
	float x1 = filter_buf_read(filter, 0, 1);
	float x2 = filter_buf_read(filter, 0, 2);
	float y1 = filter_buf_read(filter->next, 0, 1);
	float y2 = filter_buf_read(filter->next, 0, 2);

	float vf = (aec_allpass_b[0] / aec_lowpass_a[0]) * x0;
	vf += (aec_allpass_b[1] / aec_allpass_a[0]) * x1;
	vf += (aec_allpass_b[2] / aec_allpass_a[0]) * x2;
	vf -= (aec_allpass_a[1] / aec_allpass_a[0]) * y1;
	vf -= (aec_allpass_b[2] / aec_allpass_a[0]) * y2;

	uint16_t v = vf;

	filter_output(filter, v);
}

float phaser_a[12] = {0, 0, 0};
float phaser_b[12] = {0, 0, 0};

// generate phase constants
void phaser_constants(fs0, f00, q0, fs1, f01, q1, fs2, f02, q2, fs3, f03, q3)
{
	#if DEBUG==1
	tty_writeln("phaser_constants");
	#endif

	float w00 = 2 * PI * f00/fs0;
	float cw00 = cos(w00);
	float sw00 = sin(w00);
	float alpha0 = sw00 / (2*q0);
	aec_allpass_b[0] = 1 - alpha0;
	aec_allpass_b[1] = -2*cw00;
	aec_allpass_b[2] = 1 + alpha0;
	aec_allpass_a[0] = 1 + alpha0;
	aec_allpass_a[1] = -2*cw00;
	aec_allpass_a[2] = 1 - alpha0;

	float w01 = 2 * PI * f01/fs1;
	float cw01 = cos(w01);
	float sw01 = sin(w01);
	float alpha1 = sw01 / (2*q1);
	aec_allpass_b[3] = 1 - alpha1;
	aec_allpass_b[4] = -2*cw01;
	aec_allpass_b[5] = 1 + alpha1;
	aec_allpass_a[3] = 1 + alpha1;
	aec_allpass_a[4] = -2*cw01;
	aec_allpass_a[5] = 1 - alpha1;

	float w02 = 2 * PI * f02/fs2;
	float cw02 = cos(w02);
	float sw02 = sin(w02);
	float alpha2 = sw02 / (2*q2);
	aec_allpass_b[6] = 1 - alpha2;
	aec_allpass_b[7] = -2*cw02;
	aec_allpass_b[8] = 1 + alpha2;
	aec_allpass_a[6] = 1 + alpha2;
	aec_allpass_a[7] = -2*cw02;
	aec_allpass_a[8] = 1 - alpha2;

	float w03 = 2 * PI * f03/fs3;
	float cw03 = cos(w03);
	float sw03 = sin(w03);
	float alpha3 = sw03 / (2*q3);
	aec_allpass_b[9] = 1 - alpha3;
	aec_allpass_b[10] = -2*cw03;
	aec_allpass_b[11] = 1 + alpha3;
	aec_allpass_a[9] = 1 + alpha3;
	aec_allpass_a[10] = -2*cw03;
	aec_allpass_a[11] = 1 - alpha3;
}

// phaser filter, takes one input, no params
void phaser_function(struct filter *filter)
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("phaser_function");
	#endif

	float x0 = filter_buf_read(filter, 0, 0);
	float x1 = filter_buf_read(filter, 0, 1);
	float x2 = filter_buf_read(filter, 0, 1);
	float y1 = filter_buf_read(filter->next, 0, 1);
	float y2 = filter_buf_read(filter->next, 0, 1);

	float vf = (aec_allpass_b[0] / aec_lowpass_a[0]) * x0;
	vf += (aec_allpass_b[1] / aec_allpass_a[0]) * x1;
	vf += (aec_allpass_b[2] / aec_allpass_a[0]) * x2;
	vf -= (aec_allpass_a[1] / aec_allpass_a[0]) * y1;
	vf -= (aec_allpass_b[2] / aec_allpass_a[0]) * y2;

	uint16_t v = vf;

	filter_output(filter, v);
}

// noise cancellation filter, takes one input, no params
// param0, 0-NUMBER_OF_STEPS, maximum diff, multiplied by 5
// param1, 0-NUMBER_OF_STEPS, number of samples to use for averaging
void noise_cancellation_function(struct filter *filter)
{
	uint16_t max_diff = filter->param0*5;
	uint16_t samples = filter->param1;

	uint16_t avg = 0;

	uint16_t v = filter_buf_read(filter, 0, 0);
	int i;
	uint16_t diff;

	for(i =1; i < samples+1; i++){
		avg += filter_buf_read(filter,0,i);
	}
	avg /= samples;

	diff = abs(avg - v);
	if(diff > max_diff){
		v = avg;
	}

	filter_output(filter, v);
}

// Lookup filter_id in an array indexed by i, the indexes in filters_buf (see filter_init).
uint16_t filter_id_to_i(uint16_t *filter_ids, uint16_t filters_count, uint16_t uid) {
	uint16_t i;
	for(i=0; i<filters_count; i++) {
		if(filter_ids[i] == uid)
			return i;
	}

	return 0;
}

// Setup a new filter struct. Allocate one and then set simple parameters from
// specification array.
struct filter *new_filter(uint16_t *filter_buf)
{
	// Extract values from specification array.
	uint16_t filter_functions_index = filter_buf[0];
	uint16_t filter_id = filter_buf[1];
	uint16_t param0 = filter_buf[4];
	uint16_t param1 = filter_buf[5];
	uint16_t param2 = filter_buf[6];
	uint16_t param3 = filter_buf[7];

	// Identify the function this filter will use when applying.
	void (*filter_function)(struct filter *filter) = filter_functions[filter_functions_index];
	// Only if the filter is MIX or FLANGE does the filter accept multiple (2) inputs.
	uint16_t multi_input = (filter_function == mix_function || filter_function == flange_function);

	// Allocate filter struct (see alloc.c).
	struct filter *filter = alloc_filter();

	// Set filter properties.
	filter->filter_id = filter_id;
	filter->filter_function = filter_function;

	// Allocate sample buffers (see alloc.c) and zero contents.
	uint16_t buf_size = filter_function_sizes[filter_functions_index] << multi_input;
	uint16_t *new_buf0 = alloc_buf(buf_size);
	zero_buf(new_buf0, buf_size);
	uint16_t *new_buf1;
	// If filter has two buffers, allocate and zero the second.
	if (multi_input) {
		new_buf1 = alloc_buf(buf_size);
		zero_buf(new_buf1, buf_size);
	}

	// Set filter buffer metadata.
	filter->multi_input = multi_input;
	filter->buf0_size = buf_size;
	filter->buf0_size_mask = buf_size - 1;
	filter->buf0_head_index = 0;
	filter->buf0 = new_buf0;
	if (multi_input) {
		filter->buf1_size = buf_size;
		filter->buf1_size_mask = buf_size - 1;
		filter->buf1_head_index = 0;
		filter->buf1 = new_buf1;
	}

	// Set filter parameters.
	filter->param0 = param0;
	filter->param1 = param1;
	filter->param2 = param2;
	filter->param3 = param3;

	return filter;
}

struct queue *q;

// Input and output filters need to be included in filters_buf.
// Input filter must be index 0 and filter_id 0.
// Output filter must be index 1 and filter_id 1.
uint16_t filter_init(uint16_t *filters_buf, uint16_t filters_count)
{
	// Initialise constants for high/low/all-pass filters.
	// @TODO: (urgent) allow for multiple filter parameters for these.
	aec_highpass_constants(2000, 1, 10);
	aec_lowpass_constants(2000, 1000, 1000);
	aec_allpass_constants(2000, 1, 10);

	#if DEBUG==1
	tty_writeln("Filter functions init");
	#endif

	uint16_t i;

	// Initialise array of filter struct pointers.
	struct filter *filters[filters_count];
	// Initialise array of filter_id at index i, for lookup.
	uint16_t filter_ids[filters_count];
	// Initialise array of when filter is used as next ("references"). Used to ensure
	// buffers aren't written to multiple times in one filter chain.
	uint16_t filter_refcount[filters_count];
	for (i = 0; i < filters_count; i++) {
		filter_refcount[i] = 0;
		filter_ids[i] = filters_buf[i*8+1];
	}

	#if DEBUG==1
	tty_writeln("Starting filter init loop");
	#endif
	for (i = 0; i < filters_count; i++) {
		// Allocate and set parameters upon a filter struct for each filter, add to array.
		struct filter *cur_filter = new_filter(&filters_buf[i*8]);
		uint16_t index = filter_id_to_i(filter_ids, filters_count, cur_filter->filter_id);
		filters[index] = cur_filter;
	}
	#if DEBUG==1
	tty_writeln("Filter alloc done");
	#endif

	// Assign parameters on filters, plus next pointers and loop checking.
	uint16_t j;
	for (j = 0; j < filters_count; j++)
	{
		// Find out index of filter in filters_buf.
		i = filter_id_to_i(filter_ids, filters_count, filters_buf[j*8+1]);

		// Get filters_buf indexes of next filters (need to look up filter_id since the
		// filters aren't necessarily in order).
		uint16_t filter_next_id = filter_id_to_i(filter_ids, filters_count, filters_buf[j*8 + 2]);
		uint16_t filter_next2_id = filter_id_to_i(filter_ids, filters_count, filters_buf[j*8 + 3]);

		if (filter_next_id) {
			// Check we're not pointing to a filter in the chain more than how many
			// buffers it has. This generally prevents cyclic graphs.
			if (filter_refcount[filter_next_id] > filters[filter_next_id]->multi_input + 1){

				#if DEBUG==1
				tty_write("ERROR. A third filter is trying to write to filter ID ");
				tty_writeln_int((int) filter_next_id);
				#endif
				return 1;
			}
			if (filter_next_id == i){ //Check if a filter is outputting to itself
				#if DEBUG==1
				tty_write("ERROR. A filter is trying to output to itself. Filter ID: ");
				tty_writeln_int((int) filter_next_id);
				tty_writeln_int((int) i);
				#endif
				return 2;
			}
			// Set next filter, buffer index in next filter. Register reference to that filter.
			filters[i]->next = filters[filter_next_id];
			filters[i]->next_buf_n = filter_refcount[filter_next_id];
			filter_refcount[filter_next_id]++;
		}
		if (filter_next2_id) {
			if (filter_refcount[filter_next2_id] > filters[filter_next2_id]->multi_input + 1) {
				#if DEBUG==1
				tty_write("ERROR. A third filter is trying to write to filter ID ");
				tty_writeln_int((int) filter_next2_id);
				#endif
				return 3;
			}
			if (filter_next2_id == i){
				#if DEBUG==1
				tty_write("ERROR. A filter is trying to output to itself. Filter ID: ");
				tty_writeln_int((int) filter_next2_id);
				#endif
				return 4;
			}
			filters[i]->next2 = filters[filter_next2_id];
			filters[i]->next2_buf_n = filter_refcount[filter_next2_id];
			filter_refcount[filter_next2_id]++;
		}
	}

	#if DEBUG==1
	tty_writeln("Finished filter init loop. Doing queuing");
	#endif

	// Set firmware-wide head filter.
	// The first filter to come down the serial MUST be the head filter.
	head_filter = filters[0];

	// Allocate a queue struct and make a filter struct pointer.
	// We use this queue to apply the filters, creating the data structure
	// only once in order to make ADC sampling complete faster (allowing
	// more complex filter chains and higher sampling frequency).
	q = new_queue();
	struct filter *current_filter;

	// Add head_filter to the queue.
	enqueue(q, head_filter);

	// Recurse tree adding filters to queue.
	while((current_filter = get_element(q)))
	{
		// Only enqueue when next_buf_n is 0 or we will end up running filters with
		// multiple inputs twice
		if(current_filter->next != NULL && current_filter->next_buf_n == 0){
			enqueue(q, current_filter->next);
		}
		if(current_filter->next2 != NULL && current_filter->next2_buf_n == 0){
			enqueue(q, current_filter->next2);
		}

		move_to_next(q);
	}

	#if DEBUG==1
	tty_writeln("Finished filter queuing");
	#endif

	return 0;
}

// marked as inline to allow for compiler optimizations
// Loop through filters, applying each one.
inline void filter_loop()
{
	#if DEBUG==1 && TRACE==1
	tty_writeln("ADC read");
	#endif

	// Operating on a queue of filter structs, loop through applying the functions.
	struct filter *current_filter;
	move_to_start(q);
	while((current_filter = get_element(q)))
	{
		current_filter->filter_function(current_filter);
		move_to_next(q);
	}

	#if DEBUG==1 && TRACE==1
	tty_writeln("Filter done");
	#endif
}

#endif
