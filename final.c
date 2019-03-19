#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "lcd.h"
#include "serial.h"
#include "encoder.h"

#define FOSC 16000000 // Clock frequency
#define BAUD 9600 // Baud rate used
#define MYUBRR FOSC / 16 / BAUD - 1

volatile unsigned char new_state, old_state;
volatile unsigned char a, b; // stores the inputs of the rotary encoder
volatile int count = 0; // count to be changed by the rotary encoder. also the minimum distance
volatile int timercount = 1;
volatile int count = 0;
volatile int mode; //current mode
volatile int acquire = 0; //1 if acquire button pressed. 0 otherwise
volatile int memmin; //the minimum distance stored in the Arduino’s EEPROM non-volatile memory
volatile char message[10];
volatile int datareceive = 0; //1 if data received from other source. notify interrupt to start gathering data. 0 if otherwise.
volatile int dataend = 0; // 1 if data completely received. 0 otherwise
volatile float dist; //variable for converted distance
volatile int pos = 0; //position of received data
char distmm[10]; //distance in mm
char distmm2[10]; //distance in mm
char temp[10]; //temporary array to make the sending process more smooth
int message_text; //used to store the message in single binary fixed-point number form

void variable_delay_us(int);
void play_note(unsigned short);

//sends the signal to the ultrasonic range sensor telling it to start sensing
void start_sensor()
{
    PORTC |= 0b00000010;
    _delay_ms(0.01);
    PORTC &= 0b11111101;
}

float distance_conversion(int count)
{
	//converts the count determined from the wave that returned from the sensor using a factor
    float factor = 0.068;
    factor = count * factor;
    return (factor);
}

void act(){
	char output[30];
	start_sensor();
	
	dist = convert_distance(count);
	
	if (dist >= 400) {
		lcd_moveto(0, 8);
		lcd_stringout("               ");
		lcd_moveto(0, 8);
		lcd_stringout("Too far!");
		snprintf(temp, 5, "%d", (int)(4001));
		serial_stringout(temp);
	}
	else {
		lcd_moveto(0, 8);
		lcd_stringout("       ");
		snprintf(output, 30, "%3d.%d", (int)dist, (int)(dist * 10) % 10);
		lcd_moveto(0, 8);
		lcd_stringout(output);
		lcd_stringout("       ");
	}
	
}

int main()
{
    //initializing the Serial Interface
    serial_init(MYUBRR);
    UCSR0C = (3 << UCSZ00);
    UCSR0B |=(1 << TXEN0 | 1 << RXEN0);
    UCSR0B |= (1 << RXCIE0);
    UCSR0C = (3 << UCSZ00);
   
    lcd_init(); //initialize the lcd
    lcd_writecommand(1); 
	
    DDRC |= 0b00110010;
	PORTC &= 0b11001111;
    DDRB |= 0b00100000;  //declaring that A1 is triggering output
   
    PORTC |= 0b00001100; //declares A2 as the "acquire" button and A3 as the "mode" button
    PORTB |= 0b00011000; //initializes the pull-up resistor for the rotary encoder
   	PORTD |= 0b00001000; //declaring D2 as the input from the rangefinder
   
   	//initialize and set up interrupts
    PCICR |= 0b00000100;
    PCMSK2 |= 0b00001000;
    PCICR |= 0b00000001; 
    PCMSK0 |= 0b00011000;

    //initialize the pin change interrupt
    mode = 0;
    count = eeprom_read_word((void*)100);
    sei();
    
    displayname(); // Writes a splash screen to the LCD
    
    while (1) {
    	lcd_moveto(1, 0);
        char mem[30];
        
        memmin = eeprom_read_word((void*)100);
        if(memmin<1 ||  memmin > 400){
			memmin = 1;
		}
        
        snprintf(mem, 30, "%s%2d", "Min:", memmin);
        lcd_stringout(mem);
        eeprom_update_word((void*)100, count);

        if (mode == 0) { //Single mode
            acquire = 0;
            lcd_moveto(0, 0);
            lcd_stringout("Single"); 
            if ((PINC & 0b00001000) == 0) {
                while ((PINC & 0b00001000) == 0) {
                }
                mode = 1;
            }
            
            if ((PINC & 0b00000100) == 0) {
                while ((PINC & 0b00000100) == 0){
                }
                
                act();
                
               	act();
                    
				snprintf(distmm, 5, "%d", (int)(dist * 10));
				serial_stringout(distmm);
				
				
				sscanf(message, "%d", &message_text);
				
				if ((message_text / 10 > (int)dist){
					PORTC &= 0b11101111;
					PORTC |= 0b00100000;
				}
				else if (message_text / 10 == (int)dist){
					PORTC &= 0b11001111;
				}
				else{
					PORTC &= 0b11011111;
					PORTC |= 0b00010000;
					
					play_note(262);
				}
            }
        	

            
        }

        if (mode == 1){//repeat mode

            lcd_moveto(0, 0);
            lcd_stringout("Repeat");
           
            if ((PINC & 0b00001000) == 0) {
                while ((PINC & 0b00001000) == 0) {
                }
                mode = 0;
            }
            
          
			start_sensor();
			
			char output1[30];
			
			dist = convert_distance(count);
			if (dist > 400) {
				lcd_moveto(0, 8);
				lcd_stringout("Too far!");
			}
			else {
				lcd_moveto(0, 8);
				lcd_stringout("     ");
				snprintf(output1, 30, "%3d.%d", (int)dist, (int)(dist * 10) % 10);
				lcd_moveto(0, 8);
				lcd_stringout(output1);
				lcd_stringout("       ");
				
				start_sensor();
			   
				char output2[30];
				dist = convert_distance(count);
				snprintf(output2, 30, "%3d.%d", (int)dist, (int)(dist * 10) % 10);
				lcd_moveto(0, 8);
				lcd_stringout(output2);
				lcd_stringout("       ");
				
				snprintf(distmm2, 5, "%d", (int)(dist * 10));
				serial_stringout(distmm2);
			}
            
        }
        if (dataend == 1){
            lcd_moveto(1, 8);
            lcd_stringout("       ");
            lcd_moveto(1, 8);
        
            sscanf(message, "%d", &message_text);
            
            if (message_text > 4000){
                lcd_moveto(1, 8);
                lcd_stringout("Too far!");
                dataend = 0;
            }
            else{
                char tempmessage[6];
                snprintf(tempmessage, 6, "%3d.%d", message_text / 10, message_text % 10);
                lcd_stringout(tempmessage);

                if (message_text / 10 > (int)dist) {
                    PORTC |= 0b00100000;
                    PORTC &= 0b11101111;
                }
                else if (message_text / 10 == (int)dist) {
                    PORTC &= 0b11001111;
                }
                else {
                    PORTC |= 0b00010000;
                    PORTC &= 0b11011111;
                }
                if ((message_text / 10) < count) {
                    play_note(262);
                }
                dataend = 0;
            }
        }
    }
}

//timer interrupt
ISR(PCINT2_vect)
{
	timercount++;
    if ((timercount % 2) == 1) {
		TCNT1 = 0;
		TIMSK1 |= (1 << OCIE1A);
        TCCR1B |= (1 << WGM12);
        TCCR1B |= 0b00000011;
    }
    else {
		TCCR1B &= 0b11111000;
        count = TCNT1;
    }
}

void play_note(unsigned short freq)
{
    unsigned long period;

    period = 1000000 / freq; // Period of note in microseconds

    while (freq--) {
        PORTB |= 0b00100000;
        variable_delay_us(period / 2); // Make PB4 high
        PORTB &= 0b11011111;
        variable_delay_us(period / 2);
        // Use variable_delay_us to delay for half the period

        // Make PB4 low

        // Delay for half the period again
    }
}

/*
    variable_delay_us - Delay a variable number of microseconds
*/
void variable_delay_us(int delay)
{
    int i = (delay + 5) / 10;

    while (i--)
        _delay_us(10);
}