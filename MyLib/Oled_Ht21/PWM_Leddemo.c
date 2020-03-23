/*
 * PWM_Leddemo.c
 *
 *  Created on: Mar 21, 2020
 *      Author: roberto-mint
 */

#include "main.h"

#define PERIOD_VALUE 1024

extern TIM_HandleTypeDef htim14;

int dir=0,intens=0;

void PWM_LED_Loop(void)
{
  if (!dir)
  {
	 intens++;
	 if(intens>=PERIOD_VALUE)
	  dir=1;
  }
  else
  {
	 if(intens>0)
		 intens--;
	 else
		 dir=0;
  }
  // Set the pulse value for channel
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, intens);
/*  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, intens);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, intens);
  __HAL_TIM_SET_COMPARE(&htim17, TIM_CHANNEL_1, intens);
*/
}

void PWM_LED_Init(void)
{
//	  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
//	  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
	  HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
//	  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
//	  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
//	  Oled_Init();
//	  Oled_Demo();
}
