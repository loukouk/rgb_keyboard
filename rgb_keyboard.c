/* Keyboard example for Teensy USB Development Board
 * http://www.pjrc.com/teensy/usb_keyboard.html
 * Copyright (c) 2008 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_keyboard.h"

#define LED_CONFIG	(DDRD |= (1<<6))
#define LED_ON		(PORTD &= ~(1<<6))
#define LED_OFF		(PORTD |= (1<<6))
#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

#define RED	0
#define GREEN	1
#define BLUE	2

#define DEFAULT_LMODE		0
#define TOUCH_LMODE		1
#define LEFT_WAVE_LMODE 	2
#define RIGHT_WAVE_LMODE	3
#define SNAKE_LMODE 		4

//You need to change some source code after editing these values
#define KEYBOARD_WIDTH	17
#define KEYBOARD_HEIGHT	5
#define KEY_MATRIX_IN 	16
#define KEY_MATRIX_OUT 	5
#define LED_MATRIX_OUT 	9 // cathodes
#define LED_MATRIX_IN 	8 // anodes. Total outputs = 8*3 = 24


uint8_t rgb[LED_MATRIX_OUT][3];
uint8_t led_arr[4*KEYBOARD_WIDTH][KEYBOARD_HEIGHT];
uint8_t led_port[LED_MATRIX_OUT][3];
uint8_t EDITOR_MODE = 0;
uint8_t LIGHTING_MODE = DEFAULT_LMODE;

uint8_t key_count = 0;
uint8_t key_map (uint8_t, uint8_t);
uint8_t KEY_FN = 0;

int main(void)
{
	uint8_t i;

	// set for 16 MHz clock
	CPU_PRESCALE(0);

	// Configure PORTA as outputs
	DDRA = 0xFF;
	PORTA = 0x00;
	// Configure PORTB 0:3 and 7 as outputs and 4:6 as inputs
	DDRB = 0x8F;
	PORTB = 0x70;
	// Configure PORTC as outputs
	DDRD = 0xFF;
	PORTD = 0x00;
	// Configure PORTD as outputs
	DDRD = 0xFF;
	PORTD = 0x00;
	// Configure PORTE 6:7 as inputs and 0:5 as outputs
	DDRD = 0xFC;
	PORTD = 0x03;
	// Configure PORTF as outputs
	DDRD = 0xFF;
	PORTD = 0x00;

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.
	usb_init();
	while (!usb_configured()) /* wait */ ;

	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(1000);

	// Configure timer 0 to generate a timer overflow interrupt every
	// 256*1024 clock cycles, or approx 61 Hz when using 16 MHz clock
	// Read a new segment of keyboard matrix at each overflow
	TCCR0A = 0x00;
	TCCR0B = 0x05;
	TIMSK0 = (1<<TOIE0);

	// Configure timer 0 to generate a timer overflow interrupt every
	// 256*1024 clock cycles, or approx 61 Hz when using 16 MHz clock
	// Update the LED lighting scheme at each overflow
	TCCR2A = 0x00;
	TCCR2B = 0x05;
	TIMSK2 = (1<<TOIE0);

	// initialize keyboard_keys array
	key_count = 0;
	for (i = 0; i < MAX_NUM_KEYS; i++)
		keyboard_keys[i] = 0;


	for (i = 0; i < LED_MATRIX_IN; i++) {
		rgb[i][RED]   = 0b00001111;
		rgb[i][GREEN] = 0b00110011;
		rgb[i][BLUE]  = 0b01010101;
	}

	while (1) {
		
		for (i = 0; i < LED_MATRIX_OUT; i++) {

			PORTA = i;
			PORTC = led_port[RED];
			PORTD = led_port[GREEN];
			PORTF = led_port[BLUE];

			_delay_ms(2);

			PORTA = 0x00;
			PORTC = 0x00;
			PORTD = 0x00;
			PORTF = 0x00;
		}
	}
}

void editor_data_send()
{
	uint8_t i, j;
	for (i = 0; i < MAX_NUM_KEYS; i++) {
		switch(keyboard_keys[i]) {
			case KEY_0:		LIGHTING_MODE = DEFAULT_LMODE;
			case KEY_1: 		LIGHTING_MODE = SNAKE_LMODE;
			case KEY_ENTER:		EDITOR_MODE = 0;
			case KEY_R:
				for (j = 0; j < LED_MATRIX_OUT; j++) {
					rgb[j][RED] = 0xFF;
					rgb[j][GREEN] = 0x00;
					rgb[j][BLUE] = 0x00;
				}
			case KEY_G:
				for (j = 0; j < LED_MATRIX_OUT; j++) {
					rgb[j][RED] = 0x00;
					rgb[j][GREEN] = 0xFF;
					rgb[j][BLUE] = 0x00;
				}
			case KEY_B:
				for (j = 0; j < LED_MATRIX_OUT; j++) {
					rgb[j][RED] = 0x00;
					rgb[j][GREEN] = 0x00;
					rgb[j][BLUE] = 0xFF;
				}
		}
	}
}

// This interrupt routine is run approx 61 times per second.
// It reads a single segment of the keyboard matrix
// If all of them have been read, it sends data out through
// USB and resets all variables to start over
ISR(TIMER0_OVF_vect)
{
	static uint8_t key_count = 0, cycle_count = 0, count = 0;
	static uint8_t key_matrix_out[KEY_MATRIX_IN];
	uint8_t i;

	key_matrix_out[cycle_count] = ((PINB & 0x70) >> 4) || ((PINE & 0xC0) >> 3);

	for (i = 0; i < KEY_MATRIX_OUT; i++) {
		if ((key_matrix_out[cycle_count] & (1 << i)) == 0 && key_count < MAX_NUM_KEYS) {
			keyboard_keys[key_count] = key_map(cycle_count, i);
			key_count++;
		}
	}

	PORTB = cycle_count++;
	if (cycle_count >= KEY_MATRIX_IN) {

		if (KEY_FN) {
			for (i = 0; i < MAX_NUM_KEYS; i++)
				keyboard_keys[i] = fn_map(keyboard_keys[i]);
		}

		if (EDITOR_MODE)
			editor_data_send();
		else
			usb_keyboard_send();
	
		keyboard_modifer_keys = 0;
		cycle_count = 0;
		key_count = 0;
		KEY_FN = 0;
		for (i = 0; i < MAX_NUM_KEYS; i++)
			keyboard_keys[i] = 0;
	}
}

// This interrupt routine is run approx 61 times per second.
// Updates the LED lighting scheme
/*
ISR(TIMER0_OVF_vect)
{
	static uint8_t count = 0;
	static uint8_t state[2] = {0,0};

	if (count < 62) {
		count++;
		return;
	}
	count = 0;

	uint8_t i, j, temp;

	switch(LIGHTING_MODE) {
		case TOUCH_LMODE:
			break;
		case LEFT_WAVE_LMODE:
			break;
		case RIGHT_WAVE_LMODE:
			break;
		case SNAKE_LMODE:
			if ((state[0] % 2) == 0) {
				if (state[1] < 4*KEYBOARD_WIDTH)
					state[1]++;
				else
					state[0]++;
			} else {
				if (state[1] > 0)
					state[1]--;
				else if (state[0] < KEYBOARD_HEIGHT)
					state[0]++;
				else
					state[0] = 0;
			}
			led_arr[state[0]][state[1]] == 1;

			break;
		default:
			break;
	}

	// Check which LEDs will be on at this point in time
	for (i=0; i < KEYBOARD_WIDTH*4; i++)
		for (j = 0; j < KEYBOARD_HEIGHT)
			if (led_arr[i][j])
				led_map_red(i,j);

	// copy the data from previous loop to all colors
	for (i = 0; i < LED_MATRIX_OUT; i++) {
		led_port[i][GREEN] = led_port[i][RED];
		led_port[i][BLUE] = led_port[i][RED];
	}

	// apply color changes to the led arrays
	for (i = 0; i < LED_MATRIX_OUT; i++) {
		led_port[i][RED] &= rgb[i][RED];
		led_port[i][GREEN] &= rgb[i][GREEN];
		led_port[i][BLUE] &= rgb[i][BLUE];
	}
}	
*/
uint8_t led_map_red(uint8_t x, uint8_t y)
{
	switch(y) {
		case 0:
			if (x < 4)
				led_port[0][RED] |= 1 << 0;
			else if (x < 8)
				led_port[1][RED] |= 1 << 0;
			else if (x < 12)
				led_port[0][RED] |= 1 << 1;
			else if (x < 16)
				led_port[1][RED] |= 1 << 1;
			else if (x < 20)
				led_port[0][RED] |= 1 << 2;
			else if (x < 24)
				led_port[1][RED] |= 1 << 2;
			else if (x < 28)
				led_port[0][RED] |= 1 << 3;
			else if (x < 32)
				led_port[1][RED] |= 1 << 3;
			else if (x < 36)
				led_port[0][RED] |= 1 << 4;
			else if (x < 40)
				led_port[1][RED] |= 1 << 4;
			else if (x < 44)
				led_port[0][RED] |= 1 << 5;
			else if (x < 48)
				led_port[1][RED] |= 1 << 5;
			else if (x < 52)
				led_port[0][RED] |= 1 << 6;
			else if (x < 60)
				led_port[1][RED] |= 1 << 6;
			else if (x < 64)
				led_port[0][RED] |= 1 << 7;
			else if (x < 68)
				led_port[1][RED] |= 1 << 7;
			return 0;
		case 1:
			if (x < 6)
				led_port[2][RED] |= 1 << 0;
			else if (x < 10)
				led_port[3][RED] |= 1 << 0;
			else if (x < 14)
				led_port[2][RED] |= 1 << 1;
			else if (x < 18)
				led_port[3][RED] |= 1 << 1;
			else if (x < 22)
				led_port[2][RED] |= 1 << 2;
			else if (x < 26)
				led_port[3][RED] |= 1 << 2;
			else if (x < 30)
				led_port[2][RED] |= 1 << 3;
			else if (x < 34)
				led_port[3][RED] |= 1 << 3;
			else if (x < 38)
				led_port[2][RED] |= 1 << 4;
			else if (x < 42)
				led_port[3][RED] |= 1 << 4;
			else if (x < 46)
				led_port[2][RED] |= 1 << 5;
			else if (x < 50)
				led_port[3][RED] |= 1 << 5;
			else if (x < 54)
				led_port[2][RED] |= 1 << 6;
			else if (x < 60)
				led_port[3][RED] |= 1 << 6;
			else if (x < 64)
				led_port[2][RED] |= 1 << 7;
			else if (x < 68)
				led_port[3][RED] |= 1 << 7;
			return 0;
		case 2:
			if (x < 7)
				led_port[4][RED] |= 1 << 0;
			else if (x < 11)
				led_port[5][RED] |= 1 << 0;
			else if (x < 15)
				led_port[4][RED] |= 1 << 1;
			else if (x < 19)
				led_port[5][RED] |= 1 << 1;
			else if (x < 23)
				led_port[4][RED] |= 1 << 2;
			else if (x < 27)
				led_port[5][RED] |= 1 << 2;
			else if (x < 31)
				led_port[4][RED] |= 1 << 3;
			else if (x < 35)
				led_port[5][RED] |= 1 << 3;
			else if (x < 39)
				led_port[4][RED] |= 1 << 4;
			else if (x < 43)
				led_port[5][RED] |= 1 << 4;
			else if (x < 47)
				led_port[4][RED] |= 1 << 5;
			else if (x < 51)
				led_port[5][RED] |= 1 << 5;
			else if (x < 60)
				led_port[4][RED] |= 1 << 6;
			else if (x < 64)
				led_port[4][RED] |= 1 << 7;
			else if (x < 68)
				led_port[5][RED] |= 1 << 7;
			return 0;
		case 3:
			if (x < 9)
				led_port[6][RED] |= 1 << 0;
			else if (x < 13)
				led_port[6][RED] |= 1 << 1;
			else if (x < 17)
				led_port[7][RED] |= 1 << 1;
			else if (x < 21)
				led_port[6][RED] |= 1 << 2;
			else if (x < 25)
				led_port[7][RED] |= 1 << 2;
			else if (x < 29)
				led_port[6][RED] |= 1 << 3;
			else if (x < 33)
				led_port[7][RED] |= 1 << 3;
			else if (x < 37)
				led_port[6][RED] |= 1 << 4;
			else if (x < 41)
				led_port[7][RED] |= 1 << 4;
			else if (x < 45)
				led_port[6][RED] |= 1 << 5;
			else if (x < 49)
				led_port[7][RED] |= 1 << 5;
			else if (x < 60)
				led_port[6][RED] |= 1 << 6;
			else if (x < 64)
				led_port[6][RED] |= 1 << 7;
			else if (x < 68)
				led_port[7][RED] |= 1 << 7;
			return 0;
		case 4:
			if (x < 5)
				led_port[8][RED] |= 1 << 0;
			else if (x < 10)
				led_port[7][RED] |= 1 << 0;
			else if (x < 15)
				led_port[8][RED] |= 1 << 1;
			else if (x < 41)
				led_port[8][RED] |= 1 << 3;
			else if (x < 46)
				led_port[8][RED] |= 1 << 4;
			else if (x < 51)
				led_port[8][RED] |= 1 << 5;
			else if (x < 56)
				led_port[8][RED] |= 1 << 6;
			else if (x < 60)
				led_port[5][RED] |= 1 << 6;
			else if (x < 64)
				led_port[7][RED] |= 1 << 6;
			else if (x < 68)
				led_port[8][RED] |= 1 << 7;
			return 0;
		default:
			return 1;
	}
	return 1;
}

uint8_t key_map (uint8_t km_in, uint8_t km_out)
{
	switch (km_out) {
		case 0:	switch (km_in) {
			case 0:  return KEY_ESC;
			case 1:  return KEY_1;
			case 2:  return KEY_2;
			case 3:  return KEY_3;
			case 4:  return KEY_4;
			case 5:  return KEY_5;
			case 6:  return KEY_6;
			case 7:  return KEY_7;
			case 8:  return KEY_8;
			case 9:  return KEY_9;
			case 10: return KEY_0;
			case 11: return KEY_MINUS;
			case 12: return KEY_EQUAL;
			case 13: return KEY_BACKSPACE;
			case 14: return KEY_DELETE;
			case 15: return KEY_NUM_LOCK;
			default: return 0;
		}
		case 1:	switch (km_in) {
			case 0:  return KEY_TAB;
			case 1:  return KEY_Q;
			case 2:  return KEY_W;
			case 3:  return KEY_E;
			case 4:  return KEY_R;
			case 5:  return KEY_T;
			case 6:  return KEY_Y;
			case 7:  return KEY_U;
			case 8:  return KEY_I;
			case 9:  return KEY_O;
			case 10: return KEY_P;
			case 11: return KEY_LEFT_BRACE;
			case 12: return KEY_RIGHT_BRACE;
			case 13: return KEY_BACKSLASH;
			case 14: return KEY_PAGE_UP;
			case 15: return KEY_HOME;
			default: return 0;
		}
		case 2:	switch (km_in) {
			case 0:  return KEY_CAPS_LOCK;
			case 1:  return KEY_A;
			case 2:  return KEY_S;
			case 3:  return KEY_D;
			case 4:  return KEY_F;
			case 5:  return KEY_G;
			case 6:  return KEY_H;
			case 7:  return KEY_J;
			case 8:  return KEY_K;
			case 9:  return 0;
			case 10: return KEY_L;
			case 11: return KEY_SEMICOLON;
			case 12: return KEY_QUOTE;
			case 13: return KEY_ENTER;
			case 14: return KEY_PAGE_DOWN;
			case 15: return KEY_END;
			default: return 0; 
		}
		case 3:	switch (km_in) {
			case 0:  keyboard_modifier_keys |= KEY_LEFT_SHIFT;
			case 1:  return 0
			case 2:  return KEY_Z;
			case 3:  return KEY_X;
			case 4:  return KEY_C;
			case 5:  return KEY_V;
			case 6:  return KEY_B;
			case 7:  return KEY_N;
			case 8:  return KEY_M;
			case 9:  return KEY_COMMA;
			case 10: return KEY_PERIOD;
			case 11: return KEY_SLASH;
			case 12: return 0;
			case 13: keyboard_modifier_keys |= KEY_RIGHT_SHIFT;
			case 14: return KEY_UP;
			case 15: return KEY_PRINTSCREEN;
			default: return 0;
		}
		case 4:	switch (km_in) {
			case 0:  keyboard_modifier_keys |= KEY_LEFT_CTRL;
			case 1:  keyboard_modifier_keys |= KEY_GUI;
			case 2:  keyboard_modifier_keys |= KEY_LEFT_ALT;
			case 3:  return 0;
			case 4:  return 0;
			case 5:  return 0;
			case 6:  return KEY_SPACE;
			case 7:  return 0;
			case 8:  return 0;
			case 9:  return 0;
			case 10: return KEY_RIGHT_ALT;
			case 11: KEY_FN = 1;
			case 12: keyboard_modifier_keys |= KEY_RIGHT_CTRL;
			case 13: return KEY_LEFT;
			case 14: return KEY_DOWN;
			case 15: return KEY_RIGHT;
			default: return 0;
		}
		default: return 0;
	}
	
	return 0;
}

uint8_t fn_map( uint8_t key )
{
	switch(key) {
		case KEY_ESC: 		return KEY_TILDE;
		case KEY_DELETE: 	return KEY_INSERT;
		case KEY_NUM_LOCK:	return KEY_SCROLL_LOCK;
		case KEY_PRINTSCREEN:	return KEY_PAUSE;
//		case KEY_ENTER:		EDITOR_MODE = 1;
		case KEY_1:		return KEY_F1;
		case KEY_2:		return KEY_F2;
		case KEY_3:		return KEY_F3;
		case KEY_4:		return KEY_F4;
		case KEY_5:		return KEY_F5;
		case KEY_6:		return KEY_F6;
		case KEY_7:		return KEY_F7;
		case KEY_8:		return KEY_F8;
		case KEY_9:		return KEY_F9;
		case KEY_0:		return KEY_F10;
		case KEY_MINUS:		return KEY_F11;
		case KEY_PLUS:		return KEY_F12;
		default:		return key;
	}

	return key;
}
