/*
 * 18x20.c
 *
 *  Created on: Mar 19, 2020
 *      Author: roberto-mint
 *
 *	V 1.00 Beta	Mar 23 Released
 *      Mar 19 start project
 */

#include "main.h"
#ifdef USE_FREERTOS
#include "cmsis_os.h"
#endif

#include "sw2ow.h"
#include <string.h>
#include "18x20.h"

volatile uint8_t TempRdy =0;

// Header declaration

uint8_t	FunctionBuffer[32];
uint8_t	ROMBuffer[32];

void DS18x20_OnComplete(void)
{

}

void DS18x20_Error(void)
{

}

int readtempds18s20(uint8_t sensnum)
{
	// 1 fractional bit 2 decimal digit
	return (100*(FunctionBuffer[1]*0x100+FunctionBuffer[0])/2);
}

int readtempds18b20(uint8_t sensnum)
{
	// 4 fractional bit 2 decimal digit
	return (100*(FunctionBuffer[1]*0x100+FunctionBuffer[0])/16);
}

int readtempds18b20_4(uint8_t sensnum)
{
	// 4 fractional bit 4 decimal digit
	return (10000*(FunctionBuffer[1]*0x100+FunctionBuffer[0])/16);
}

void restart__ds18x20(void)
{
	TempRdy =0;
}

uint8_t test_single_ds18x20(void)
{
	static uint8_t statenum=0;
	static uint16_t tickcount=1;
	if (tickcount)
		tickcount--;
	else
	{
		switch (statenum)
		{
		case 0:
			//TempRdy =0;
			OneWire_Init();
			OneWire_SetCallback(DS18x20_OnComplete, DS18x20_Error);
			OneWire_Execute(0xcc,0,0,0); /* skip rom phase */
			tickcount=100;
			statenum++;
			break;
		case 1:
			FunctionBuffer[0]=1;
			FunctionBuffer[1]=2;
			FunctionBuffer[2]=0x1f;
			OneWire_Execute(0xcc,0,0x4e,FunctionBuffer); /* skip rom phase set TH TL Config */
			tickcount=100;
			statenum++;
			break;
		case 2:
			OneWire_Execute(0xcc,0,0x44,0); /* start to Convert T */
			tickcount=1100;	// 1 tick per mS 1.1 Sec Delay
			statenum++;
			break;
		case 3:
			OneWire_Execute(0xcc,ROMBuffer,0xbe,FunctionBuffer); /* start to read configuration & result */
			tickcount=300;
			statenum++;
			break;
		case 4:
			TempRdy=1;
			statenum++;
			break;
		case 5:
			if(!TempRdy)
			{
				statenum=0;
				tickcount=10;
			}
		}

	}
	return(TempRdy);
}

