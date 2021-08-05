//main.c
#include "mcu_init.h"

char a;
int uturn = 0;
int cnt = 0;
int timee = 0;
int button = 0;
int flag0 = 0;
int flag = 0;
int number = 0;
int sign = 0;
int adc_array[8] = {0, };
int adc_max[8] = {0, };
int adc_min[8] = {1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023};
int line[8] = {0, };
int line_s = 0;
double sigma_R = 0;
double sigma_L = 0;

ISR(INT0_vect) // 버튼 0번을 누르면 Interrupt 0번 활성화
{
	button++; // 전역변수 button 1증가
}

ISR(TIMER2_OVF_vect) // T/C2 사용 Overflow Interrupt Routine
{
	cnt++;
	TCNT0 = 131; // 주기 2ms
	
	if(cnt == 10) // 20ms 마다
	{
		cnt = 0;
		timee++;
		if(line_s > 4) { flag0++; timee = 0; } // 검은줄로 가려진 센서가 5개 이상이면 flag0을 1증가, 시간 연장
		if((line_s < 4) && (flag0 > 0)) { flag0 = 0; flag++; } // flag0이 0 이상이면서 검은줄로 가려진 센서가 5개 미만이면 flag를 1증가, flag0을 0으로 초기화
		// 검은색 줄을 지나갈 때를 체크함
	}
	
	if(flag > 2) // flag가 3이상(짧은 시간 내에 줄을 3개 지났을 때)이면 
	{
		if(uturn == -1) // 이미 유턴을 한 경우 정지 (모터 출력핀을 끈다.)
		{
			OCR1A = 0;
			OCR1B = 0;
			DDRE = 0x00;
			DDRB = 0x00;
		}
		else // 다음에 검은줄을 만났을 때 유턴
		{
			PORTE = 0x06;
			OCR1A = 0;
			OCR1B = 0;
			_delay_ms(3000);
			OCR1B = 4999 * 0.47;
			OCR1A = 4999 * 0.47;
			_delay_ms(250);
			flag = 0;
			uturn = 1;
		}
	}
	
	if(timee > 23) { timee = 0; flag = 0; } // 460ms가 지나도록 다음 검은줄을 지나지 못한다면 flag를 0으로 초기화
}

void UART1_TransNum(int num) //숫자를 uart로 출력
{
	int j;
	if(num < 0)
	{
		UART1_Transmit('-');
		num = -num;
	}
	for(j = 100 ; j > 0; j /= 10)
	{
		UART1_Transmit((num/j) + 48);
		num %= j;
	}
	UART1_Transmit(' ');
}

void Normalization(int array[], int max[], int min[]){ 
	double numerator = 0; // 분자
	double denominator = 0; // 분모
	
	for(int i = 0; i < 8; i++)
	{
		numerator = array[i] - min[i];
		denominator = max[i] - min[i];
		array[i] = (numerator / denominator) * 100; // ADC값 정규화
		if(array[i] <= 50) line[i] = 1; // 검은색이면 1 흰색이면 0으로 배열 line에 저장
		else line[i] = 0;
	}
}

void Weighted_Data_Processing(){ // 가중치
	int j = 0;
	sigma_L = 0; // 좌측 ADC 가중치
	sigma_R = 0; //우측 ADC 가중치
	for(int i = 0; i < 4; i++) sigma_L -= (line[i] * (1<<(3 - i))); // -8, -4, -2, -1
	for(int i = 4; i < 8; i++) sigma_R += (line[i] * (1<<(i - 4))); // 1, 2, 4, 8
 	UART1_Transmit(' ');
	UART1_TransNum(sigma_R + sigma_L); // 총 가중치를 uart로 출력
}

int main(void)
{
	DDRA = 0xff;
	UART1_INIT();
	MOTOR_INIT();
	ADC_INIT();
	BUTTON_INIT();
	Timer2_INIT();
	
	sei();
	
	while (1)
	{
		ADC_Receive(adc_array); // ADC값을 받아온다.
		
		if(button == 1) // 버튼을 한 번 누르면 ADC 최대 최소값 받음
		{
			PORTA = 0x55;
			for(int i = 0; i < 8; i++)
			{
				if(adc_array[i] > adc_max[i]) adc_max[i] = adc_array[i];
				if(adc_array[i] < adc_min[i]) adc_min[i] = adc_array[i];
			}
		}
		else if(button >= 2) // 버튼을 한 번 더 누르면
		{
			PORTA = ~(1 << flag);
			Normalization(adc_array, adc_max, adc_min); // 정규화
			line_s = 0;
			for(int i = 0; i < 8; i++)
			{
				UART1_Transmit(line[i] + 48); // 각 센서가 검은줄 위에 있는지 0/1로 uart로 출력
				line_s += line[i]; // 검은줄 위에 있는 수발광 센서의 수
			}
			if((sigma_L + sigma_R) != 0) uturn = MOTOR_Direction((sigma_R + sigma_L), line_s, uturn); // 가중치 가 0이 아닐 경우 모터 방향 변경
			//uturn = MOTOR_Direction((sigma_R + sigma_L), line_s, uturn); // 수정
			// 문제점 1. 가중치가 0일 경우를 제외하였기 때문에 직진을 바르게 하지 못했음.
		}
		UART1_Transmit(' '); // 띄어쓰기
		Weighted_Data_Processing(); // 가중치
		UART1_Transmit(13); // uart 줄넘김
	}
}

// 문제점 : 가중치가 0일 경우를 제외하였기 때문에 직진을 바르게 하지 못했음.
/*
 완주 실패 원인 : 주행중 모터가 멈췄다.
 1. Atmega128 과열로 인함
 2. 가중치가 0인 경우를 제외시켰기 때문에(그렇지만 멈춘 이유는 아님)
 3. 전원부 모듈의 몰렉스가 약해져 연결이 불안정하기 때문
*/