/*
 * sw2ow_irq.h
 *
 *  Created on: Mar 19, 2020
 *      Author: roberto-mint
 */

#ifndef INC_SW2OW_IRQ_H_
#define INC_SW2OW_IRQ_H_

/*
 * Include this code on void DMA1_Channel2_3_IRQHandler(void) user code:
 *
 *

 	// TX
	  if(LL_DMA_IsActiveFlag_TC2(DMA1))
	  {
	    LL_DMA_ClearFlag_GI2(DMA1);
	    // Call function Transmission complete Callback
	    DMA1_TransmitComplete_Callback();
	  }
	  else if(LL_DMA_IsActiveFlag_TE2(DMA1))
	  {
	    // Call Error function
	    USART_TransferError_Callback();
	  }

	  // RX
	  if(LL_DMA_IsActiveFlag_TC3(DMA1))
	  {
	    LL_DMA_ClearFlag_GI3(DMA1);
	    // Call function Reception complete Callback
	    DMA1_ReceiveComplete_Callback();
	  }
	  else if(LL_DMA_IsActiveFlag_TE3(DMA1))
	  {
	    // Call Error function
	    USART_TransferError_Callback();
	  }

 *
 *
 *
 */


void DMA1_TransmitComplete_Callback(void);
void DMA1_ReceiveComplete_Callback(void);
void USART_TransferError_Callback(void);

#endif /* INC_SW2OW_IRQ_H_ */
