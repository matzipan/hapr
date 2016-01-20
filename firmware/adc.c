// Modified 2014-01-28 by Andrei Zisu
//	- Restructured
// Modified 2014-01-30 by Michael Mokrysz
//	- Debugging
// Modified 2014-02-25 by Michael Mokrysz
//	- Renaming
// Modified 2014-02-28 by Alex Oyston
//	- Added stuff for params
// Modified 2014-03-05 by Andrei Zisu
//  - Debugging stuff
//	- Fix allocation stuff
//	- Refactoring
// Modified 2014-03-08 by Alex Oyston
//	- Replaced loop with queue to breadth-first search the filter chain as queue wasn't checking filter->next2
// Modified 2014-03-08 by Michael Mokrysz
//  - Added fix for breadth-first search
// 	- Replaced queue with previous loop until a fix to the error is found
// Modified 2014-03-09 by Michael Mokrysz
//	- Meaning 3 ADC channels to help reduce noise
// Modified 2014-03-10 by Michael Mokrysz
//	- Fixed error with queue and merged
// Modified 2014-03-11 by Michael Mokrysz
//  - Set 2 ADC channels for guitar input and added microphone input channel

#ifndef _HAPR_ADC
#define _HAPR_ADC

#include "LPC17xx.h"
#include "lpc_types.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"

#include "filter_chain.h"
#include "queue.h"

// marked as inline to allow compiler optimizations
inline uint16_t adc_get_data() {
	uint16_t v0 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_0) & 0xFFF;
	uint16_t v1 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_1) & 0xFFF;
	uint16_t v = (v0 + v1) / 2;

	#if DEBUG==1 && TRACE==1
	tty_writeln_int(v);
	#endif
	return v;
}

// Used to sample from microphone for SCRAMBLE functionality.
// marked as inline to allow compiler optimizations
inline uint16_t adc_get_scramble_data() {
	uint16_t v2 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2) & 0xFFF;
	return v2;
}

void adc_init()
{
	PINSEL_CFG_Type PinCfg0;
	PinCfg0.Funcnum = 1;
	PinCfg0.OpenDrain = 0;
	PinCfg0.Pinmode = 0;
	PinCfg0.Portnum = 0;
	PinCfg0.Pinnum = 23;
	PINSEL_ConfigPin(&PinCfg0);
	PINSEL_CFG_Type PinCfg1;
	PinCfg1.Funcnum = 1;
	PinCfg1.OpenDrain = 0;
	PinCfg1.Pinmode = 0;
	PinCfg1.Portnum = 0;
	PinCfg1.Pinnum = 24;
	PINSEL_ConfigPin(&PinCfg1);
	PINSEL_CFG_Type PinCfg2;
	PinCfg2.Funcnum = 1;
	PinCfg2.OpenDrain = 0;
	PinCfg2.Pinmode = 0;
	PinCfg2.Portnum = 0;
	PinCfg2.Pinnum = 25;
	PINSEL_ConfigPin(&PinCfg2);

	uint32_t clockrate = 200000;

	ADC_Init(LPC_ADC, clockrate);
	ADC_BurstCmd(LPC_ADC, ENABLE);

	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_1, ENABLE);
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);
}

#endif
