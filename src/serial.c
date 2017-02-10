/**
  ******************************************************************************
  * File Name          : serial.c
  * Description        : Serial buffer handling
  ******************************************************************************
  *
  * Copyright (c) 2016 S54MTB
  * Licensed under Apache License 2.0 
  * http://www.apache.org/licenses/LICENSE-2.0.html
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"
#include <string.h>
#include "serial.h"
#include "setup.h"

/**
 * Command line edit chars
 */
#define CNTLQ      0x11
#define CNTLS      0x13
#define DEL        0x7F
#define BACKSPACE  0x08
#define CR         0x0D
#define LF         0x0A


/** Serial driver globals */
static uint8_t line_flag; 			// Flag to indicate new received line
char line_buf[256];
int line_idx = 0;


extern UART_HandleTypeDef huart2;

/**
 * Process received char, check if LF or CR received
 * Set flag when line is done
 */
void process_rx_char(char rx_char)
{
	if (rx_char == CR)  rx_char = LF;   
    if (rx_char == BACKSPACE  ||  rx_char == DEL) 
    {    // process backspace
      	if (line_idx != 0)  
      	{            
        	line_idx--;                      // decrement index
        	#ifdef LOCALECHO
        		putchar (BACKSPACE);               // echo backspace
        		putchar (' ');
        		putchar (BACKSPACE);
        	#endif
      	}
    }
    else 
    {
      	#ifdef LOCALECHO
      		putchar (rx_char);                   // echo 
      	#endif 
      	line_buf[line_idx++] = rx_char; 	   // store character and increment index
    }
    
    // check limit and end line feed
  	if ((line_idx == 0xff)  ||  (rx_char == LF))
  	{
  		line_buf[line_idx-1] = 0;                  // mark end of string
  		line_idx = 0;
			line_flag = 1;
  	}

}

uint8_t IsLineReceived(void)
{
	uint8_t x = line_flag;
	line_flag = 0;
	return x;
}


void uart_puts(char *str)
{
	uint16_t len = strlen(str);
	HAL_UART_Transmit_IT(&huart2, (uint8_t *)str, len);
	HAL_Delay(len*2);  // foolproof
}


void uart_putchar(char ch)
{
	HAL_UART_Transmit_IT(&huart2, (uint8_t *)&ch, 1);
	HAL_Delay(2);
}

char *get_line_buffer(void)
{
	return line_buf;
}

