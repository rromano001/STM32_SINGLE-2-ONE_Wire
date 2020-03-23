/*
 * sw2ow.h
 *
 *  Created on: Mar 19, 2020
 *      Author: roberto-mint
 */

#ifndef INC_SW2OW_H_
#define INC_SW2OW_H_

void OneWire_Init(void);
//void OneWire_UARTInit(uint32_t baudRate);
void OneWire_Execute(uint8_t ROM_Command,uint8_t* ROM_Buffer,
                     uint8_t Function_Command,uint8_t* Function_buffer);
// Conversion complete, Error (aka no device) call back
void OneWire_SetCallback(void(*OnComplete)(void), void(*OnErr)(void));

#endif /* INC_SW2OW_H_ */
