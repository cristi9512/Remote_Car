#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "usart.h"
#include "usart.c"
#include <stdint.h>
#include <math.h>

#define HC_SR04_NUM_ITERATIONS 		1
#define MINIMUM_SAFE_DISTANCE_STATIC	10.0f
#define MINIMUM_SAFE_DISTANCE_MOVING	50.0f 
static volatile uint8_t flag = 0;
static volatile uint8_t count = 0;
static volatile uint8_t saved_tcnt0 = 0;
static volatile uint8_t saved_count = 0;
static volatile float saved_distance = 0.0f;
static volatile uint8_t timer2_count = 0;
static volatile uint8_t wrong_samples = 0;

volatile uint8_t too_close_flag = 0;

ISR(TIMER0_OVF_vect, ISR_NOBLOCK)
{
	count++;
}

ISR(PCINT0_vect, ISR_NOBLOCK)
{
	if ((PINA & (1 << PA1)) != 0) {
		TCNT0 = 0;
		count = 0;
	}
	else {
		saved_tcnt0 = TCNT0;
		saved_count = count;
		flag = 1;
	}
}

void HC_SR04_init()
{
	too_close_flag = 0;
	wrong_samples = 0;

	TCCR0A = 0;
	TCCR0B = 0;
	TIMSK0 = 0;

	sei(); //Porneste intreruperile
	TCCR0B |= (1 << CS02); //Prescaler = 256 => T = 16us
	TIMSK0 |= (1 << TOIE0);

	//Pini pentru Echo si Trigger
	DDRA |= (1 << PA0); // Trigger
	DDRA &= ~(1 << PA1); // Echo

	PORTA &= ~(1 << PA0);

	//Pin change interrupt pe pinul Echo
	PCICR |= (1 << PCIE0);
	PCMSK0 = 0;
	PCMSK0 |= (1 << PCINT1);
}



float HC_SR04_get_distance()
{
	double sum = 0;
	for (int i = 1; i <= HC_SR04_NUM_ITERATIONS; i++) {
		flag = 0;
		
		//Reset Trigger
		PORTA &= ~(1 << PA0);
		_delay_us(5);

		//Trimite semnal catre Trigger
		PORTA |= (1 << PA0);
		_delay_us(11);
		PORTA &= ~(1 << PA0);
		
		//Asteapta pentru semnalul de la Echo
		while(flag == 0 && count <= 5);
		
		sum = sum + (((int)saved_tcnt0 + 255 * saved_count) * 16.0) * 0.014;
		
		if (i < HC_SR04_NUM_ITERATIONS)
		_delay_ms(75);
	}

	return sum / HC_SR04_NUM_ITERATIONS;
}



ISR(TIMER2_COMPA_vect, ISR_NOBLOCK)
{
	timer2_count++;
	if (timer2_count == 12) {
		TIMSK2 &= ~(1 << OCIE2A);

		float new_distance = HC_SR04_get_distance();
		if (fabs(new_distance - saved_distance) >= 500) {
			wrong_samples++;
			if (wrong_samples >= 5) {
				saved_distance = new_distance;
				wrong_samples = 0;
			}			
		}
		else {
			saved_distance = new_distance;
			wrong_samples = 0;
		}
		
		float min_dist = ((float)MINIMUM_SAFE_DISTANCE_MOVING - MINIMUM_SAFE_DISTANCE_STATIC) * 1 / 100.0f;
		min_dist += MINIMUM_SAFE_DISTANCE_STATIC;

		if (saved_distance < min_dist)	
			too_close_flag = 1;	
		else 
			too_close_flag = 0;
		timer2_count = 0;
		TCNT2 = 0;
		TIMSK2 |= (1 << OCIE2A);
	}	
}


int main()
{
	
	USART0_init(); 
	
	DDRD |= (1 << PD7); // LED

	DDRC |= (1 << PC0); // out motor 1
	DDRC |= (1 << PC1); // out motor 1
	DDRC |= (1 << PC2); // out motor 2
	DDRC |= (1 << PC3); // out motor 2
	
	PORTC &= ~(1 << PC0);
	PORTC &= ~(1 << PC1);
	PORTC &= ~(1 << PC2);
	PORTC &= ~(1 << PC3);
	int turn = 0;
	int canMove = 1;
	
	HC_SR04_init();
	sei();
	
	float dist = 0.0;
	while (1) {
		
		
		float dist = HC_SR04_get_distance();
		if (dist < 20){
			//Daca distanta e mai mica de 30 cm. opresc motorul si nu ma mai pot deplasa in fata
			PORTC &= ~(1 << PC0);
			PORTC &= ~(1 << PC1);
			PORTC &= ~(1 << PC2);
			PORTC &= ~(1 << PC3);
			canMove = 0;
			//Opresc ledul daca nu ma pot deplasa in fata
			PORTD &= ~(1 << PD7);
		} else {
			//Ledul va fi aprins daca ma pot deplasa in fata
			PORTD |= (1 << PD7);
			canMove = 1;
		}
		
		char c = USART0_receive();
		switch(c) {
			case 'a': // Merge in fata
			if (canMove == 1){
				PORTC &= ~(1 << PC1);
				PORTC |= (1 << PC0);
				
				PORTC &= ~(1 << PC3);
				PORTC |= (1 << PC2);
			}
			break;
			case 'c': // Merge in spate
			PORTC &= ~(1 << PC0);
			PORTC &= ~(1 << PC2);
			
			PORTC |= (1 << PC1);
			PORTC |= (1 << PC3);
			break;
			case 'B': // Merge in dreapta
			PORTC &= ~(1 << PC0);
			PORTC |= (1 << PC1);
		
			PORTC &= ~(1 << PC3);
			PORTC |= (1 << PC2);
			turn = 1;
			break;
			case 'D': // Merge in stanga
			PORTC &= ~(1 << PC1);
			PORTC |= (1 << PC0);

			PORTC &= ~(1 << PC2);
			PORTC |= (1 << PC3);
			turn = 1;
			break;
			case 'G':
			PORTC &= ~(1 << PC0);
			PORTC &= ~(1 << PC1);
			PORTC &= ~(1 << PC2);
			PORTC &= ~(1 << PC3);
			break;
			case 'g':
			PORTC &= ~(1 << PC0);
			PORTC &= ~(1 << PC1);
			PORTC &= ~(1 << PC2);
			PORTC &= ~(1 << PC3);
			break;
		}
		
		if (turn == 1){
			_delay_ms(850);
			PORTC &= ~(1 << PC0);
			PORTC &= ~(1 << PC1);
			PORTC &= ~(1 << PC2);
			PORTC &= ~(1 << PC3);
			turn = 0;
		}
	}
	
	return 0;
}