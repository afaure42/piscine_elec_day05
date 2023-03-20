#include <avr/io.h>
#include <avr/eeprom.h>
#include "uart.h"


int main()
{
	uart_init();
	
	for (size_t i = 0; i < 1024; i++)
		eeprom_write_byte((uint8_t *)i, 0);
	uart_dump_eeprom();
}
