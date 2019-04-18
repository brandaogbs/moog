#include "usart.h"

void configura_usart2(void) {
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	// configura os pinos da usart2 (pa2 e pa3)
	GPIO_InitTypeDef config_pin;
	config_pin.GPIO_Pin = (GPIO_Pin_10 | GPIO_Pin_11);
	config_pin.GPIO_Mode = (GPIO_Mode_AF);
	config_pin.GPIO_Speed = (GPIO_Speed_50MHz);
	config_pin.GPIO_OType = (GPIO_OType_PP);
	config_pin.GPIO_PuPd = (GPIO_PuPd_NOPULL);
	GPIO_Init(GPIOC, &config_pin);

	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_USART3);

	// inicia a usart2
	USART_InitTypeDef usart_config;
	usart_config.USART_BaudRate = 115200;
	usart_config.USART_WordLength = USART_WordLength_8b;
	usart_config.USART_StopBits = USART_StopBits_1;
	usart_config.USART_Parity = USART_Parity_No;
	usart_config.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usart_config.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART3, &usart_config);

	// Habilita a interrupção da USART
	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	 NVIC_InitTypeDef nvic_config;
	 nvic_config.NVIC_IRQChannel = USART3_IRQn;
	 nvic_config.NVIC_IRQChannelPreemptionPriority = 0;
	 nvic_config.NVIC_IRQChannelSubPriority = 0;
	 nvic_config.NVIC_IRQChannelCmd = ENABLE;

	 NVIC_Init(&nvic_config);

	 USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART3, ENABLE);
}

void enviachar_usart2(char c) {
	while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET)
		;
	USART_SendData(USART3, c);
}

void enviastring_usart2(char *str) {
	int i;

	for (i = 0; i < 50; i++) {
		if (*(str + i) == 0) {
			i = 0;
			break;
		}

		enviachar_usart2(*(str + i));
	}
}

char recebechar_usart2(void) {
	char dado;

	while (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET);
	dado = USART_ReceiveData(USART3);

	return dado;
	//enviachar_usart2(dado);
}
