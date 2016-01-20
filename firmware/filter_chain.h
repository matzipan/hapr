#ifndef _HAPR_FC_H
#define _HAPR_FC_H

uint16_t filters_count;
uint16_t filters_buf[2048];

struct filter *head_filter;

#define REGULAR_BUFFER_SIZE 16 
#define DELAY_BUFFER_SIZE 4096
#define FLANGE_BUFFER_SIZE 256
#define PASS_BUFFER_SIZE 32
#define NOISE_BUFFER_SIZE 2048

void input_function(struct filter *filter);
void output_function(struct filter *filter);
void passthrough_function(struct filter *filter);
void zero_function(struct filter *filter);
void max_function(struct filter *filter);
void min_function(struct filter *filter);
void min_function(struct filter *filter);
void sine_function(struct filter *filter);
void reverb_function(struct filter *filter);
void delay_function(struct filter *filter);
void mix_function(struct filter *filter);
void tremolo_function(struct filter *filter);
void flange_function(struct filter *filter);
void upward_compressor_function(struct filter *filter);
void downward_compressor_function(struct filter *filter);
void n_bits_function(struct filter *filter);
void distortion_function(struct filter *filter);
void triangle_function(struct filter *filter);
void noise_cancellation_function(struct filter *filter);
void aec_lowpass_function(struct filter *filter);
void aec_highpass_function(struct filter *filter);
void aec_allpass_function(struct filter *filter);

void (*filter_functions[21])(struct filter *filter) = {
	input_function,					//0
	output_function,				//1
	passthrough_function, 			//2
	zero_function,					//3
	max_function,					//4
	min_function,					//5
	sine_function,					//6
	reverb_function,				//7
	delay_function,					//8
	mix_function,					//9
	tremolo_function,				//10
	flange_function,				//11
	upward_compressor_function,		//12
	downward_compressor_function,	//13
	n_bits_function, 				//14
	distortion_function, 			//15
	triangle_function, 				//16
	noise_cancellation_function,	//17
	aec_lowpass_function,			//18
	aec_highpass_function,			//19
	aec_allpass_function,			//20
};

// To ensure filters can access a good number of previous outputs, filters
// should have a buffer size of at least 16.
// They need to be a power of two
uint32_t filter_function_sizes[21] = {
	REGULAR_BUFFER_SIZE,		//0
	REGULAR_BUFFER_SIZE,		//1
	REGULAR_BUFFER_SIZE,		//2
	REGULAR_BUFFER_SIZE,		//3
	REGULAR_BUFFER_SIZE,		//4
	REGULAR_BUFFER_SIZE,		//5
	REGULAR_BUFFER_SIZE,		//6
	DELAY_BUFFER_SIZE,			//7
	DELAY_BUFFER_SIZE,			//8
	REGULAR_BUFFER_SIZE,		//9
	REGULAR_BUFFER_SIZE,		//10
	FLANGE_BUFFER_SIZE,			//11
	REGULAR_BUFFER_SIZE,		//12
	REGULAR_BUFFER_SIZE,		//13
	REGULAR_BUFFER_SIZE,		//14
	REGULAR_BUFFER_SIZE,		//15
	REGULAR_BUFFER_SIZE,		//16
	NOISE_BUFFER_SIZE,			//17
	PASS_BUFFER_SIZE,			//18
	PASS_BUFFER_SIZE,			//19
	PASS_BUFFER_SIZE,			//20
};

uint16_t filter_init(uint16_t *filters_buf, uint16_t filters_count);
inline void filter_loop();

#endif