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

#define RED   0
#define GREEN 1
#define BLUE  2

//You need to change some source code after editing these values
#define KEY_MATRIX_IN 16
#define KEY_MATRIX_OUT 5
#define LED_MATRIX_IN 10
#define LED_MATRIX_OUT 8 // 24 total (3 * 8)

uint8_t key_count = 0;
uint8_t key_map (uint8_t, uint8_t);
uint8_t rgb[LED_MATRIX_IN][3];

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
	// This demonstrates how to use interrupts to implement a simple
	// inactivity timeout.
	TCCR0A = 0x00;
	TCCR0B = 0x05;
	TIMSK0 = (1<<TOIE0);

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
		
		for (i = 0; i < LED_MATRIX_IN; i++) {
			PORTC = rgb[i][RED];
			PORTD = rgb[i][GREEN];
			PORTF = rgb[i][BLUE];
		}
	}
}

// This interrupt routine is run approx 61 times per second.
// A very simple inactivity timeout is implemented, where we
// will send a space character.
ISR(TIMER0_OVF_vect)
{
	static uint8_t key_count = 0, cycle_count = 0;
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
		usb_keyboard_send();
		cycle_count = 0;
		key_count = 0;
		for (i = 0; i < MAX_NUM_KEYS; i++)
			keyboard_keys[i] = 0;
	}
}


uint8_t key_map (uint8_t km_in, uint8_t km_out)
{
/*
	switch (km_in) {
		case 0:	switch (km_out) {
			case 0:  return
			case 1:  return
			case 2:  return
			case 3:  return
			case 4:  return
			case 5:  return
			case 6:  return
			case 7:  return
			case 8:  return
			case 9:  return
			case 10: return
			case 11: return
			case 12: return
			case 13: return
			case 14: return
			case 15: return
			default: return 0;
		}
		case 1:	switch (km_out) {
			case 0:  return
			case 1:  return
			case 2:  return
			case 3:  return
			case 4:  return
			case 5:  return
			case 6:  return
			case 7:  return
			case 8:  return
			case 9:  return
			case 10: return
			case 11: return
			case 12: return
			case 13: return
			case 14: return
			case 15: return
			default: return 0;
		}
		case 2:	switch (km_out) {
			case 0:  return
			case 1:  return
			case 2:  return
			case 3:  return
			case 4:  return
			case 5:  return
			case 6:  return
			case 7:  return
			case 8:  return
			case 9:  return
			case 10: return
			case 11: return
			case 12: return
			case 13: return
			case 14: return
			case 15: return
			default: return 0;
		}
		case 3:	switch (km_out) {
			case 0:  return
			case 1:  return
			case 2:  return
			case 3:  return
			case 4:  return
			case 5:  return
			case 6:  return
			case 7:  return
			case 8:  return
			case 9:  return
			case 10: return
			case 11: return
			case 12: return
			case 13: return
			case 14: return
			case 15: return
			default: return 0;
		}
		case 4:	switch (km_out) {
			case 0:  return
			case 1:  return
			case 2:  return
			case 3:  return
			case 4:  return
			case 5:  return
			case 6:  return
			case 7:  return
			case 8:  return
			case 9:  return
			case 10: return
			case 11: return
			case 12: return
			case 13: return
			case 14: return
			case 15: return
			default: return 0;
		}
		default: return 0;
	}
	*/
	return KEY_A;
}
