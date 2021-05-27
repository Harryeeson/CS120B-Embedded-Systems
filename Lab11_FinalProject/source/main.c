/*	Author: Harrison Yee
 *  Partner(s) Name: 
 *	Lab Section: 21
 *	Assignment: Lab 11  Exercise 1
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 * 	Demo Link:
 */
#include <avr/io.h>
#include "timer.h"
#include "bit.h"
#include "scheduler.h"
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

// pattern: (0 - Off), (1 - On)
// row: (0 - On), (1 - Off)

static task task1, task2;
task *tasks[] = {&task1, &task2};
//starting point for each array
unsigned char colArr[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
unsigned char rowArr[3] = {0x03, 0x07, 0x0F};
unsigned char pattern = 0x01;
unsigned char row = 0x03;
unsigned char j = 0x00;
unsigned char k = 0x00;
unsigned char FirstFlag = 0x00;

enum LED_States {LED_Start, LED_Display, LED_Reverse, LED_Off};
int LED_Tick(int state) {
	switch (state) {
		case LED_Start:
			state = LED_Display;
			break;

		case LED_Display:
			if(row == 0xF8 || row == 0xFC || row == 0xFE) {
				state = LED_Reverse;
			}
			else {
				state = LED_Display;
			}
			break;

		case LED_Reverse:
			if(row == 0xE3 || row == 0xE7 || row == 0xEF) {
				state = LED_Display;
			}
			else {
				state = LED_Reverse;
			}
			break;

		case LED_Off:
			state = LED_Off;
			break;
		
		default:
			state = LED_Start;
			break;
	}
	
	switch (state) {
		case LED_Display:
			if(FirstFlag == 0x00) {			
				row = (row >> 1) | 0xF0;
			}
			else {
				FirstFlag = 0x00;
			}
			break;

		case LED_Reverse:
			row = (row << 1) | 0x01;
			break;
	
		case LED_Off:
			break;

		default:
			break;
	}
	PORTC = pattern;
	PORTD = row;
	return state;
}

enum Button_States {Button_Start, Button_Wait, Button_Next, Button_NextWait, Button_On, Button_Off, Button_Reset};
int Button_Tick(int state) {
	switch(state) {
		case Button_Start:
			state = Button_Wait;
			break;

		case Button_Wait:
			if((~PINA & 0x03) == 0x01) {				// Place block
				state = Button_Next;
			}
			else if((~PINA & 0x03) == 0x02) {			// Turn on/off game
				state = Button_Off;
			}
			else if((~PINA & 0x03) == 0x03) {			// Reset game
				state = Button_Reset;
			}
			else {
				state = Button_Wait;
			}
			break;

		case Button_Next:
			state = Button_NextWait;
			break;

		case Button_NextWait:
			if((~PINA & 0x03) == 0x01) {
				state = Button_NextWait;
			}
			else {
				state = Button_Wait;
			}
			break;

		case Button_Off:
			if((~PINA & 0x03) == 0x02) {			
				state = Button_On;
			}
			else {
				state = Button_Off;
			}
			break;

		case Button_On:
			state = Button_Wait;
			break;
			
		case Button_Reset:
			state = Button_Wait;
			break;

		default:
			state = Button_Start;
			break;
	}
	
	switch(state) {
		case Button_Wait:
			break;

		case Button_Next:
			if(tasks[0]->state == LED_Reverse) {
				tasks[0]->state = LED_Start;
			}
			j++;
                        if(j == 2 || j == 5) {
				k++;
			}
                        pattern = colArr[j];
			row = rowArr[k];
			FirstFlag = 0x01;
			break;

		case Button_NextWait:
			break;
		
		case Button_Off:
			tasks[0]->state = LED_Off;
			pattern = 0x00;
			break;

		case Button_On:
			tasks[0]->state = LED_Start;
			pattern = colArr[j];
			row = rowArr[k];
			FirstFlag = 0x01;
			break;

		case Button_Reset:
			j = 0x00;
			k = 0x00;
			tasks[0]->state = LED_Start;
			pattern = colArr[0];
			row = rowArr[0];
			PORTC = pattern;
			PORTD = row;
			break;

		default:
			break;
	}

	return state;
}

int main(void) {
    /* Insert DDR and PORT initializations */
    DDRA = 0x00; PORTA = 0xFF;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0xFF; PORTD = 0x00;
    /* Insert your solution below */

    const unsigned short numTasks = sizeof(tasks) / sizeof(task*);

    const char start = -1;
    task1.state = start;
    task1.period = 450;
    task1.elapsedTime = task1.period;
    task1.TickFct = &LED_Tick;

    task2.state = start;
    task2.period = 10;
    task2.elapsedTime = task2.period;
    task2.TickFct = &Button_Tick;

    unsigned short i;
    unsigned long GCD = tasks[0]->period;
    for(i = 1; i < numTasks; i++) {
	GCD = findGCD(GCD, tasks[i]->period);
    }
    TimerSet(GCD);
    TimerOn();

    while (1) {
       	for(i = 0; i < numTasks; i++) {
		if( tasks[i]->elapsedTime == tasks[i]->period) {
			tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
			tasks[i]->elapsedTime = 0;
		}
		tasks[i]->elapsedTime += GCD;
	}
	while(!TimerFlag);
	TimerFlag = 0;
    }
    return 1;
}