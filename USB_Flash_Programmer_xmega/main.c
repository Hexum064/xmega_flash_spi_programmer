/*
 * DIYReflowOvenV1.c
 *
 * Created: 2016-12-13 22:13:06
 * Author : Branden
 */ 
/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */
//#define __AVR_ATxmega128A4U__


#define  F_CPU 32000000UL

#include <asf.h>
#include <avr/io.h>
#include <stdio.h>
#include <avr/delay.h>

#define CS_ENABLE() (PORTC.OUTCLR = PIN4_bm)
#define CS_DISABLE() (PORTC.OUTSET = PIN4_bm)

#define MEM_WRITE_STATUS 0x01
#define MEM_WRITE 0x02
#define MEM_READ 0x03
#define MEM_READ_STATUS 0x05
#define MEM_WRITE_EN 0x06
#define MEM_FAST_READ 0x0B
#define MEM_SECT_ERASE 0x20
#define MEM_READ_ID 0x9F





#define MEM_STAT_WEN 0x02
#define MEM_STAT_BUSY 0x01

void uart_putchar(uint8_t c, FILE * stream);
FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
volatile uint8_t _runTest = 0x00;

void initSPI()
{	
	sysclk_enable_module(SYSCLK_PORT_C, SYSCLK_SPI);
	PORTC.DIRSET = PIN4_bm | PIN5_bm | PIN7_bm;
	PORTC.DIRCLR = PIN6_bm;
	SPIC.CTRL =  SPI_CLK2X_bm | SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESCALER_DIV4_gc;
}


uint8_t sendSPI(uint8_t data)
{
	SPIC.DATA = data;
	
	while(!(SPIC.STATUS & SPI_IF_bm)) {}
	
	return SPIC.DATA;
	
}

uint8_t sendDummy()
{
	return sendSPI(0x00);
}

void sendString(uint8_t *chr)
{
	while(*chr)
	{
		sendSPI(*(chr++));
	}
	
}

void memSendAddress(uint32_t address)
{
	sendSPI((uint8_t)((address >> 16) & 0xFF));
	sendSPI((uint8_t)((address >> 8) & 0xFF));
	sendSPI((uint8_t)((address >> 0) & 0xFF));
}

uint8_t getMemStatus()
{
	uint8_t out;
	CS_DISABLE();
	CS_ENABLE();
	
	sendSPI(MEM_READ_STATUS);
	out = sendDummy();	
	CS_DISABLE();
	return out;
}



void memEnableWrite()
{
	printf("Enabling Write\r\n");
	CS_DISABLE();
	CS_ENABLE();
	
	sendSPI(MEM_WRITE_EN);
	
	while(!(getMemStatus() & MEM_STAT_WEN)) {}
		
	CS_DISABLE();
	return;
}

void memEraseSector(uint32_t address)
{
	while((getMemStatus() & MEM_STAT_BUSY)) {}
			
	memEnableWrite();
	
	
	
	CS_DISABLE();
	CS_ENABLE();
	
	sendSPI(MEM_SECT_ERASE);
	memSendAddress(address);
	
	CS_DISABLE();
	return;	
}

void memRead256ToStdOut(uint32_t address)
{
	uint8_t data[16];
	while((getMemStatus() & MEM_STAT_BUSY)) {}
		
	CS_DISABLE();
	CS_ENABLE();
	sendSPI(MEM_READ);
	memSendAddress(address);;
		
	for(uint8_t i = 0; i < 32; i++)
	{
		for (uint8_t j = 0; j < 16; j++)
		{
			data[j] = sendDummy();
			printf("%02x ", data[j]);
		}
		
		printf("\t");
			
		for (uint8_t j = 0; j < 16; j++)
		{
			printf("%c", data[j]);
		}			
		printf("\r\n");
	}
	
	CS_DISABLE();
	return;	
}

void memWrite256(uint32_t address, uint8_t data)
{
	while((getMemStatus() & MEM_STAT_BUSY)) {}
	
	memEnableWrite();

	CS_DISABLE();
	CS_ENABLE();
	
	sendSPI(MEM_WRITE);
	memSendAddress(address);
	
	for(uint16_t i = 0; i < 256; i++)
	{
		sendSPI(data);
	}
	
	CS_DISABLE();
	return;	
	
}

void memWriteString(uint32_t address, uint8_t *str)
{
	while((getMemStatus() & MEM_STAT_BUSY)) {}
	
	memEnableWrite();

	CS_DISABLE();
	CS_ENABLE();
	
	sendSPI(MEM_WRITE);
	memSendAddress(address);
	
	while(*str)
	{
		sendSPI(*(str++));
	}
	
	CS_DISABLE();
	return;
	
}


void TESTWriteMem()
{
	uint32_t address = 0x00001000;
	uint8_t data[17] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00};
	printf("Testing Write\r\n");
	printf("Status: 0x%02x\r\n", getMemStatus());
	printf("First Erase\r\n");
	memEraseSector(address);

	printf("Read 256 bytes to confirm erase\r\n");
	memRead256ToStdOut(address);
	
	printf("Write string\r\n");
	memWriteString(address, "Eat, Sleep,     Rave, Repeat.");
	
	printf("Read 256 bytes to confirm write\r\n");
	memRead256ToStdOut(address);
	
	printf("Done. Status: 0x%02x\r\n", getMemStatus());
}

void uart_putchar(uint8_t c, FILE * stream)
{
	udi_cdc_putc(c);
}

bool my_callback_cdc_enable(void)
{
	return true;
}

void my_callback_cdc_disable(void)
{

}

void my_callback_rx_notify(uint8_t port)
{
	uint8_t data = udi_cdc_getc();

	udi_cdc_putc(data);
	
	if (data == 'a')
	{
		_runTest = 0xFF;
	}
}

void my_callback_tx_empty_notify(uint8_t port)
{
	
}

int main(void)
{

	CS_DISABLE();
	
	cli();

	sysclk_init();
	udc_start();
	initSPI();
	
	stdout = &mystdout;
	
	irq_initialize_vectors();


	sei();

	while(1)
	{
		if (_runTest)
		{
			TESTWriteMem();
			_runTest = 0x00;
		}
	}
	

}
