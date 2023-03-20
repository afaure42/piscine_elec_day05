#include "uart.h"

const char hex_chars[] = "0123456789ABCDEF";

char uart_rx(void)
{
	while (((UCSR0A >> RXC0) & 1) == 0);

	char ret = UDR0;
	return (ret);
}

void print_hex_value(unsigned char c)
{
	uint8_t to_print = c >> 4;
	uart_tx(hex_chars[to_print]);
	to_print = c & 0b1111;
	uart_tx(hex_chars[to_print]);
}

void uart_init()
{
	//set baudrate
	UBRR0H = UART_BAUD_SETTING >> 8;
	UBRR0L = UART_BAUD_SETTING;

	//enable transmitter and receiver
	UCSR0B = 1 << TXEN0 | 1 << RXEN0;

	//set frame format (8data, no parity, 1 stop)
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

}

void uart_tx(char c)
{
	//writing data to send register
	while(((UCSR0A >> UDRE0) & 1) == 0);
	UDR0 = c;
}

void uart_printbyte(uint8_t byte)
{
	for(uint8_t i = 8; i > 0; i--)
		uart_tx(((byte >> (i - 1)) & 1) + '0');
}

void uart_printstr(const char *str)
{
	while(*str)
	{
		uart_tx(*str);
		str++;
	}
}

void uart_putnbr(int32_t number)
{
	if (number < 0)
	{
		number *= -1;
		uart_tx('-');
	}
	if (number >= 10)
		uart_putnbr(number / 10);
	uart_tx(number % 10 + '0');
}

void uart_dump_eeprom(void)
{
	for(uint8_t i = 0; i < 1024 / 16; i++)
	{
		uint16_t temp = (uint16_t)i * 16;
		uart_printstr("[0x");
		print_hex_value((temp & 0xFF00) >> 8);
		print_hex_value (temp & 0x00FF);
		uart_printstr("]\t");
		for(uint8_t j = 0; j < 16; j++)
		{
			const uint16_t ptr = (uint16_t)i * 16 + j;
			uart_printstr(" 0x");
			print_hex_value(eeprom_read_byte((uint8_t *)ptr));
		}
		uart_printstr("\r\n");
	}
}

void uart_print_twi_status()
{
 	//extracting status code from TW status register
	uint8_t status = TWSR;
	status &= TW_STATUS_MASK;
	switch (status)
	{
	case TW_START: {
		uart_printstr("A start condition has been transmitted\r\n");
		break;
	}
	case TW_REP_START: {
		uart_printstr("A repeated start condition has been transmitted\r\n");
		break;
	}
	case TW_MT_SLA_ACK: {
		uart_printstr("SLA + W has been transmitted, ACK has been received\r\n");
		break;
	}
	case TW_MT_SLA_NACK: {
		uart_printstr("SLA + W has been transmitted, NACK has been received\r\n");
		break;
	}
	case TW_MT_DATA_ACK: {
		uart_printstr("Data byte has been transmitted, ACK has been received\r\n");
		break;
	}
	case TW_MT_DATA_NACK: {
		uart_printstr("Data byte has been transmitted, NACK has been received\r\n");
		break;
	}
	case TW_MT_ARB_LOST: {
		uart_printstr("Arbitration lost\r\n");
		break;
	}
	case TW_MR_SLA_ACK: {
		uart_printstr("SLA + R has been transmitted, ACK has been received\r\n");
		break;
	}
	case TW_MR_SLA_NACK: {
		uart_printstr("SLA + R has been transmitted, NACK has been received\r\n");
		break;
	}
	case TW_MR_DATA_ACK: {
		uart_printstr("Data byte has been received, ACK has been returned\r\n");
		break;
	}
	case TW_MR_DATA_NACK: {
		uart_printstr("Data byte has been received, NACK has been returned\r\n");
		break;
	}
	case TW_SR_SLA_ACK: {
		uart_printstr("Slave receiver received SLA + W, ACK returned\r\n");
		break;
	}
	case TW_SR_GCALL_ACK: {
		uart_printstr("Slave receiver received GCALL, ACK returned\r\n");
		break;
	}
	case TW_SR_GCALL_DATA_ACK: {
		uart_printstr("Slave receiver received GCALL DATA, ACK returned\r\n");
		break;
	}
	case TW_SR_GCALL_DATA_NACK: {
		uart_printstr("Slave receiver received GCALL DATA, NACK returned\r\n");
		break;
	}
	case TW_SR_DATA_ACK: {
		uart_printstr("Slave receiver received data, ACK returned\r\n");
		break;
	}
	case TW_SR_DATA_NACK: {
		uart_printstr("Slave receiver received data, NACK returned\r\n");
		break;
	}
	case TW_SR_STOP: {
		uart_printstr("Slave receiver received STOP\r\n");
		break;
	}
	case TW_BUS_ERROR: {
		uart_printstr("TWI BUS ERROR\r\n");
		break;
	}
	case TW_NO_INFO: {
		uart_printstr("TWI NO INFO\r\n");
		break;
	}
	default: {
		uart_printstr("Unkown status, status code is:");
		uart_printbyte(status);
		uart_printstr("\r\n");
		break;
	}
	}
}
