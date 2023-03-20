DEVICE=atmega328p
F_CPU = 16000000UL
BAUDRATE=115200
PROGRAMMER= -c arduino -b$(BAUDRATE) -p $(DEVICE)

SRCS = main.c

BINS = $(SRCS:.c=.bin)
HEX = main.hex

CC = avr-gcc


CFLAGS = -Wall -D F_CPU=$(F_CPU) -D UART_BAUDRATE=$(BAUDRATE) -mmcu=$(DEVICE) -Os

all: flash



%.bin: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(HEX): $(BINS)
	$(CC) $(CFLAGS) -o linked.bin ${BINS}
	avr-objcopy -O ihex linked.bin ${HEX}



hex: $(HEX)

flash: hex
	avrdude $(PROGRAMMER) -P /dev/ttyUSB0 -U flash:w:$(HEX):i

# flash2: hex
# 	avrdude $(PROGRAMMER) -P /dev/ttyUSB1 -U flash:w:$(HEX):i
# 	avrdude $(PROGRAMMER) -P /dev/ttyUSB0 -U flash:w:$(HEX):i


clean:
	rm -rf $(BINS) $(HEX)
	
re: clean all
