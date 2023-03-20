#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/twi.h>
#include "uart.h"

#define MAGIC_NUMBER 0x5A
#define MAGIC_NUMBER_ADDR (size_t)0x0003
#define COUNTER1_ADDR (size_t)0x00FF
#define MALLOC_HEADER_SIZE 5

#define SCL_FREQUENCY 100000
#define TWI_BITRATE (((F_CPU / SCL_FREQUENCY) - 16) / 2)

#define ACK (1 << TWEA)
#define NACK 0
#define EXPANDER_ADDR 0b00100000
#define TW_WRITE 0
#define TW_READ 1



const uint8_t magic = MAGIC_NUMBER;
uint8_t last_read_value = 0;
uint8_t current_counter = 0;
uint8_t last_button_value = 1;

void display(void);
void ft_eeprom_init(void);
uint8_t safe_eeprom_read(void *buffer, size_t offset, size_t length);
uint8_t safe_eeprom_write(void *buffer, size_t offset, size_t length);

void display()
{
	//clearing output port flags
	PORTB = ~(1 << PB0 | 1 << PB1 | 1 << PB2 | 1 << PB4);

	PORTB |= (TCNT0 & 0b0111) | ((TCNT0 & 0b1000) << 1);
}

// void ft_eeprom_init(void)
// {
// 	uint8_t buffer = 0;
// 	for (size_t i = 0; i < 4; i++)
// 	{
// 		if (safe_eeprom_read(&buffer, COUNTER1_ADDR + i * 2, 1) == 0)
// 		{
// 			uart_printstr("MAGIC MISSING, INIT TO 0\r\n");
// 			uart_putnbr(i);
// 			uart_printstr("\r\n");
// 			safe_eeprom_write(&buffer, COUNTER1_ADDR + i * 2, 1);
// 		}
// 	}
// 	last_read_value = 0;
// }
//	ptr 	1		2		3	 4	      5
// [MAGIC] [ID1] [ID2] [SIZE1] [SIZE2] [BLOCK]

uint8_t find_next_magic(uint16_t start, uint16_t *ptr)
{
	uint8_t buff;
	while (*ptr < 1024)
	{
		if (eeprom_read_byte((uint8_t *)(*ptr) == MAGIC_NUMBER))
			return 1;
	}
	return 0;
}

uint8_t eeprommaloc_find_id(uint16_t id, uint16_t * ptr, uint16_t * last_known)
{
	uint16_t buffer;
	*last_known = 0;
	*ptr = 0;
	while (*ptr < 1024)
	{
		if (eeprom_read_byte((uint8_t*)(*ptr)) == MAGIC_NUMBER)
		{
			uart_printstr("FOUND MAGIC NUMBER!\r\n");
			//read id
			buffer = eeprom_read_word((uint16_t *)((*ptr) + 1));
			if (buffer == id)
				return 1; // success
			//read size to increment pointer
			buffer = eeprom_read_word((uint16_t *)((*ptr) + 3));
			*last_known = *ptr;
			*ptr += buffer + MALLOC_HEADER_SIZE;
		}
		else
		{
			(*ptr)++;
		}
	}
	return 0; //fail we dont have enough memory
}

uint8_t eepromalloc_write(uint16_t id, void *buffer, uint16_t length)
{
	uint16_t ptr;
	uint16_t last_segment;
	//find id
	if (eeprommaloc_find_id(id, &ptr, &last_segment) == 0)
	{
		//if id isnt known, find a slot
		//where i can fit the new data
		while (last_segment + length)
		{
		}
		
		if (last_segment + length + MALLOC_HEADER_SIZE < 1024)
		{
			if (last_segment == 0 && ptr == 0)
				ptr = 0;
			else
				ptr = last_segment + length + MALLOC_HEADER_SIZE;
			eeprom_write_byte((uint8_t *)ptr, MAGIC_NUMBER);
			eeprom_write_word((uint16_t *)(ptr + 1), id);
			eeprom_write_word((uint16_t *)(ptr + 3), length);
			eeprom_write_block(buffer, (uint8_t*)(ptr + MALLOC_HEADER_SIZE), length);
			return 1;
		}
		else //not enough memory
			return 0;
	}
	else //if id is found
	{
		uint16_t current_length;

		eeprom_read_block(&current_length, (uint8_t *)(ptr + 3), 2);
		//if there is enough memory
		if (current_length >= length)
		{
			eeprom_write_block(buffer, (uint8_t *)(ptr + MALLOC_HEADER_SIZE), length);
			return 1;
		}
		else	//if there isnt enough memory
			return 0;
	}

	//if not enough memory return 0
	return (0);
}

uint8_t eepromalloc_read(uint16_t id, void *buffer, uint16_t length)
{
	uint16_t ptr;
	uint16_t last_segment;
	//find id
	if (eeprommaloc_find_id(id, &ptr, &last_segment) == 0)
		return 0;

	eeprom_read_block(buffer, (uint8_t *)(ptr + MALLOC_HEADER_SIZE), length);
	return 1;
}

uint8_t eepromalloc_free(uint16_t id)
{
	uint16_t ptr;
	uint16_t last_segment;

	if (eeprommaloc_find_id(id, &ptr, &last_segment) == 0)
		return 0;

	eeprom_write_byte((uint8_t *)ptr, 0x00);
	return 1;
}

void ft_timer0_init(void)
{
	TCCR0A = 0; //normal mode of operation with 0xFF as top
	//external clock source on falling edge
	TCCR0B = (1 << CS02) | (1 << CS01);
	TCNT0 = 0xFF;
}

//*********************TWI
/*void i2c_init(void)
{
	//setting SCL frequency in the bitrate register
	TWBR = TWI_BITRATE;

	//setting no prescaler 
	TWSR = 0;
}

void i2c_stop()
{
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void i2c_wait(void)
{
	while((TWCR & (1 << TWINT)) == 0);
}

void i2c_start(uint8_t address, uint8_t write)
{
	TWCR = 1 << TWINT | 1 << TWSTA | 1 << TWEN; //sending start

	i2c_wait();

	TWDR = (address << 1) | (write & 1);

	TWCR = (1 << TWINT) | (1 << TWEN);
	i2c_wait();
}

uint8_t i2c_send_byte(uint8_t * buffer, uint8_t size)
{
	for(uint8_t i = 0; i < size && TWSR != TW_MR_DATA_NACK; i++)
	{
		TWDR = buffer[i];

		TWCR = (1 << TWINT) | (1 << TWEN);
		i2c_wait();
	}
	return 0;
}

uint8_t i2c_read_byte(uint8_t * buffer, uint8_t size)
{
	for(uint8_t i = 0; i < size; i++)
	{
		TWCR = (1 << TWINT) | (1 << TWEN) | (i == size - 1 ? NACK : ACK);
		i2c_wait();

		buffer[i] = TWDR;
	}
	return 0;
}

uint8_t poll_switch3()
{
	uint8_t command = 0;
	uint8_t buffer;
	i2c_start(EXPANDER_ADDR, TW_WRITE);
	i2c_send_byte(&command, 1);
	i2c_start(EXPANDER_ADDR, TW_READ);
	i2c_read_byte(&buffer, 1);
	i2c_stop();

	if (last_button_value == 1 && (buffer & 1) == 0)//if button just got pressed
	{
		uint8_t counter_buffer;
		current_counter++;
		if (current_counter == 4)
			current_counter = 0;
		
		last_button_value = 0;
		uart_printstr("BUTTON [");
		uart_putnbr(current_counter);
		uart_printstr("] PRESSED\r\n");
		if (safe_eeprom_read(&counter_buffer, COUNTER1_ADDR + current_counter * 2, 1) == 0)
		{
			uart_printstr("READ ERROR\r\n");
			return 0;
		}
		last_read_value = counter_buffer;
		TCNT0 = last_read_value;
		display();
	}
	if (last_button_value == 0 && (buffer & 1) == 1)
	{
		last_button_value = 1;
	}
	return 1;
}*/

int main()
{
	uart_init();
	uint16_t id = 0;
	for (;;)
	{
	char buffer[20];
	uart_printstr("ID:");
	uart_putnbr(id);
	uart_printstr("\r\n");
	if (eepromalloc_read(id, buffer, 20) == 0)
	{
		uart_printstr("no id allocating new\r\n");
		if (eepromalloc_write(id, "Coucou, c'est moi !", 20) == 0)
			uart_printstr("Allocation failed\r\n");
		else
		{
			uart_printstr("Allocation success\r\n");
			eepromalloc_read(id, buffer, 20);
			uart_printstr(buffer);
		}
	}
	else
	{
		uart_printstr("Read success\r\n");
		uart_printstr(buffer);
	}

	uart_printstr("Free: Y or N\r\n");
	char c = uart_rx();
	if (c == 'y' || c == 'Y')
		eepromalloc_free(id);
	uart_printstr("Id i or d\r\n");
	c = uart_rx();
	if (c == 'i')
		id++;
	if (c == 'd')
		id--;
	uart_dump_eeprom();
	}
}
