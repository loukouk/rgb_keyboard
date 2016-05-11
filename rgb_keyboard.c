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

#define COM	0
#define RED	1
#define GREEN	2
#define BLUE	3

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
#define LED_MATRIX_OUT 	8 // cathodes
#define LED_MATRIX_IN 	9 // anodes. Total outputs = 9*3 = 27


uint16_t rgb[LED_MATRIX_OUT][3];
uint8_t led_on[KEYBOARD_HEIGHT][4*KEYBOARD_WIDTH];
uint8_t led_on_prev[KEYBOARD_HEIGHT][4*KEYBOARD_WIDTH];
uint8_t led_ports[LED_MATRIX_OUT][4];
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

			PORTA = led_ports[0];
			PORTC = led_ports[1];
			PORTD = led_ports[2];
			PORTF = led_ports[4];

			_delay_ms(2);

			PORTA = 0x00;
			PORTC = 0x00;
			PORTD = 0x00;
			PORTF = 0x00;
		}
	}
}

// This interrupt routine is run approx 61 times per second.
// Updates the LED lighting scheme
ISR(TIMER0_OVF_vect)
{
	switch(LIGHTING_MODE) {
		case TOUCH_LMODE:
			break;
		case LEFT_WAVE_LMODE:
			break;
		case RIGHT_WAVE_LMODE:
			break;
		case SNAKE_LMODE:
			break;
		default:
			break;
	}


	PORTA = (i & 0x0F) | (((rgb[i][RED] << 8) & 0x01) >> 1) | (((rgb[i][GREEN] << 8) & 0x01) >> 2) | (((rgb[i][BLUE] << 8) & 0x01) >> 3);
	PORTC = 0x00FF & rgb[i][RED];
	PORTD = 0x00FF & rgb[i][GREEN];
	PORTF = 0x00FF & rgb[i][BLUE];
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
//	key_matrix_out[cycle_count] = 1;

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

		if (!EDITOR_MODE)
			usb_keyboard_send();
	
		keyboard_modifer_keys = 0;
		cycle_count = 0;
		key_count = 0;
		KEY_FN = 0;
		for (i = 0; i < MAX_NUM_KEYS; i++)
			keyboard_keys[i] = 0;
	}
}

uint

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
		case KEY_ENTER:		EDITOR_MODE = 1;
		default:		return key;
	}

	return 0;
}
