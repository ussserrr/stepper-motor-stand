#ifndef USART_H
#define USART_H


#include <avr/io.h>

#define BAUDRATE 57600
#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1)

void USART_init(void);
uint8_t USART_receive(void);
void USART_send(uint8_t data);
void USART_putstring(const char *string_ptr);


#endif // USART_H
