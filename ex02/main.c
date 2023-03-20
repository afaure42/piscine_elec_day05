#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/twi.h>
#include "uart.h"

#define MAGIC_NUMBER 0x5A
#define MAGIC_NUMBER_ADDR (size_t)0x0003
#define COUNTER1_ADDR (size_t)0x00FF


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

void ft_eeprom_init(void)
{
	uint8_t buffer = 0;
	for (size_t i = 0; i < 4; i++)
	{
		if (safe_eeprom_read(&buffer, COUNTER1_ADDR + i * 2, 1) == 0)
		{
			uart_printstr("MAGIC MISSING, INIT TO 0\r\n");
			uart_putnbr(i);
			uart_printstr("\r\n");
			safe_eeprom_write(&buffer, COUNTER1_ADDR + i * 2, 1);
		}
	}
	last_read_value = 0;
}

uint8_t safe_eeprom_read(void *buffer, size_t offset, size_t length)
{
	if (eeprom_read_byte((uint8_t *)offset) != MAGIC_NUMBER)
		return 0;
	eeprom_read_block(buffer, (uint8_t *)(offset + 1), length);
	return 1;
}

uint8_t safe_eeprom_write(void *buffer, size_t offset, size_t length)
{
	if (eeprom_read_byte((uint8_t *)offset) == MAGIC_NUMBER)
	{
		//compare write
		//compare each byte and if it isnt the same write it again else dont write
		//to preserve eeprom
		for(size_t i = 0; i < length; i++)
		{
			if (eeprom_read_byte((uint8_t *)(offset + i + 1)) != ((uint8_t *)buffer)[i])
				eeprom_write_byte((uint8_t *)(offset + i + 1), ((uint8_t *)buffer)[i]);
		}
	}
	else
	{
		uart_printstr("NO MAGIC DETECTED WRITING IT DIRECTLY\r\n");
		//plain write
		eeprom_write_byte((uint8_t *)offset, MAGIC_NUMBER);
		eeprom_write_block(buffer, (uint8_t *)(offset + 1), length);
	}
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
void i2c_init(void)
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
}

int main()
{
	ft_timer0_init();
	uart_init();
	ft_eeprom_init();
	i2c_init();

	DDRD = 0; //initialising as input

	DDRB = 1 << PB0 | 1 << PB1 | 1 << PB2 | 1 << PB4;

	EIMSK = 1 << INT0; // enabling interrupt on INT0
	EICRA = 1 << ISC00; // enabling interrupt on logical change
	TCNT0 = last_read_value;

	for(;;)
	{
		if (last_read_value != TCNT0)
		{
			display();
			last_read_value = TCNT0;
			if (safe_eeprom_write(&last_read_value, COUNTER1_ADDR + (current_counter * 2), 1) == 0)
			{
				uart_printstr("WRITE ERROR\r\n");
				break;
			}
		}
		if (poll_switch3() == 0)
		{
			uart_printstr("MEMORY ERROR\r\n");
			break;
		}
	}
}
