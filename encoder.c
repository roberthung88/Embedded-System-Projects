#include <avr/io.h>
#include <util/delay.h>

#include "encoder.h"
extern volatile unsigned char a, b; // stores the inputs of the rotary encoder
extern volatile int count = 0; 
extern volatile unsigned char new_state, old_state;



ISR(PCINT0_vect)
{
	//grabs the values of the rotary encoder inputs
    a = ((PINB & 0b00010000) >> 4);
    b = ((PINB & 0b00001000) >> 3); 


    if (count <= 0) {
        count = 0;
    }
    if (count > 400) {
        count = 0;
    }
    if (old_state == 0) {
        if (a) {
            count++;
            new_state = 1;
        }
        if (b) {
            count--;
            new_state = 2;
        }
        // Handles A and B inputs for state 0
    }
    else if (old_state == 1) {
        if (!a) {
            count--;
            new_state = 0;
        }
        if (b) {
            count++;
            new_state = 3;
        }

        // Handles A and B inputs for state 1
    }
    else if (old_state == 2) {
        if (a) {
            count--;
            new_state = 3;
        }
        if (!b) {
            count++;
            new_state = 0;
        }

        // Handles A and B inputs for state 2
    }
    else { // old_state = 3
        if (!a) {
            count++;
            new_state = 2;
        }
        if (!b) {
            count--;
            new_state = 1;
        }

        // Handles A and B inputs for state 3
    }
    old_state = new_state; // change the new state to the old state inside the interrupt function
}