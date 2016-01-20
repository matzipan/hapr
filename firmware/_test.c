#define DEBUG 0 //used to print debug messages, cannot be used in cojunction with the GUI
#define TRACE 0 //used to print filter tracing messages, can only be used in debug mode

#include "lpc_types.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "math.h"
#include "string.h"

#define REPL_HALT_COMMAND 'h'
#define REPL_GET_COMMAND 'g'
#define REPL_SET_COMMAND 's'
#define REPL_APPLY_COMMAND 'a'
#define REPL_DOWNLOAD_COMMAND 'd'
#define REPL_FILTER_COMMAND 'f'
#define REPL_NOOP_COMMAND '0'
#define REPL_LOAD_COMMAND 'z'
#define REPL_SAVE_COMMAND 'x'

#include "alloc.c"
#include "serial.c"
#include "filter_chain.c"
#include "adc.c"
#include "dac.c"
#include "timer.c"
#include "iap.c"
#include "scramble.c"

void do_passthrough(void)
{
	timer_stop();

	free_all_filters();
	free_all_buf();

	filters_count = 2;

	filters_buf[0] = 0;
	filters_buf[1] = 0;
	filters_buf[2] = 1;
	filters_buf[3] = 0;
	filters_buf[4] = 0;
	filters_buf[5] = 0;
	filters_buf[6] = 0;
	filters_buf[7] = 0;

	filters_buf[8+0] = 1;
	filters_buf[8+1] = 1;
	filters_buf[8+2] = 0;
	filters_buf[8+3] = 0;
	filters_buf[8+4] = 0;
	filters_buf[8+5] = 0;
	filters_buf[8+6] = 0;
	filters_buf[8+7] = 0;

	filter_init(filters_buf, filters_count);

	timer_start();
}

void do_debug()
{
	timer_stop();

	free_all_filters();
	free_all_buf();

	filters_count = 3;

	uint16_t n = 0; //used to stop me making errors changing the filters

	filters_buf[n*8 + 0] = 0;
	filters_buf[n*8 + 1] = 0;
	filters_buf[n*8 + 2] = 2;
	filters_buf[n*8 + 3] = 0;
	filters_buf[n*8 + 4] = 0;
	filters_buf[n*8 + 5] = 0;
	filters_buf[n*8 + 6] = 0;
	filters_buf[n*8 + 7] = 0;
	n++;

	filters_buf[n*8 + 0] = 2;
	filters_buf[n*8 + 1] = 2;
	filters_buf[n*8 + 2] = 1;
	filters_buf[n*8 + 3] = 0;
	filters_buf[n*8 + 4] = 0;
	filters_buf[n*8 + 5] = 0;
	filters_buf[n*8 + 6] = 0;
	filters_buf[n*8 + 7] = 0;
	n++;

	filters_buf[n*8 + 0] = 1;
	filters_buf[n*8 + 1] = 1;
	filters_buf[n*8 + 2] = 0;
	filters_buf[n*8 + 3] = 0;
	filters_buf[n*8 + 4] = 0;
	filters_buf[n*8 + 5] = 0;
	filters_buf[n*8 + 6] = 0;
	filters_buf[n*8 + 7] = 0;
	n++;

	if(n != filters_count){
		#if DEBUG==1
		tty_writeln("UPDATE FILTERS COUNT IN DO_DEBUG!");
		#endif
	}

	filter_init(filters_buf, filters_count);

	timer_start();
}

void main(void)
{
	char read_buffer[16];
	char s[16];

	uint16_t skip_reading = 0;

	serial_init();
	#if DEBUG==1
	tty_writeln("Serial Init");
	#endif

	dac_init();
	#if DEBUG==1
	tty_writeln("DAC Init");
	#endif

	adc_init();
	#if DEBUG==1
	tty_writeln("ADC Init");
	#endif

	timer_init(frequency);
	#if DEBUG==1
	tty_writeln("Timer Init");
	#endif

	alloc_init();
	#if DEBUG==1
	tty_writeln("Alloc Init");
	#endif

	#if DEBUG==1
		do_debug();
	#else
		do_passthrough();
	#endif

	#if DEBUG==1
	tty_writeln("Passthrough Init");
	#endif

	while(1);
}
