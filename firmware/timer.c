#ifndef _HAPR_TIMER
#define _HAPR_TIMER

#include "LPC17xx.h"
#include "lpc_types.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_dac.h"

#include "adc.h"
#include "scramble.h"
#include "filter_chain.h"
#include "timer.h"

uint16_t cycle = 0;
uint16_t frequency = 20000;

void timer_init(int frequency)
{
	TIM_TIMERCFG_Type TIM_ConfigStruct;
	TIM_MATCHCFG_Type TIM_MatchConfigStruct;

	PINSEL_CFG_Type PinCfg;

	PinCfg.Funcnum = 3;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 1;
	PinCfg.Pinnum = 28;
	PINSEL_ConfigPin(&PinCfg);

	TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
	TIM_ConfigStruct.PrescaleValue	= 10;
	TIM_MatchConfigStruct.MatchChannel = 0;
	// Enable interrupt when MR0 matches the value in TC register
	TIM_MatchConfigStruct.IntOnMatch = TRUE;
	// Enable reset on MR0: TIMER will reset if MR0 matches it
	TIM_MatchConfigStruct.ResetOnMatch = TRUE;
	// Stop on MR0 if MR0 matches it
	TIM_MatchConfigStruct.StopOnMatch = FALSE;
	// Toggle MR0.0 pin if MR0 matches it
	TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	// Set Match value, count value of 100000 (100000 * 10uS = 1000000us = 1s --> 1 Hz)
	TIM_MatchConfigStruct.MatchValue = (int) 100000.0 / frequency;

	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);
	TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
	// preemption = 1, sub-priority = 1
	NVIC_SetPriority(TIMER0_IRQn, ((0x01<<3)|0x01));
}

void timer_start() {
	TIM_Cmd(LPC_TIM0,ENABLE);
	NVIC_EnableIRQ(TIMER0_IRQn);
}

void timer_stop() {
	NVIC_DisableIRQ(TIMER0_IRQn);
	TIM_Cmd(LPC_TIM0,DISABLE);
}

void TIMER0_IRQHandler ()
{
	if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) {
		NVIC_DisableIRQ(TIMER0_IRQn); //disable interrupt, to prevent it from overlapping if it takes too long to process
    	TIM_ClearIntPending(LPC_TIM0,TIM_MR0_INT);
        TIM_ResetCounter(LPC_TIM0);

        #if DEBUG==1 && TRACE==1
		tty_writeln("Timer trigger");
		#endif

		if (scramble_mode) {
			scramble_timer_handler();
		} else {
	        filter_loop();
	    }

        cycle++; //used for sine wave generation
		if(cycle > frequency)
			cycle = 0;

        NVIC_EnableIRQ(TIMER0_IRQn);
	}
}

#endif
