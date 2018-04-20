#include "USART.h"


void USART_init(void) {
    UBRR0H = (uint8_t)(BAUD_PRESCALLER>>8);
    UBRR0L = (uint8_t)BAUD_PRESCALLER;
    UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);
    UCSR0C = (1<<UCSZ00) | (1<<UCSZ01);
}

uint8_t USART_receive(void) {
    while ( !(UCSR0A & (1<<RXC0)) ) {}
    return UDR0;
}

void USART_send(uint8_t data) {
    while ( !(UCSR0A & (1<<UDRE0)) ) {}
    UDR0 = data;
}

void USART_putstring(const char *string_ptr) {
    while (*string_ptr != 0) {
        USART_send(*string_ptr);
        string_ptr++;
    }

    // Force clear buffer
    UCSR0A |= (1<<TXC0);
}
