#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define MAGIC_NUMBER 0x5A
#define MAGIC_NUMBER_ADDR (uint8_t *)0x0000
#define COUNTER_ADDR (uint8_t *)0x00FF

uint8_t volatile g_counter = 0;
uint8_t volatile g_switch1_counter = 0;


void display(void);
void ft_eeprom_init(void);


ISR(INT0_vect)
{
	g_switch1_counter++;
	if(g_switch1_counter & 1)
	{
		g_counter++;
		eeprom_write_byte(COUNTER_ADDR, g_counter);
		display();
	}
	_delay_ms(1);
	EIFR = 1 << INTF0;
}

void display()
{
	//clearing output port flags
	PORTB = ~(1 << PB0 | 1 << PB1 | 1 << PB2 | 1 << PB4);

	PORTB |= (g_counter & 0b0111) | ((g_counter & 0b1000) << 1);
}

void ft_eeprom_init(void)
{
	if (eeprom_read_byte(MAGIC_NUMBER_ADDR) != MAGIC_NUMBER)
	{
		eeprom_write_byte(MAGIC_NUMBER_ADDR, MAGIC_NUMBER);
		eeprom_write_byte(COUNTER_ADDR, 0);
		g_counter = 0;
	}
	else
		g_counter = eeprom_read_byte(COUNTER_ADDR);
}


int main()
{
	ft_eeprom_init();

	DDRD = 0; //initialising as input

	DDRB = 1 << PB0 | 1 << PB1 | 1 << PB2 | 1 << PB4;

	EIMSK = 1 << INT0; // enabling interrupt on INT0
	EICRA = 1 << ISC00; // enabling interrupt on logical change

	display();	
	sei();
	for(;;);
}
