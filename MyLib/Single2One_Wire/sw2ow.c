/*
 * sw2ow.c
 *
 *  Created on: Mar 19, 2020
 *      Author: roberto-mint
 *
 *	V 1.00 Beta	Mar 23 First Beta release (tested overnite with read failure counter unchanged)
 *      Mar 22 added LL core function to manage RX TX data (HAL seem fail on large buffers?? ) hang on irq too
 *      Mar 21 Changing functions to reduce code footptint
 *      Mar 19 start project
 */

#include "main.h"
#ifdef USE_FREERTOS
#include "cmsis_os.h"
#endif

#include "sw2ow.h"
#include <string.h>

#define DIRECT_OWCALLBAK 1
#define OWD_RX	0
#define OWD_TX	1

/*
 * USART2(Tx=D5 Rx=D6)
 */


typedef struct
	{
    volatile uint8_t Main_SM;              //Communication Phase 1: Main_SM
    volatile uint8_t ROM_SM;        //Communication Phase 2: Rom State Machine
    volatile uint8_t Function_SM;   //Communication Phase 3: DS function State Machine
    volatile uint8_t *ROM_TxBuffer;	// Buffer for ROM codes requiring transmit some data (ROM)
    volatile uint8_t *ROM_RxBuffer;	// Buffer for ROM codes requiring receive some data
    volatile uint8_t ROM_TxCount;	// number of byte to transmit
    volatile uint8_t ROM_RxCount;	// number of byte to be received
    volatile uint8_t *Function_TxBuffer;	// Buffer for Funct codes requiring transmit some data
    volatile uint8_t *Function_RxBuffer;	// Buffer for Funct codes requiring receive some data
    volatile uint8_t Function_TxCount;	// number of byte to transmit
    volatile uint8_t Function_RxCount;	// number of byte to be received
    volatile uint8_t ROM_CmdCode;        // Communication Phase 2: Rom code
    volatile uint8_t Function_CmdCode;   // Communication Phase 3: DS function code
    volatile uint8_t DataReady;			// set when data is ready on scratchpad
    volatile uint8_t NoDevice;			// Set When no device answer with presence pulse
	} State;
State state;

uint8_t internal_Buffer_rx[10];
uint8_t internal_Buffer_tx[10];
typedef struct
	{
		void(*OnComplete)(void);
		void(*OnErr)(void);
	}OneWire_Callback;

volatile OneWire_Callback onewire_callback={NULL,NULL};

volatile uint8_t OW_BSend =0;
static	uint8_t OWB_index=0;
static  uint8_t OWB_Len;
static  uint8_t OWB_Dir;
volatile  uint8_t *OWB_BufferPtr;
uint8_t	FunctionBuffer[32];
uint8_t	ROMBuffer[32];


// Header declaration
void StateMachine(void);
void OneWire_SetCallback(void(*OnComplete)(void), void(*OnErr)(void));
uint8_t ROMStateMachine(void);
uint8_t FunctionStateMachine(void);
void OW_DoRTXBuffer(void);
void OW_RTXByte(uint8_t val); //, uint16_t len) byte has fixed len 8
void OW_RTX(uint16_t len);

#ifndef DIRECT_OWCALLBAK
void OneWire_TxCpltCallback()
#else
/**
  * @brief  Function called from DMA1 IRQ Handler when Tx transfer is completed
  * @param  None
  * @retval None
  */
void DMA1_TransmitComplete_Callback(void)
//#endif
{
	/* DMA Tx transfer completed */
	/* Disable DMA1 Tx Channel */
	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
}
#endif

#ifndef DIRECT_OWCALLBAK
void OneWire_RxCpltCallback(void)
#else
/**
  * @brief  Function called from DMA1 IRQ Handler when Rx transfer is completed
  * @param  None
  * @retval None
  */
void DMA1_ReceiveComplete_Callback(void)
#endif
{
	  /* DMA Rx transfer completed */
	/* Disable DMA1 Rx Channel */
	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_3);
	if(!OW_BSend)	// Skip on multiple byte send (HAL Bug??)
		StateMachine();
	else
		OW_DoRTXBuffer();
}


 /* OneWire_SendBytes & OneWire_ReadBytes */

void OneWire_SetCallback(void(*OnComplete)(void), void(*OnErr)(void))
{
	onewire_callback.OnErr = OnErr;
	onewire_callback.OnComplete = OnComplete;
}

/*
 * Tilen Majerle Library CRC 8-5-4+1
 * Maxim datasheet report CRC 8541 on writing
 *  Fig 11 has different bit mask, miss one tap.
*/
uint8_t TM_OneWire_CRC8(uint8_t *addr, uint8_t len) {
	uint8_t crc = 0, inbyte, i, mix;

	while (len--) {
		inbyte = *addr++;
		for (i = 8; i; i--) {
			mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) {
				crc ^= 0x8C;
			}
			inbyte >>= 1;
		}
	}

	/* Return calculated CRC */
	return crc;
}

uint8_t DSCRC_OK(void)
{
	if (FunctionBuffer[8]==TM_OneWire_CRC8(FunctionBuffer,8))
		return(1);
	return(0);
}

// Set OW baudrate
void OneWire_UARTInit(uint32_t baudRate)
{
	LL_USART_Disable(OWHUART);
	LL_USART_SetBaudRate(OWHUART, SystemCoreClock, LL_USART_OVERSAMPLING_16, baudRate);
	LL_USART_Enable(OWHUART);
}

// Prepare Reset/Presence detect, baudrate 9600 to transmit 0xf0 byte
void OneWire_Init()
{
	OneWire_UARTInit(9600);
}

// translate Rx char buffer to byte value (0xfe, 0xff 1 otherwise 0)
uint8_t	OW_Buffer2Byte(void)
{
	uint8_t rvalue=0;
	for (uint8_t i=0;i<8;i++)
	{
		if(internal_Buffer_rx[i]>= 0xfe)
			rvalue |=1<<i;	// set bit to 1
		//rvalue <<= 1;	// shift right
	}
	return(rvalue);
}

// Set internal var and start transfer multiple byte to OW bus
void OW_SetRTXBuffer(volatile uint8_t *buffer, uint8_t len, uint8_t dir)
{
	OWB_index=0;	// init buffer indexer
	OWB_Len= len;	// set Transfer len
	OWB_Dir = dir;	// Set RX or tx mode
	OWB_BufferPtr=buffer;	// Set buffer pointer to actual buffer
	OW_DoRTXBuffer();	// start transfer
}

// Transfer loop of multiple byte called by DMA transfer complete (nbytes transferred = 8*bufferlen)
void OW_DoRTXBuffer(void)
{
	if(OWB_Dir==OWD_TX)
		OW_RTXByte(OWB_BufferPtr[OWB_index]);	// tx buffer
	else
	{
		OW_RTXByte(0xff);	// tx 0xff to receive back data
		if(OW_BSend)	// Skip first time need transmit only then receive
			OWB_BufferPtr[OWB_index-1]=	OW_Buffer2Byte();	// convert received string to byte, store (rx buffer pointer is one less TX)
	}
	if (!OWB_index&&!OW_BSend)
		OW_BSend=1;	// return here till last OWByte
	if (++OWB_index>OWB_Len)
	{
		OW_BSend =0;	// terminate transfer loop
		StateMachine();	// execute state machine on last byte
	}
}

// convert byte value to a string formatted as required by OW protocol
void OW_RTXByte(uint8_t val) //, uint16_t len) byte has fixed len 8
{
	for (uint8_t i=0;i<8;i++)
		internal_Buffer_tx[i]=((val>>i)&0x01)?0xff:0x00;
	OW_RTX(8);	// 1 byte 8 bit -> 8 char 2 OW
}

void OW_RTX(uint16_t len)
{
	/* Disable DMA Channel Rx & Tx, if not SetDataLenght and ConfigAddress fail */
	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_3);
	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);

	// Set Dma TX Address and len
	LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_2,
						 (uint32_t)internal_Buffer_tx,
						 LL_USART_DMA_GetRegAddr(OWHUART, LL_USART_DMA_REG_DATA_TRANSMIT),
						 LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_CHANNEL_2));
	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, len);

	// Set Dma RX Address and len
	LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_3,
						 LL_USART_DMA_GetRegAddr(OWHUART, LL_USART_DMA_REG_DATA_RECEIVE),
						 (uint32_t)internal_Buffer_rx,
						 LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_CHANNEL_3));
	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, len);

	/* Enable DMA transfer complete/error interrupts  */
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_2);
	LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_2);
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_3);
	LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_3);

	/* Enable DMA RX Interrupt */
	LL_USART_EnableDMAReq_RX(OWHUART);
	/* Enable DMA TX Interrupt */
	LL_USART_EnableDMAReq_TX(OWHUART);

	/* Enable DMA Channel Rx */
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);
	/* Enable DMA Channel Tx */
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
}

void StateMachine(void)
{
    switch (state.Main_SM)
    {
	case 0: // start the Main_SM, reset device/test presence;
		OneWire_UARTInit(9600);
		internal_Buffer_rx[0]=0xf0;
		internal_Buffer_tx[0]=0xf0;
		state.Main_SM++;
		OW_RTX(1);
	break;
	case 1: // check if the device exist or not.
		if (internal_Buffer_rx[0]==0xf0)
		{
			if(onewire_callback.OnErr!=NULL)
				onewire_callback.OnErr();	// No device presence bit
		}
		else
			ROMStateMachine();			// Start ROM command @115200
		state.Main_SM++;
	break;
	case 2:
		if (ROMStateMachine()==0)
		{
			state.Main_SM++;
			FunctionStateMachine();		// Start Function command @115200
		}
	break;
	case 3:
		if (FunctionStateMachine()==0)
		{
			// OW transaction complete
			LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
			state.Main_SM++;
			if(onewire_callback.OnComplete!=NULL)
				onewire_callback.OnComplete();
		}
	//else
	break;
//	default:
    }
//    return ;
}

uint8_t ROMStateMachine(void)
{
    switch(state.ROM_SM)
    {
	case 0: // start the ROM_SM command by sendingROM_SMmmand
		OneWire_UARTInit(115200);
		OW_RTXByte(state.ROM_CmdCode);
		state.ROM_SM++;
	break;
	case 1: // continue by sending necessary Tx buffer
		if (state.ROM_TxCount!=0)
		{
			OW_SetRTXBuffer(state.ROM_TxBuffer, state.ROM_TxCount, OWD_TX);
			state.ROM_SM++;
		}
		else
			if (state.ROM_RxCount!=0)
			{
				OW_SetRTXBuffer(state.ROM_RxBuffer, state.ROM_RxCount, OWD_RX);
				state.ROM_SM++;
			}
			else
				state.ROM_SM=0;
	break;
	case 2:
		// Terminate ROM command and data transaction
		state.ROM_SM=0;
		break;
	}
    return state.ROM_SM;
}

uint8_t FunctionStateMachine(void)
{
    switch(state.Function_SM)
    {
	case 0:
		// Start Function and relative data read write transaction
		OneWire_UARTInit(115200);	// always set on ROM transaction
		OW_RTXByte(state.Function_CmdCode);
		state.Function_SM++;
		break;
	case 1: // continue by sending/receiving necessary Tx/Rx buffer
		if (state.Function_TxCount!=0)
		{
			//  OW TX data
			OW_SetRTXBuffer(state.Function_TxBuffer, state.Function_TxCount, OWD_TX);
			state.Function_SM++;
		}
		else
			if (state.Function_RxCount!=0)
			{
				// OW Rx data
				OW_SetRTXBuffer(state.Function_RxBuffer, state.Function_RxCount, OWD_RX);
				state.Function_SM++;
			}
			else
				state.Function_SM=0;
	break;
	case 2:
		// Adsorbing state if not previously terminated
		state.Function_SM=0;
		break;
	}
    return (state.Function_SM);
}


void OneWire_Execute(uint8_t ROM_CmdCode,uint8_t* ROM_Buffer,uint8_t Function_CmdCode,uint8_t* Function_buffer)
{
	// Bus transaction, Send Presence detection Pulse, if detected send/receive ROM,Function codes and data
    memset(&(state),0,sizeof(State));
    state.ROM_CmdCode=ROM_CmdCode;
    state.Function_CmdCode=Function_CmdCode;
    switch (ROM_CmdCode)
    {
        case 0x33:  // Read ROM
            state.ROM_RxBuffer=ROM_Buffer;
            state.ROM_RxCount=8; //8 byte
            break;
        case 0x55:  // Match ROM
            state.ROM_TxBuffer=ROM_Buffer;
            state.ROM_TxCount=8;
            break;
        case 0xf0: break; // Search ROM it might be too hard to implement you might need to refer to Chapter "C.3. Search ROM Command" in the pdf here:http://pdfserv.maximintegrated.com/en/an/AN937.pdf
        case 0xec: break; // Alarm Search it might be too hard to implement refer to http://pdfserv.maximintegrated.com/en/an/AN937.pdf if in need.
        case 0xcc: break; // Skip Rom just send the 0xcc only since the code is implement one-slave need.
    }
    switch (Function_CmdCode)
    {
        case 0x44: break; // Convert T need to transmit nothing or we can read a 0 while the temperature is in progress read a 1 while the temperature is done.
        case 0x4e:  // Write Scratchpad
            state.Function_TxBuffer=Function_buffer;
            state.Function_TxCount=3;
            break;
        case 0x48: break; // Copy Scratchpad need to transmit nothing
        case 0xbe:  // Read Scratchpad
            state.Function_RxBuffer=Function_buffer;
            state.Function_RxCount=9;
            break;
        case 0xb8: break; // Recall EEPROM return transmit status to master 0 for in progress and 1 is for done.
        case 0xb4: break; // read power supply only work for undetermined power supply status. so don't need to implement it
    }
    StateMachine();
}

#ifndef DIRECT_OWCALLBAK
/**
  * @brief  Function called from DMA1 IRQ Handler when Tx transfer is completed
  * @param  None
  * @retval None
  */
void DMA1_TransmitComplete_Callback(void)
{
  /* DMA Tx transfer completed */
	OneWire_TxCpltCallback();
}

/**
  * @brief  Function called from DMA1 IRQ Handler when Rx transfer is completed
  * @param  None
  * @retval None
  */
void DMA1_ReceiveComplete_Callback(void)
{
  /* DMA Rx transfer completed */
	OneWire_RxCpltCallback();
}
#endif

/**
  * @brief  Function called in case of error detected in USART IT Handler
  * @param  None
  * @retval None
  */
void USART_TransferError_Callback(void)
{
  /* Disable DMA1 Tx Channel */
  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);

  /* Disable DMA1 Rx Channel */
  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_3);

  /* Set Led to Fast Blinking mode to indicate error */
  //LED_Blinking(LED_BLINK_ERROR);
}


