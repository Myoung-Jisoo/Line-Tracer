//mcu_init.h
#ifndef UART_INIT_H_
#define UART_INIT_H_

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>

void UART1_INIT();								// UART1을 초기화 하기 위한 함수
void UART1_Transmit(unsigned char cData);		// 데이터 송신 함수
unsigned char UART1_Receive();					// 데이터 수신 함수

#endif /* UART_INIT_H_ */