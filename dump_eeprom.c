#include "uart.h"

int main()
{
	uart_init();

	uart_dump_eeprom();
}
