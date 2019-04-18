#include "teclado.h"

void Inicia_Pino(GPIO_TypeDef *porta, uint16_t pino, GPIOMode_TypeDef mode,
		GPIOSpeed_TypeDef speed, GPIOPuPd_TypeDef pupd,
		uint32_t RCC_APB2Periph_GPIOx) {

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOx, ENABLE);

	GPIO_InitTypeDef GPcoisas;

	GPcoisas.GPIO_Mode = mode;
	GPcoisas.GPIO_Speed = speed;
	GPcoisas.GPIO_Pin = pino;
	GPcoisas.GPIO_PuPd = pupd;

	GPIO_Init(porta, &GPcoisas);
}

void EXTI_conf() {
	EXTI_InitTypeDef EXTI_Biroscas;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource1);

	EXTI_Biroscas.EXTI_LineCmd = ENABLE;
	EXTI_Biroscas.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_Biroscas.EXTI_Trigger = EXTI_Trigger_Rising_Falling;

	EXTI_Biroscas.EXTI_Line = EXTI_Line1;
	EXTI_Init(&EXTI_Biroscas);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource2);
	EXTI_Biroscas.EXTI_Line = EXTI_Line2;
	EXTI_Init(&EXTI_Biroscas);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD, EXTI_PinSource3);
	EXTI_Biroscas.EXTI_Line = EXTI_Line3;
	EXTI_Init(&EXTI_Biroscas);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource4);
	EXTI_Biroscas.EXTI_Line = EXTI_Line4;
	EXTI_Init(&EXTI_Biroscas);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource5);
	EXTI_Biroscas.EXTI_Line = EXTI_Line5;
	EXTI_Init(&EXTI_Biroscas);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource6);
	EXTI_Biroscas.EXTI_Line = EXTI_Line6;
	EXTI_Init(&EXTI_Biroscas);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource7);
	EXTI_Biroscas.EXTI_Line = EXTI_Line7;
	EXTI_Init(&EXTI_Biroscas);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource8);
	EXTI_Biroscas.EXTI_Line = EXTI_Line8;
	EXTI_Init(&EXTI_Biroscas);


}

void NVIC_conf(void) {
	NVIC_InitTypeDef nVICKY;

	nVICKY.NVIC_IRQChannelCmd = ENABLE;
	nVICKY.NVIC_IRQChannelPreemptionPriority = 15; // Não importa, só há sub-prioridade, então a menor
	nVICKY.NVIC_IRQChannelSubPriority = 0; // Maior sub-prioridade possível

	nVICKY.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_Init(&nVICKY);

	nVICKY.NVIC_IRQChannel = EXTI2_IRQn;
	NVIC_Init(&nVICKY);

	nVICKY.NVIC_IRQChannel = EXTI4_IRQn;
	NVIC_Init(&nVICKY);

	nVICKY.NVIC_IRQChannel = EXTI3_IRQn;
	NVIC_Init(&nVICKY);

	nVICKY.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_Init(&nVICKY);

}
