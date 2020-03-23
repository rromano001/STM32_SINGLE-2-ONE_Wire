/*
 * oled_demo.h
 *
 *  Created on: Mar 21, 2020
 *      Author: roberto-mint
 */

#ifndef OLED_DEMO_H_
#define OLED_DEMO_H_

extern int temp,humd,dstemp,dsterr;

void Oled_Init(void);
void Oled_Demo(void);
void Ht21_Display_Loop(void);

#endif /* OLED_DEMO_H_ */
