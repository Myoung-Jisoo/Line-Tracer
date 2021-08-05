//mcu_init.c
#include "mcu_init.h"

void UART1_INIT(){ // UART 초기 설정
	DDRD = 0b00001000; // PD2에 있는 RX는 입력 PD3에 있는 TX는 출력
	
	UCSR1A = 0x00; // 전송속도 2배 설정안함
	UCSR1B = 0b00011000; // RX Complete Interrupt 사용 안하고 RX, TX Enable 및 8bit 데이터 사용
	UCSR1C = 0b00000110; // 비동기 모드, Non parity, Stop bit 1, 8bit 사용
	
	UBRR1H = 0;
	UBRR1L = 103; // Board rate 9600
}

void UART1_Transmit(unsigned char cData){ // UART 수신
	while(!(UCSR1A & (1<<UDRE1)));
	UDR1 = cData;
}

unsigned char UART1_Receive(){ // UART 송신
	while(!(UCSR1A & (1<<RXC1)));
	return UDR1;
}

void Timer2_INIT(){
	TCCR2 = (0<<WGM21) | (0<<WGM20) | (0<<COM21) | (0<<COM20) | (1<<CS22) | (0<<CS21) | (0<<CS20);
	// Nomal, Normal port operation, Clear OC2 on compare match, prescaler 256
	TIMSK = (1<<TOIE2); // T/C2 Overflow Interrupt Enable
	TCNT2 = 131;
}

void BUTTON_INIT()
{
	DDRD = 0x00; // PD핀 입력
	
	EIMSK = (1<<INT0); // Interrupt 0번 활성화
	EICRA = (1<<ISC01) | (0<< ISC00); // falling edge 때 실행
}

void MOTOR_INIT(){ // 모터 초기 설정
	DDRE = 0x0f;
	DDRB = 0xff;
	
	TCCR1A = (1<<COM1A1) | (0<<COM1A0) | (1<<COM1B1) | (0<<COM1B0) | (1<<WGM11);
	TCCR1B = (1<<WGM13) | (1<<WGM12) | (0<<CS12) | (0<<CS11) | (1<<CS10);
	// channel A와 B non-inverting mode, Fast PWM, clk 1 (No prescaling)
	
	ICR1 = 4999;// TOP
	OCR1A = 0;
	OCR1B = 0;
}

int MOTOR_Direction(double Weight, int line, int uturn){ // 모터 구동
	if(line > 0) // 수발광 센서 하나라도 검은선에 걸친 경우
	{
		if((uturn > 0) && (line > 5)) // 결승점 세개 선에서 정지한 후에 검은줄을 만난 경우 유턴
		{
			PORTE = 0x0a;
			OCR1A = 4999 * 0.48; // 오른쪽
			OCR1B = 4999 * 0.48; // 왼쪽
			_delay_ms(980);
			uturn = -1; // uturn에 -1값을 넣어 반환할 것임
		}
		
		if(line == 1) Weight *= 2; // 줄이 2줄 미만, 1줄만 걸친 경우 가중치 2배
		PORTE = 0x06; // 전진 설정
		if((Weight < 3) && (Weight > -3)) // 가중치가 -2 ~ +2 인 경우
		{
			OCR1B = 4999 * 0.45; // 왼쪽
			OCR1A = 4999 * 0.45; // 오른쪽
		}
		else
		{
			if(Weight < 0) // 우회전
			{
				if((Weight < 7) && (Weight > -7)) OCR1B = 4999 * 0.28; // 가중치가 작을 경우 왼쪽 모터도 살짝 돌려준다.
				else OCR1B = 0;
				OCR1A = (4999 * 0.26) - (4999 * 0.17 * (Weight / 15));
			}
			else // 좌회전
			{
				if((Weight < 7) && (Weight > -7)) OCR1A = 4999 * 0.28; // 가중치가 작을 경우 오른쪽 모터도 살짝 돌려준다.
				else OCR1A = 0;
				OCR1B = (4999 * 0.26) + (4999 * 0.17 * (Weight / 15));
			}
		}
	}
	UART1_Transmit(' ');
	UART1_TransNum(line);
	return uturn; // uturn값을 반환
}

void ADC_INIT(){
	DDRF = 0x00;
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); // ADC Enable, 분주비 128
}

void ADC_Receive(int array[]){ // 8개의 adc값을 차례로 받아온다.
	for(int i = 0; i < 8; i++)
	{
		ADCSRA |= (1<<ADSC);
		ADMUX = i;
		
		while(!(ADCSRA & (1<<ADIF)));
		array[i] = ADC;
	}
}
