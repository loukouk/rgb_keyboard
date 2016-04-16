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

#define NUM_IN 8
#define NUM_OUT 8

uint16_t idle_count = 0;
uint8_t key_count = 0;

uint8_t key_map (uint8_t, uint8_t);

int main(void)
{
	uint8_t b, d, i, j, mask, reset_idle;
	uint8_t b_prev=0xFF, d_prev=0xFF;

	// set for 16 MHz clock
	CPU_PRESCALE(0);

	// Configure PORTB as outputd
	DDRB = 0xFF;
	PORTB = 0x00;
	// Configure PORTD as inputs
	DDRD = 0x00;
	PORTD = 0xFF;

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

	while (1) {

		// initialize keyboard_keys array
		key_count = 0;
		for (i = 0; i < MAX_NUM_KEYS; i++)
			keyboard_keys[i] = 0;

		// read all port D pins
		d = PIND;

		// check if any pins are low, but were high previously
		reset_idle = 0;
		for (i = 0; i < NUM_IN; i++) {
			PORTB = 1 << i;
			mask = 1;
			for (j = 0; j < NUM_OUT; j++) {
				if (((d & mask) == 0) && (d_prev & mask) != 0) {
					keyboard_keys[key_count] = key_map(i,j);
					reset_idle = 1;
					key_count++;
				}
			mask = mask << 1;
			}
		}
		usb_keyboard_send();

		// if any keypresses were detected, reset the idle counter
		if (reset_idle) {
			// variables shared with interrupt routines must be
			// accessed carefully so the interrupt routine doesn't
			// try to use the variable in the middle of our access
			cli();
			idle_count = 0;
			sei();
		}
		// now the current pins will be the previous, and
		// wait a short delay so we're not highly sensitive
		// to mechanical "bounce".
		d_prev = d;
		_delay_ms(2);
	}
}

// This interrupt routine is run approx 61 times per second.
// A very simple inactivity timeout is implemented, where we
// will send a space character.
ISR(TIMER0_OVF_vect)
{
	idle_count++;
	if (idle_count > 61 * 8) {
		idle_count = 0;
		usb_keyboard_press(KEY_SPACE, 0);
	}
}



uint8_t key_map (uint8_t b, uint8_t d)
{
	switch (d) {
		case 0:	switch (b) {
					case 0: return
					case 1: return
					case 2: return
					case 3: return
					case 4: return
					case 5: return
					case 6: return
					case 7: return
					default: return 0;
				}
		case 1: switch (b) {
					case 0: return
					case 1: return
					case 2: return
					case 3: return
					case 4: return
					case 5: return
					case 6: return
					case 7: return
					default: return 0;
				}
		case 2: switch (b) {
					case 0: return
					case 1: return
					case 2: return
					case 3: return
					case 4: return
					case 5: return
					case 6: return
					case 7: return
					default: return 0;
				}
		case 3: switch (b) {
					case 0: return
					case 1: return
					case 2: return
					case 3: return
					case 4: return
					case 5: return
					case 6: return
					case 7: return
					default: return 0;
				}
		case 4: switch (b) {
					case 0: return
					case 1: return
					case 2: return
					case 3: return
					case 4: return
					case 5: return
					case 6: return
					case 7: return
					default: return 0;
				}
		case 5: switch (b) {
					case 0: return
					case 1: return
					case 2: return
					case 3: return
					case 4: return
					case 5: return
					case 6: return
					case 7: return
					default: return 0;
				}
		case 6: switch (b) {
					case 0: return
					case 1: return
					case 2: return
					case 3: return
					case 4: return
					case 5: return
					case 6: return
					case 7: return
					default: return 0;
				}
		case 7: switch (b) {
					case 0: return
					case 1: return
					case 2: return
					case 3: return
					case 4: return
					case 5: return
					case 6: return
					case 7: return
					default: return 0;
				}
	}
}
