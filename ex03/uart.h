#ifndef UART_H
#define UART_H

#define UART_BAUD_SETTING (((F_CPU / 8 / UART_BAUDRATE ) -1 ) / 2)

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/twi.h>

char uart_rx(void);
void uart_print_twi_status();
void uart_printstr(const char *str);
void uart_printbyte(uint8_t byte);
void uart_tx(char c);
void uart_init();
void print_hex_value(unsigned char c);
void uart_putnbr(int32_t number);
void uart_dump_eeprom(void);

#endif
