#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h> 
#include <avr/interrupt.h>
#include "lcd.h"
#include <avr/eeprom.h>

#define FOSC 16000000 // Clock frequency
#define BAUD 9600 // Baud rate used
#define MYUBRR (FOSC/16/BAUD-1) // Value for UBRR0

void variable_delay_us(int);
void play_note(unsigned short);

// Frequencies for natural notes from middle C (C4)
// up one octave to C5.
unsigned short frequency[8] =
    { 262, 294, 330, 349, 392, 440, 494, 523 };
    
volatile unsigned char new_state, old_state;
volatile unsigned char tempp[5];
volatile unsigned char buff[5];
volatile unsigned char datareceived = 0;
volatile unsigned char datadone = 0;
volatile unsigned char datasend = 0;
volatile unsigned char datanum = 0;
volatile unsigned char buttonstate=0;
volatile unsigned char mode = 0;
volatile unsigned char buttonflag = 0;
volatile unsigned int bufcount = 0;
volatile unsigned char a, b, c, d;
volatile int count = 1;		// Count to display
volatile int flag = 0;		//1 if LCD needs to be updated. 0 otherwise.
volatile unsigned int pulse_count = 0;
volatile int distance = 0;
volatile int localdist = 0;





void serial_txchar(char ch)
{

    while ((UCSR0A & (1<<UDRE0)) == 0);
    UDR0 = ch;//sending one character each time
}

void serial_stringout(char *s)
{
	serial_txchar('@');
	int index = 0;
	while(index < 4){
		// if(!(s[index]>='0')||!(s[index]<='9')){
 			index++;
	}else{
 			serial_txchar(s[index]);
 			index++;
 		}
		serial_txchar(s[index]);
		index++;
	}
	serial_txchar('$');

}
void serial_init(unsigned short ubrr_value)
{
	UCSR0B |= (1 << TXEN0 | 1 << RXEN0); // Enable RX and TX
	UCSR0C = (3 << UCSZ00); // Async., no parity,
							// 1 stop bit, 8 data bits
	UBRR0 = ubrr_value; 
	
    // Set up USART0 registers
   
}

int main(void) {

	//set up LED
	DDRC |= 0b00010000;
	DDRB |= 0b00100000;
	
	//set up and enable tristate
	DDRB |= 0b00001000;
	PORTB &= 0b11110111;
	
	count = eeprom_read_word((void *) 0);
	if(count<1 || count > 400){
		count = 1;
	}
	
	serial_init(MYUBRR);
	UCSR0B |= (1 << RXCIE0); // Enable receiver interrupts
   
    
	// Initialize DDR and PORT registers and LCD
	DDRC &= 0b11010001;
	DDRD |= 0b00000100;
	DDRD &= 0b11110111;
	PORTC |= 0b00101110;
	DDRB |= 0b00010000;
	lcd_init();
	
	//Initialize the Timer
	
	TCCR1B |= (1 << WGM12); // Set to CTC mode
	TCCR1B &= ~(1 << WGM13); 
	TIMSK1 |= (1 << OCIE1A); // Enable Timer Interrupt
	
	// Load the MAX count
	// Assuming prescalar=8
	// counting to 2 =
	// 1 us w/ 16 MHz clock
	OCR1A = 2; 
	//TCCR1B &= 0b11111000; // timer set to zero but left in stopped state
	TCCR1B  = 0;

    
    
    //initialize and set up interrupts
	cli();
	PCICR |=  ((1 << PCINT0)| (1 << PCINT1)|(1 << PCINT2));
	PCMSK1 = 0b00101110;
	PCMSK0 = 0b00011000;
	PCMSK2 = 0b00001000;
	sei();
	
	lcd_moveto(0,0);
	lcd_stringout("Single");
	lcd_moveto(1,0);
    snprintf(display_count, 30, "%s%d", "Min:", count);
    lcd_stringout(display_count);
    
    // Read the A and B inputs to determine the intial state
    // Warning: Do NOT read A and B separately.  You should read BOTH inputs
    // at the same time, then determine the A and B values from that value.    

	b = PINC & 0b00100000;
	a = PINC & 0b00000010;
	
    if (!b && !a)
	old_state = 0;
    else if (!b && a)
	old_state = 1;
    else if (b && !a)
	old_state = 2;
    else
	old_state = 3;

    new_state = old_state;

    while (1) {       
    	if(mode){
    		PORTD |= 0b00000100;
			_delay_us(10);
			PORTD &= 0b11111011;
    	}
    	if(buttonflag == 2){
    	//acquire button pressed
			
			if(buttonstate == 0){
				PORTD |= 0b00000100;
				_delay_us(10);
				PORTD &= 0b11111011;
				
			}
			buttonflag = 0;
    	}
    	if(datadone){
    		
    		lcd_moveto(0,7);
    		lcd_stringout("                 ");
    		lcd_moveto(0,7);
    		lcd_stringout(buff);
    		
			sscanf(buff , "%d" , &distance);
			sscanf(tempp , "%d" , &localdist);
			
			if(distance < count){
				lcd_moveto(0,7);
				
				//lcd_stringout("Below min value!");
				//play_note(262);
			}
			if(distance>400){
				lcd_moveto(0,7);
				lcd_stringout("Too far!");
			}
			
			if(distance < localdist){
				PORTC |= 0b00010000;
				PORTB &= 0b11011111;
			}else if(distance > localdist){
				PORTB |= 0b00100000;
				PORTC &= 0b11101111;
			}else{
				PORTB &= 0b11011111;
				PORTC &= 0b11101111;
			}
			datadone = 0;
    		
    	}
		if (flag) { // Did state change?
		// Output count to LCD
			int temp = count; // temporary value to capture correct value of 'count'
			if(temp<1){
				temp=1;
				count=1;
			}else if(temp>400){
				temp=400;
				count=400;
			}else{
				flag = 0;
				
				snprintf(display_count, 30, "%s%d", "Min:", temp);
				
				lcd_moveto(1,0);
				lcd_stringout(display_count);
				if(temp<100){
					lcd_stringout(" ");
				}
				
			}
		}
    }
}


ISR(PCINT2_vect){
	if(PIND & 0b00001000){
		TCNT1 = 0;
		TCCR1B |= (1 << CS11); // Set prescalar = 8 and start counter
		TCCR1B &= 0b11111010;
	}else if(!(PIND & 0b00001000)){
		TCCR1B =0;
		pulse_count = TCNT1;
		lcd_moveto(0,7);
    	snprintf(tempp, 5, "%d", pulse_count/58); 
    	
    	
		if((pulse_count/58)>=1 && (pulse_count/58)<=400){
			lcd_moveto(1,8);
			lcd_stringout(tempp);
			lcd_stringout("            ");
		}else{
			lcd_moveto(1,8);
			lcd_stringout("Too Far!");
		}
    	
    	serial_stringout(tempp);
    	
    	
	}
	

}

ISR(TIMER1_COMPA_vect){
	
}


ISR(USART_RX_vect)
{

	char ch;
	ch = UDR0; //receive the sent character
	if(ch == '$' && bufcount > 0){
		datadone = 1;
		datareceived = 0;
		datanum = bufcount;
		bufcount = 0;
		lcd_moveto(0,7);
		for(int i = 0; i<datanum;i++){
			lcd_stringout(buff[i]);
		}
	}
	if(datareceived){
		if(bufcount > 4){
			datareceived = 0;
			bufcount = 0;
			datanum = 0;
		}
		int temp = 0;
		sscanf (ch , "%d" , temp);
		
		if(temp >= 0 && temp <= 9){
			buff[bufcount] = ch;
			bufcount++;
		}else{
			datareceived = 0;
			bufcount = 0;
			datanum = 0;
		}
		
		
	}
	if(ch == '@'){
		datareceived = 1;
		datadone = 0;
		bufcount = 0;
		
	}
	
	
	
	
	
}

ISR(PCINT0_vect){

}
ISR(PCINT1_vect){
// Read the input bits and determine A and B
	a = PINC & 0b00000010;
	b = PINC & 0b00100000;
	c = PINC & 0b00000100;
	d = PINC & 0b00001000;
	if(c == 0){
		_delay_ms(5);
		while( (PINC & 0b0000100) == 0 ) { }
		_delay_ms(5);
		
		if(buttonstate == 0){
			buttonstate = 1;
		}else if(buttonstate == 1){
			buttonstate = 0;
		}
		
		if(buttonstate == 0){
			lcd_moveto(0,0);
			lcd_stringout("Single");
			mode = 0;
		}else{
			lcd_moveto(0,0);
			lcd_stringout("Repeat");
			mode = 1;
		}      
	}else if(d == 0){
		_delay_ms(5);
		while( (PINC & 0b00001000) == 0 ) { }
		_delay_ms(5);
		buttonflag = 2;
	}else{
		flag = 1;
		
		if (old_state == 0) {
		// Handle A and B inputs for state 0
			if(a){
				new_state = 1;
				count++;
			}else if(b){
				new_state = 2;
				count--;
			}

		}	
		else if (old_state == 1) {
		// Handle A and B inputs for state 1
			if(b){
				new_state = 3;
				count++;
			}else if(!a){
				new_state = 0;
				count--;
			}	
		}
		else if (old_state == 2) {
		// Handle A and B inputs for state 2
			if(!b){
				new_state = 0;
				count++;
			}else if(a){
				new_state = 3;
				count--;
			}
		}
		else {   // old_state = 3
		// Handle A and B inputs for state 3
			if(!a){
				new_state = 2;
				count++;
			}else if(!b){
				new_state = 1;
				count--;
			}
		}
		//eeprom_update_word((void *) 0, count);
		old_state = new_state;
	}
	
}

/*
  Play a tone at the frequency specified for one second
*/
void play_note(unsigned short freq)
{
    unsigned long period;

    period = 1000000 / freq;      // Period of note in microseconds

    while (freq--) {
	// Make PB4 high
	PORTB |= (1 << PB4);
	// Use variable_delay_us to delay for half the period
	variable_delay_us(period/2);                                                                                                                  
	// Make PB4 low
	PORTB &= ~(1 << PB4);
	// Delay for half the period again
	variable_delay_us(period/2);
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

