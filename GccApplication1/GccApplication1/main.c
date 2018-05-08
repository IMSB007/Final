#define F_CPU 8000000UL
#include <avr/io.h>
#include <stdint.h> // needed for uint8_t
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdbool.h>
#define SAMPLE_PERIOD	20  	
#define BUFF_SIZE		50		

#define BAUD 9600
#define MYUBRR F_CPU/8/BAUD -1
volatile char ReceivedChar;
volatile uint8_t ADCvalue;

uint16_t readData, preReadData;		   		
uint16_t timeCount = 0;        
uint16_t firstTimeCount = 0;   
uint16_t secondTimeCount = 0;   


uint16_t IBI;
int BPM;

uint16_t data[BUFF_SIZE] = {0};
uint8_t index = 0; 				
uint16_t max, min, mid;			
		

bool PULSE = false;				
bool PRE_PULSE = false;        

uint8_t pulseCount = 0;			
uint32_t i;




uint16_t Get_Array_Max(uint16_t * array, uint32_t size);
uint16_t Get_Array_Min(uint16_t * array, uint32_t size);


int USART0SendByte(char u8Data, FILE *stream)
{
	while(!(UCSR0A&(1<<UDRE0))){};
	// Transmit data
	UDR0 = u8Data;
	return 0;
}
	
ISR (USART_RX_vect)
{
	ReceivedChar = UDR0; // Read data from the RX buffer
	UDR0 = ReceivedChar; // Write the data to the TX buffer
}
ISR(ADC_vect)
{
	ADCvalue = ADCH; // only need to read the high value for 8 bit
}
//set stream pointer
FILE usart0_str = FDEV_SETUP_STREAM(USART0SendByte,NULL,  _FDEV_SETUP_RW);
ISR(TIMER1_OVF_vect)
{
	cli();
	readData = ADCvalue;	
	PRE_PULSE = PULSE;	
	if(readData >= 180 && PRE_PULSE == false && PULSE == false)
	{
		
		PULSE = true;
	}
	else if(readData < 180 && PRE_PULSE == true && PULSE == true)
	{
		
		PULSE = false;
	}
	
	if (PRE_PULSE == false && PULSE == true)  
	{
		pulseCount++;
		pulseCount %= 2;
		if(pulseCount == 1)
		{
			firstTimeCount = timeCount;   
		}
		if(pulseCount == 0)  
		{
			secondTimeCount = timeCount;  
			timeCount = 0;

			if ( (secondTimeCount > firstTimeCount))
			{
				IBI = (secondTimeCount - firstTimeCount)* SAMPLE_PERIOD;	
				BPM = 60000 / IBI;  
			
			}
		}
	}
	if(PRE_PULSE == false && PULSE == false && timeCount > 200)
	{
		BPM = 0;
		IBI = 0;
	}
	printf("BPM %u IBI %u     \r\n",BPM,IBI);
	sei();
	timeCount++;  
	
		TCNT1 = 65535;
		
}
uint16_t Get_Array_Max(uint16_t * array, uint32_t size)
{
	uint16_t max = array[0];
	uint32_t i;	
	for (i = 1; i < size; i++)
	{
		if (array[i] > max)
		max = data[i];
	}
	
	return max;
}

uint16_t Get_Array_Min(uint16_t * array, uint32_t size)
{
	uint16_t min = array[0];
	uint32_t i;
	
	for (i = 1; i < size; i++)
	{
		if (array[i] < min)
		min = data[i];
	}
	return min;
}
int main( void )
{
	stdin = stdout = &usart0_str;
	/*Set baud rate */
	UBRR0H = (MYUBRR >> 8);
	UBRR0L = MYUBRR;
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0)|(1<<RXCIE0); // Enable receiver and transmitter
	UCSR0C |= (1 << UCSZ00) | (1 << UCSZ00); // Set frame: 8data, 1 stp
	
	ADMUX = 0; // use ADC0
	ADMUX |= (1 << REFS0); // use AVcc as the reference
	ADMUX |= (1 << ADLAR); // Right adjust for 8 bit resolution
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // 128 prescale for 16Mhz
	ADCSRA |= (1 << ADATE); // Set ADC Auto Trigger Enable
	ADCSRB = 0; // 0 for free running mode
	ADCSRA |= (1 << ADEN); // Enable the ADC
	ADCSRA |= (1 << ADIE); // Enable Interrupts
	ADCSRA |= (1 << ADSC); // Start the ADC conversion
	
	
	TIMSK1 = (1<<TOIE1);
	TCNT1 = 65535;//57724; // 65535 - 7811
	TCCR1B = (1<<CS12)|(1<<CS10); //1024
	sei();
	while(1)
	{

	}
	return 1;
}



