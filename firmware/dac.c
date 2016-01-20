// Modified 2014-01-28 by Andrei Zisu
//  - Restructure
// Modified 2014-03-05 by Andrei Zisu
//  - Refactoring and fix allocation
// Modified 2014-03-11 by Alex Oyston
//  - Added global cycle increment
// Modified 2014-03-11 by Andrei Zisu
//  - Cleanup

#ifndef _HAPR_DAC
#define _HAPR_DAC

#include "LPC17xx.h"
#include "lpc_types.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_dac.h"

#include "dac.h"

// marked as inline to allow compiler optimizations
inline void dac_set_value(uint16_t value) {
	DAC_UpdateValue(LPC_DAC, value >> 2);
}

void dac_init()
{
  PINSEL_CFG_Type PinCfg;

  PinCfg.Funcnum = 2;
  PinCfg.OpenDrain = 0;
  PinCfg.Pinmode = 0;
  PinCfg.Portnum = 0;
  PinCfg.Pinnum = 26;
  PINSEL_ConfigPin(&PinCfg);

  DAC_Init(LPC_DAC);
}

#endif
