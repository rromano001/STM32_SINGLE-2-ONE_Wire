/* Work in progress, Project is working but require cleaning.
It work on Single wire Onewire over UART using just one pin no external hardware.
*/


HAL_StatusTypeDef HAL_SnglWireFullDuplex_EnableRTX(UART_HandleTypeDef *huart)
{
  __HAL_LOCK(huart);
  huart->gState = HAL_UART_STATE_BUSY;

  /* Enable the USART's transmit and receive interface by setting the TE bit in the USART CR1 register */
  SET_BIT(huart->Instance->CR1, (USART_CR1_TE | USART_CR1_RE));

  huart->gState = HAL_UART_STATE_READY;

  __HAL_UNLOCK(huart);

  return HAL_OK;
}


HAL_StatusTypeDef HAL_SnglWireFullDuplex_DisableRTX(UART_HandleTypeDef *huart)
{
  __HAL_LOCK(huart);
  huart->gState = HAL_UART_STATE_BUSY;

  /* Disable both Receive and transmit by Clearing TE and RE bits */
  CLEAR_BIT(huart->Instance->CR1, (USART_CR1_TE | USART_CR1_RE));

  huart->gState = HAL_UART_STATE_READY;

  __HAL_UNLOCK(huart);

  return HAL_OK;
}


//  skeleton preview (work in progress)
void OW_Send_ReceiveByte(uint8_t Exchange[])
{

// reset code
		OneWire_UARTInit(9600);
		internal_Buffer_rx[0]=0xf0;
		internal_Buffer_tx[0]=0xf0;
		//HAL_HalfDuplex_EnableTransmitter(&huart4);
		HAL_HalfDuplex_EnableRTX(&huart4);
		HAL_UART_Receive_IT(&huart4,&(internal_Buffer_rx[0]),1);
		HAL_UART_Transmit_IT(&huart4,&(internal_Buffer_tx[0]),1);
		//HAL_HalfDuplex_EnableReceiver(&huart4);
// byte exchange
		OneWire_UARTInit(115200);
		for (uint8_t i=0;i<8;i++)
			internal_Buffer_tx[i]=((state.ROM_Command>>i)&0x01)?0xff:0x00;
		//HAL_HalfDuplex_EnableTransmitter(&huart4);
		HAL_UART_Receive_IT(&huart4,&(internal_Buffer_rx[0]),8);
		HAL_UART_Transmit_IT(&huart4,&(internal_Buffer_tx[0]),8);
		//HAL_HalfDuplex_EnableReceiver(&huart4);

}

void OneWire_Init()
{
	OneWire_UARTInit(9600);
}
// Declare a USART_HandleTypeDef handle structure.
void OneWire_UARTInit(uint32_t baudRate)
{
/*
	huart4.Instance = UART4;
	huart4.Init.BaudRate = 115200;
	huart4.Init.WordLength = UART_WORDLENGTH_8B;
	huart4.Init.StopBits = UART_STOPBITS_1;
	huart4.Init.Parity = UART_PARITY_NONE;
	huart4.Init.Mode = UART_MODE_TX_RX;
	huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart4.Init.OverSampling = UART_OVERSAMPLING_16;
	huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_HalfDuplex_Init(&huart4) != HAL_OK)
	{
		Error_Handler();
	}
*/
	huart4.Instance=UART4; // USART2;
    huart4.Init.BaudRate = baudRate;
    huart4.Init.WordLength = UART_WORDLENGTH_8B;
    huart4.Init.StopBits = UART_STOPBITS_1;
    huart4.Init.Parity = UART_PARITY_NONE;
    huart4.Init.Mode = UART_MODE_TX_RX;
    huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling = UART_OVERSAMPLING_16;
//    HAL_UART_Init(&huart4);
    HAL_HalfDuplex_Init(&huart4);
 //   return ;
}

