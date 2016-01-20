#ifndef _HAPR_SERIAL
#define _HAPR_SERIAL

#include "LPC17xx.h"
#include "lpc_types.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"

#include "serial.h"

int tty_read_blocking(char *buf,int length)
{
	return(UART_Receive((LPC_UART_TypeDef *)LPC_UART0, (uint8_t *)buf, length, BLOCKING));
}

int tty_write_blocking(char *buf,int length)
{
	return(UART_Send((LPC_UART_TypeDef *)LPC_UART0,(uint8_t *)buf,length, BLOCKING));
}

void tty_write(char *buf){
	int length = 0;
	uint8_t chr = buf[0];
	while(chr != '\0'){
		length++;
		chr = buf[length];
	}
	tty_write_blocking(buf, length);
}

void tty_writeln(char *buf){
	tty_write(buf);

	// for some reason, there is a bug in the pySerial library...
	// it does not recognize neither \n or \r as line breaks
	// it just fetches until timeout
	// so we're not printing EOL characters, just for easier interfacing
	#if DEBUG==1
	tty_write_blocking("\n\r", 2);
	#endif
}

void tty_write_binaryten(uint32_t n) {
	int k = 0;
	char chars[35];
	chars[0] = '0';
	chars[1] = 'b';
	chars[34] = 0;
	for(k = 0; k < 32; k++) {
		if ((n & ( 1 << k)) >> k == 1) {
			chars[33-k] = '1';
		} else {
			chars[33-k] = '0';
		}
	}
	tty_writeln(chars);
}

void tty_write_binary(int n) {
	tty_write_binaryten(n);
	return;
	int k = 0;
	char chars[11];
	chars[0] = '0';
	chars[1] = 'b';
	chars[11] = '\0';
	for(k = 7; k >= 0; k--) {
		if ((n & ( 1 << k)) >> k == 1) {
			chars[11-(k+2)] = '1';
		} else {
			chars[11-(k+2)] = '0';
		}
	}
	tty_writeln(chars);
}

void tty_write_int(int n) {
  char chars[32];
  int i;
  for (i = 0; i < 29; i++) {
    chars[i] = ' ';
  }
  chars[29] = 0;
  i = 0;
  while (n > 0 || i == 0) {
    i++;
    chars[29 - i] = (n % 10) + '0';
    n = n / 10;
  }
  tty_write(chars);
}

void tty_writeln_int(int n) {
  tty_write_int(n);
  tty_writeln("");
}

void serial_init()
{
	UART_CFG_Type UARTConfigStruct; // UART Configuration structure variable
	UART_FIFO_CFG_Type UARTFIFOConfigStruct; // UART FIFO configuration Struct variable
	PINSEL_CFG_Type PinCfg; // Pin configuration for UART

	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;

	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 3;
	PINSEL_ConfigPin(&PinCfg);

	UARTConfigStruct.Baud_rate = 9600;
	UARTConfigStruct.Databits = 8;
	UARTConfigStruct.Stopbits = 1;
	UARTConfigStruct.Parity = UART_PARITY_NONE;

	UART_ConfigStructInit(&UARTConfigStruct);

	UARTFIFOConfigStruct.FIFO_DMAMode = DISABLE;
	UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV0;
	UARTFIFOConfigStruct.FIFO_ResetRxBuf = ENABLE;
	UARTFIFOConfigStruct.FIFO_ResetTxBuf = ENABLE;

	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);

	UART_Init((LPC_UART_TypeDef *)LPC_UART0, &UARTConfigStruct);

	UART_FIFOConfig((LPC_UART_TypeDef *)LPC_UART0, &UARTFIFOConfigStruct);

	UART_TxCmd((LPC_UART_TypeDef *)LPC_UART0, ENABLE);
}

#endif
