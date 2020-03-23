/*
 * oled_demo.c
 *
 *  Created on: Mar 21, 2020
 *      Author: roberto-mint
 *
 *      insert variables on predefined property settings:
 *      #define IIC
 *      #define  PERIOD_VALUE 1024
 *
 */

#include "main.h"

#include "num2str.h"

#include "u8g2.h"

// uncomment to display hex readout of HT21
//#define TESTHEX0

static u8g2_t u8g2;
uint32_t Timeout                 = 0; /* Variable used for Timeout management */
char tpstr[15];	// temperature string
char rhstr[15];	// humidity string
char dsstr[15];	// temperature string DS

#ifdef TESTHEX0
char tempstr[15];	// test string
#endif

int mytickcounter =0;
int dstemp=100,dsterr=0, temp,humd,state_thm=0;

uint8_t u8x8_stm32_gpio_and_delay(U8X8_UNUSED u8x8_t *u8x8,
    U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int,
    U8X8_UNUSED void *arg_ptr)
{
  switch (msg)
  {
  case U8X8_MSG_GPIO_AND_DELAY_INIT:
    //HAL_Delay(1);
    break;
  case U8X8_MSG_DELAY_MILLI:
    HAL_Delay(arg_int);
    break;
  case U8X8_MSG_GPIO_DC:
    //HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, arg_int);
    break;
  case U8X8_MSG_GPIO_RESET:
    //HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, arg_int);
    break;
  }
  return 1;
}

uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
    void *arg_ptr)
{

	  switch(msg)
	  {
	    case U8X8_MSG_BYTE_SEND:
	      Timeout = 5;
	      while( arg_int > 0 && Timeout)
	      {
			if(LL_I2C_IsActiveFlag_TXIS(I2C1))
			{
	    	  //data = (uint8_t)(*arg_ptr);
	    	  LL_I2C_TransmitData8(I2C1, *((uint8_t *)arg_ptr)); //data);
	    	  Timeout = 5;
	    	  arg_ptr++;
	    	  arg_int--;
			}
			if (LL_SYSTICK_IsActiveCounterFlag())
				Timeout--;
	      }
	      break;

	    case U8X8_MSG_BYTE_INIT:
	    	LL_I2C_Enable(I2C1);
	      break;
	    case U8X8_MSG_BYTE_SET_DC:
	      break;
	    case U8X8_MSG_BYTE_START_TRANSFER:
	    	if (!arg_int)
	    		arg_int = 2;

	    	LL_I2C_HandleTransfer(I2C1, u8x8_GetI2CAddress(u8x8), LL_I2C_ADDRSLAVE_7BIT,
	    			arg_int, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);	//  LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_NOSTARTSTOP); //

	    	//LL_I2C_GenerateStartCondition(I2C1);
	      break;
	    case U8X8_MSG_BYTE_END_TRANSFER:
	    	LL_I2C_ClearFlag_STOP(I2C1);
	    	//LL_I2C_GenerateStopCondition(I2C1);
	      break;
	    default:
	      return 0;
	  }
	  return 1;
}

void htu21d_write_cmd(uint8_t icaddr, uint8_t data, uint8_t numbytes)
{
	LL_I2C_Enable(I2C1);
	LL_I2C_HandleTransfer(I2C1, icaddr, LL_I2C_ADDRSLAVE_7BIT,
			numbytes, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);	//  LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_NOSTARTSTOP); //

	//LL_I2C_GenerateStartCondition(I2C1);


	Timeout = 5;
	while( numbytes > 0 && Timeout)
	{
		if(LL_I2C_IsActiveFlag_TXIS(I2C1))
		{
		  //data = (uint8_t)(*arg_ptr);
		  LL_I2C_TransmitData8(I2C1, data); //data);
		  Timeout = 5;
		  //arg_ptr++;
		  //arg_int--;
		  numbytes--;
		}
		if (LL_SYSTICK_IsActiveCounterFlag())
			Timeout--;
	}

	LL_I2C_ClearFlag_STOP(I2C1);
	    	//LL_I2C_GenerateStopCondition(I2C1);
}

uint16_t htu21d_read_hrel_temp(uint8_t icaddr, uint8_t numbytes)
{
	uint16_t temp =0;
	LL_I2C_Enable(I2C1);
	// Force address to read mode
	LL_I2C_HandleTransfer(I2C1, icaddr|1, LL_I2C_ADDRSLAVE_7BIT,
			numbytes, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);	//  LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_NOSTARTSTOP); //

	//LL_I2C_GenerateStartCondition(I2C1);


	Timeout = 50;
	while( numbytes > 0 &&  Timeout)
	{
	    /* Check RXNE flag value in ISR register */
	    if(LL_I2C_IsActiveFlag_RXNE(I2C1))
	    {
	      /* Read character in Receive Data register.
	      RXNE flag is cleared by reading data in RXDR register */
	      temp <<= 8;	// move actual value to MSB
	      temp |= LL_I2C_ReceiveData8(I2C1);
	      numbytes--;
	      Timeout = 5;
	    }
		if (LL_SYSTICK_IsActiveCounterFlag())
			Timeout--;
	}

	LL_I2C_GenerateStopCondition(I2C1);
	    	//LL_I2C_GenerateStopCondition(I2C1);
	HAL_Delay(1);
	LL_I2C_ClearFlag_STOP(I2C1);
	return temp;
}

void Oled_Init(void)
{
#ifndef IIC
  // SPI
  u8g2_Setup_ssd1306_128x64_noname_1(&u8g2, U8G2_R0, u8x8_byte_4wire_hw_spi,
      u8x8_stm32_gpio_and_delay);
#else
  // I2C
  u8g2_Setup_ssd1306_i2c_128x64_noname_1(&u8g2, U8G2_R0, u8x8_byte_hw_i2c,
	      u8x8_stm32_gpio_and_delay);
#endif
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);
}

void Oled_Demo(void)
{
  u8g2_FirstPage(&u8g2);
  do
  {
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&u8g2, 0, 15, "Hello World!");
//    u8g2_DrawCircle(&u8g2, 21, 40, 20, U8G2_DRAW_ALL);	// Left		Center 28, 40 Radius 20
    u8g2_DrawCircle(&u8g2, 64, 40, 20, U8G2_DRAW_ALL);	// Center	Center 28, 40 Radius 20
//    u8g2_DrawCircle(&u8g2,107, 40, 20, U8G2_DRAW_ALL);	// Right	Center 28, 40 Radius 20
    u8g2_DrawFrame(&u8g2, 0, 18, 127, 46);
  } while (u8g2_NextPage(&u8g2));
}

void Ht21_Display_Loop(void)
{
	if(mytickcounter < 50)
		mytickcounter++;
	else
	{
		mytickcounter =0;
		switch (state_thm++)
		{
		case 0:
			htu21d_write_cmd(0x80,0xe3,1); // read data temp cmd
			break;
		case 1:
			temp=htu21d_read_hrel_temp(0x80,2); // read data temp
#ifdef TESTHEX0
			hex2str(tempstr,temp,4);
#endif
			temp *= 17572;	// Integer calc, FP from DS multiply'd by 100
			temp >>=16;	// divide maxint 16 fractional
			temp -=4685;	// remove offset again By 100
			break;
		case 2:
			htu21d_write_cmd(0x80,0xe5,1); // read data humidity cmd
			break;
		case 3:
#ifdef TESTHEX0
			tempstr[4]=':';
#endif
			humd=htu21d_read_hrel_temp(0x80,2); // read data temp
#ifdef TESTHEX0
			hex2str(&tempstr[5],humd,4);
#endif
			// deviation from Datasheet, don't use Floating point multiply everything but divisor by 100
			humd *=12500;	// integer calc 2 decimal % RH
			humd >>=16;	// divide maxint 16 fractional
			humd -=600;		// remove offset base
		break;
		case 4:
			dec2str(tpstr,temp,6,2);
			tpstr[6]=0xB0; //'°';
			tpstr[7]='C';
			tpstr[8]=0;
			dec2str(rhstr,humd,6,2);
			rhstr[6]='%';
			rhstr[7]='R';
			rhstr[8]='H';
			rhstr[9]=0;
			dec2str(dsstr,dstemp,6,2);
			dsstr[6]=0xB0;
			dsstr[7]='C';
			dsstr[8]=',';
			//dsstr[8]=',';
			dec2str(&dsstr[9],dsterr,4,0);
			dsstr[13]=0;
			u8g2_FirstPage(&u8g2);
			do
			{
			  u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
#ifdef TESTHEX0
			  u8g2_DrawStr(&u8g2, 0, 15, tempstr);
			  u8g2_DrawStr(&u8g2, 0, 31, tpstr);
			  u8g2_DrawStr(&u8g2, 0, 47, rhstr);
			  u8g2_DrawStr(&u8g2, 0, 63, dsstr);
#else
			  u8g2_DrawStr(&u8g2, 0, 15, tpstr);
			  u8g2_DrawStr(&u8g2, 0, 34, rhstr);
			  u8g2_DrawStr(&u8g2, 0, 63, dsstr);
#endif
			} while (u8g2_NextPage(&u8g2));
			state_thm = 0;
		break;
		default:
			state_thm = 0;
		}
	}
}

