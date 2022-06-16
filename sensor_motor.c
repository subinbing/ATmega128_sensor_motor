#define F_CPU 7372800UL

#include <avr/io.h>

#include <util/delay.h>

#include <avr/interrupt.h>

​

volatile int step_MOTOR[2][8] = { {0x08, 0x04, 0x02, 0x01, 0x08, 0x04, 0x02, 0x01}, //1상 여자 방식 (시계 방향)

{0x01, 0x02, 0x04, 0x08, 0x01, 0x02, 0x04, 0x08}, //1상 여자 방식 (반시계 방향)

{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} }; //스텝 모터 정지

​

int i, a, led;

volatile unsigned int distance = 0, flag = 0, count = 0;

​

void setting() { // 초음파 센서

	TCCR3B = 0x0a; //0b00001010 CTC모드, 분주비 87

	ETIMSK = 0x10; //0b00010000 비교일치 인터럽트 허용

	EICRB = 0x03; //0b00000011 상승 모서리에서 인터럽트 요구

	EIMSK = 0x10; //0b00010000 INT4 인터럽트 허용

	OCR3A = 0; //비교일치 레지스터 초기화

}

​

void STEP_MOTOR() {

	for (i = 0; i < 8; i++) {

		PORTB = step_MOTOR[a][i];

		_delay_ms(5);

	}

}

​

void LED_SHIFT_RIGHT() {

	PORTA = ~led; //PORTA에 led 출력

	led = led >> 1; //오른쪽으로 시프트

	if (led < 0x01) led = 0x80; //led이 1번째까지 오면 led을 다시 초기화

}

​

void LED_SHIFT_LEFT() {

	PORTA = ~led; //PORTA에 led 출력

	led = led << 1; //왼쪽으로 시프트

	if (led > 0x80) led = 0x01; //led가 8번째까지 오면 led를 다시 초기화

}

​

ISR(TIMER3_COMPA_vect) { //타이머카운터3 비교일치모드

	if (count < 10) { //10uS가 되기 전

		PORTE = 0x08; //E3(Trigger)

		OCR3A = 1; //1us

	}

	else { // 10uS가 되었을 때

		PORTE = 0x00;

		OCR3A = 55296; //(7372800/8)*0.06=55296 (측정 주기: 60ms 이상으로 할 것-데이터시트)

		count = 0;

	}

	count++;

}

​

ISR(INT4_vect) { //E4(Echo)

	if (flag == 0) {

		TCNT3 = 0; // TCNT값 리셋

		EICRB = 0x02; // 하강모서리 인터럽트

		flag = 1;

	}

	else {

		distance = TCNT3 / 58; //에코 신호 수신 공식:uS/58=cm

		EICRB = 0x03; //상승모서리 인터럽트

		flag = 0;

	}

}

​

int main(void) {

	DDRA = 0Xff; //led 출력

	DDRB = 0x0f; //PORTB0~PORTB3 (스텝모터)

	DDRD = 0x00; //D0,D1,D2 스위치 입력

	DDRE = 0x08; //E3 트리거 입력, E4 에코 출력

	PORTA = 0x55; //초기상태 led

	​

		setting();

	sei(); //전역 인터럽트 허용

	​

		while (1) {

			if ((~PIND & 0X0F) == 0x01) { //0번째 스위치를 누르면

				if (distance < 20) { //20보다 작으면

					_delay_ms(5);

					LED_SHIFT_RIGHT(); //led를 오른쪽으로 시프트

					a = 0; //시계 방향(1상 여자 방식)

					STEP_MOTOR();

				}

				else if (distance == 20) { //20이면

					_delay_ms(5);

					PORTA = ~0x00; //led 정지

					a = 2; //정지

					STEP_MOTOR();

				}

				else if (distance > 30) { //30보다 크면

					_delay_ms(5);

					LED_SHIFT_LEFT(); //led를 왼쪽으로 시프트

					a = 1; //반시계 방향(1상 여자 방식)

					STEP_MOTOR();

				}

			}

			else if ((~PIND & 0X0F) == 0x02) { //1번째 스위치를 누르면

				if (distance <= 30) { //30 이하이면

					a = 0; //시계 방향(1상 여자 방식)

					STEP_MOTOR();

				}

				else if (distance > 30 && distance < 60) { // 30보다 크고 60보다 작으면

					a = 1; //반시계 방향(1상 여자 방식)

					STEP_MOTOR();

				}

			}

		}

}