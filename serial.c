#include <avr/io.h>
#include <util/delay.h>

#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "lcd.h"
#include "serial.h"
extern volatile int datareceive = 0; //1 if data received from other source. notify interrupt to start gathering data. 0 if otherwise.
extern volatile char message[10];
extern volatile int pos = 0; //position of received data
extern volatile int dataend = 0; // 1 if data completely received. 0 otherwise

void serial_txchar(char ch)
{
    while ((UCSR0A & (1 << UDRE0)) == 0);
        
    UDR0 = ch; //sends one character at a time
}

//sends the entire message
void serial_stringout(char* s)
{
    serial_txchar('@');
    int index = 0;
    while (index < strlen(s)) {
        if (s[index] != ' ') {
            serial_txchar(s[index]);
            index++;
        }
    }
    serial_txchar('$');
}


void serial_init(unsigned short ubrr_value)
{
    DDRD |= 0b00000100;
    PORTD &= 0b11111011; // enables and initializes tri-state
    UBRR0 = ubrr_value;
    // Set up USART0 registers
   
}

ISR(USART_RX_vect)
{

    char ch;
    ch = UDR0; //grab the first character sent
    if (datareceive == 1) {
        message[pos] = ch;
        pos++;
    }
    if (ch == '@') {
        datareceive = 1;
    }
    if (datareceive != 1) {
        return;
    }
    if (ch == '$') {
        datareceive = 0;
        pos = 0;
        dataend = 1;
    }
}