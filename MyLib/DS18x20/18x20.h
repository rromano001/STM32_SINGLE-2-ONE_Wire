/*
 * 18x20.h
 *
 *  Created on: Mar 19, 2020
 *      Author: roberto-mint
 */

#ifndef INC_18X20_H_
#define INC_18X20_H_

// External access to restart State Machine
extern volatile uint8_t TempRdy;
// Restart State machine clearing TempRdy
void restart__ds18x20(void);
uint8_t test_single_ds18x20(void);
// temp is 100 multiplied (2 decimal digits)
int readtempds18b20(uint8_t sensnum);
int readtempds18s20(uint8_t sensnum);
// temp is 10,000 multiplied (4 decimal digits)
int readtempds18b20_4(uint8_t sensnum);

uint8_t DSCRC_OK(void);	// from sw2ow

#endif /* INC_18X20_H_ */
