// Created 2014-01-17 by Andrei Zisu
// Modified 2014-02-18 by Andrei Zisu
//  - Interface
// Modified 2014-02-18 by Michael Mokrysz
//	- Alloc stuff
// Modified 2014-03-06 by Andrei Zisu
//	- Frequency set/get
// Modified 2014-03-07 by Alex Oyston
//	- Load/save commands
// Modified 2014-03-07 by Andrei Zisu
//	- Download command
// Modified 2014-03-07
//  - debugging
// Modified 2014-03-09 by Michael Mokrysz
//  - Added queue stuff
// Modified 2014-03-09
//  - Improvements
// Modified 2014-03-11 by Michael Mokrysz
//  - Added scramble

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

#include "adc.c"
#include "alloc.c"
#include "board.c"
#include "dac.c"
#include "filter_chain.c"
#include "iap.c"
#include "queue.c"
#include "scramble.c"
#include "serial.c"
#include "timer.c"

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

	filters_count = 5;

	uint16_t n = 0; //used to stop me making errors changing the filters

	filters_buf[n*8 + 0] = 0;
	filters_buf[n*8 + 1] = 0;
	filters_buf[n*8 + 2] = 2;
	filters_buf[n*8 + 3] = 3;
	filters_buf[n*8 + 4] = 0;
	filters_buf[n*8 + 5] = 0;
	filters_buf[n*8 + 6] = 0;
	filters_buf[n*8 + 7] = 0;
	n++;

	filters_buf[n*8 + 0] = 2;
	filters_buf[n*8 + 1] = 2;
	filters_buf[n*8 + 2] = 4;
	filters_buf[n*8 + 3] = 0;
	filters_buf[n*8 + 4] = 0;
	filters_buf[n*8 + 5] = 0;
	filters_buf[n*8 + 6] = 0;
	filters_buf[n*8 + 7] = 0;
	n++;

	filters_buf[n*8 + 0] = 2;
	filters_buf[n*8 + 1] = 3;
	filters_buf[n*8 + 2] = 4;
	filters_buf[n*8 + 3] = 0;
	filters_buf[n*8 + 4] = 0;
	filters_buf[n*8 + 5] = 0;
	filters_buf[n*8 + 6] = 0;
	filters_buf[n*8 + 7] = 0;
	n++;

	filters_buf[n*8 + 0] = 9;
	filters_buf[n*8 + 1] = 4;
	filters_buf[n*8 + 2] = 1;
	filters_buf[n*8 + 3] = 0;
	filters_buf[n*8 + 4] = 50;
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

	//scramble_init();
	//tty_writeln("Scramble Init");
	//scramble_enable();
	//while(1); // @TODO: Make scramble work alongside main function

	#if DEBUG==1
		do_debug();
	#else
		do_passthrough();
	#endif

	#if DEBUG==1
	tty_writeln("Passthrough Init");
	#endif

	while(1)  {
		if(!skip_reading)
			tty_read_blocking(read_buffer, 16);
		skip_reading = 0;

		#if DEBUG==1 && TRACE==1
		tty_writeln("While");
		#endif

		if(read_buffer[0] == REPL_NOOP_COMMAND) {
			// noop command used to check the connection status when the GUI connects
			tty_writeln("Noop");
		}

		if(read_buffer[0] == REPL_HALT_COMMAND) {
			// halt command, enables passthrough
			do_passthrough();
			tty_writeln("Halt");
		}

		if(read_buffer[0] == REPL_GET_COMMAND) {
			sprintf(s, "Get:%d", frequency);
			tty_writeln(s);
		}

		// Download current filter chain to GUI
		if(read_buffer[0] == REPL_DOWNLOAD_COMMAND) {
			// download the filter that is currently loaded into memory (including passthrough)

			#if DEBUG==1
			tty_writeln("Downloading filter chain to gui");
			#endif

			sprintf(s, "Download:%d", filters_count);
			tty_writeln(s);

			int i=0;
			while(i < filters_count){
				tty_read_blocking(read_buffer, 16);
				if(read_buffer[0] == REPL_DOWNLOAD_COMMAND) {
					sprintf(s, "Download:%c%c%c%c%c%c%c%c",
									filters_buf[i*8 + 0]+32, //adding 32 because python is fussy with special characters
									filters_buf[i*8 + 1]+32,
									filters_buf[i*8 + 2]+32,
									filters_buf[i*8 + 3]+32,
									filters_buf[i*8 + 4]+32,
									filters_buf[i*8 + 5]+32,
									filters_buf[i*8 + 6]+32,
									filters_buf[i*8 + 7]+32);

					tty_writeln(s);

					if(i == filters_count-1) {
						#if DEBUG==1
						tty_writeln("Filter download done");
						#endif

						break;
					}
				} else {
					skip_reading = 1; //the command was invalid, which probably means the interface or connection crashed
					break;
				}


				i++;
			}
		}

		// Load a filter chain from the flash memory, initialise and run it
		if(read_buffer[0] == REPL_LOAD_COMMAND) {
			uint8_t block = read_buffer[1];
			if(block > 9){ //10 blocks available to load from
				#if DEBUG==1
				tty_writeln("ERROR: Requested block does not exist");
				#else
				tty_writeln("ERROR");
				#endif

				break;
			}

			// store so we can recover in case of failure of retrieval
			uint16_t fc = filters_count;

			get_filter_chain_size(&filters_count, block);


			if(filters_count == 0){
				#if DEBUG==1
				tty_writeln("Requested block is empty");
				#else
				tty_writeln("Block empty");
				#endif

				filters_count = fc;
			}else{
				timer_stop();

				// copy filters_count here as it is wiped by free_all_filters()
				fc = filters_count;

				free_all_filters();
				free_all_buf();

				// reset filters_count to what it was before
				filters_count = fc;

				flash_to_filter_chain(filters_buf, filters_count, block);

				filter_init(filters_buf, filters_count);

				timer_start();

				tty_writeln("Loaded");
			}
		}

		// Save a filter chain to flash
		// Can only save to each block once due to flash sectors not erasing
		if(read_buffer[0] == REPL_SAVE_COMMAND) {
			uint8_t block = read_buffer[1];

			if(block > 9){ //10 blocks available to load from
				#if DEBUG==1
				tty_writeln("ERROR: Requested block does not exist");
				#else
				tty_writeln("ERROR");
				#endif

				break;
			}
			//Save into requested block
			if(filter_chain_to_flash(filters_buf, &filters_count, block) != 0){
				#if DEBUG==1
				tty_writeln("ERROR: Block has already been written to");
				#else
				tty_writeln("Error");
				#endif
			} else {
				tty_writeln("Saved");
			}
		}

		if(read_buffer[0] == REPL_SET_COMMAND) {
			// command that gets the frequency from CLI and sets
			int index = 1;
			frequency = 0;

			while(read_buffer[index] != 0) { //if not EOL
				frequency = frequency*10+(read_buffer[index]-'0');

				index++;
			}

			timer_stop(); //@TODO test if it is really needed
			timer_init(frequency);
			timer_start(); //@TODO test if it is really needed

			tty_writeln("Set");
		}

		if(read_buffer[0] == REPL_APPLY_COMMAND) {
			// command that gets a filter chain from CLI and applies it
			timer_stop();

			free_all_filters();
			free_all_buf();

			tty_writeln("Apply");

			filters_count = 0;

			while(1) {
				tty_read_blocking(read_buffer, 16);

				if(read_buffer[0]== REPL_APPLY_COMMAND) {
					uint16_t error =  filter_init(filters_buf, filters_count);
					if(error == 0) {
						timer_start();

						tty_writeln("End filters");
					} else {
						sprintf(s, "Error: %d", error);

						tty_writeln(s);
					}

					break;
				} else if(read_buffer[0]== REPL_FILTER_COMMAND) {
					// the first filter is always the INPUT!

					// (int)read_buffer[1] is filter function index
					// (int)read_buffer[2] filter id (unique) - 1
					// (int)read_buffer[3,4] filter ids to output to, it is limited to 2 for simplicity - 2,3
					// each of the following bytes can be a parameter (the gui takes 4 values of 0-254 !!! 254, not 255!!! comma sepparated)
					// next_id of 0x00 implies there is no next filter - filter->next = NULL (next filter is set to the input filter, which is obviously not taking any buffer input)

					/*
						filters_buf[n*8 + 0] = filter_type
						filters_buf[n*8 + 1] = unique_filter_id
						filters_buf[n*8 + 2] = output_filter_id #1
						filters_buf[n*8 + 3] = output_filter_id #2
						filters_buf[n*8 + 4] = parameter 0
						filters_buf[n*8 + 5] = parameter 1
						filters_buf[n*8 + 6] = parameter 2
						filters_buf[n*8 + 7] = parameter 3
					*/

					int i;
					for(i=0; i<8; i++) {
						filters_buf[filters_count*8 + i] = read_buffer[i+1];
					}

					filters_count++;

					tty_writeln("Filter");
				} else {
					skip_reading = 1; //the command was invalid, which probably means, the interface or connection crashed
					break;
				}
			}
		}
	}

	#if DEBUG==1 && TRACE==1
	tty_writeln("Escaped loop. This should not happen");
	#endif

	while(1);
}
