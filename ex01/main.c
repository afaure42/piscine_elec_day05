#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/twi.h>

#define MAGIC_NUMBER 0x5A
#define MAGIC_NUMBER_ADDR (uint8_t *)0x0003
#define COUNTER1_ADDR (uint8_t *)0x00FF


#define SCL_FREQUENCY 100000
#define TWI_BITRATE (((F_CPU / SCL_FREQUENCY) - 16) / 2)

#define ACK (1 << TWEA)
#define NACK 0
#define EXPANDER_ADDR 0b00100000
#define TW_WRITE 0
#define TW_READ 1

uint8_t last_read_value = 0;
uint8_t current_counter = 0;
uint8_t last_button_value = 1;

void display(void);
void ft_eeprom_init(void);

void display()
{
	//clearing output port flags
	PORTB = ~(1 << PB0 | 1 << PB1 | 1 << PB2 | 1 << PB4);

	PORTB |= (TCNT0 & 0b0111) | ((TCNT0 & 0b1000) << 1);
}

void ft_eeprom_init(void)
{
	if (eeprom_read_byte(MAGIC_NUMBER_ADDR) != MAGIC_NUMBER)
	{
		eeprom_write_byte(MAGIC_NUMBER_ADDR, MAGIC_NUMBER);
		eeprom_write_block("\0\0\0\0", COUNTER1_ADDR, 4);
		last_read_value = 0;
	}
	else
		last_read_value = eeprom_read_byte(COUNTER1_ADDR + current_counter);
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

void poll_switch3()
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
		current_counter++;
		if (current_counter == 4)
			current_counter = 0;
		
		last_button_value = 0;
		last_read_value = eeprom_read_byte(COUNTER1_ADDR + current_counter);
		TCNT0 = last_read_value;
		display();
	}
	if (last_button_value == 0 && (buffer & 1) == 1)
	{
		last_button_value = 1;
	}
}

int main()
{
	ft_timer0_init();
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
			eeprom_write_byte(COUNTER1_ADDR + current_counter, TCNT0);
			last_read_value = TCNT0;
		}
		poll_switch3();
	}
}
