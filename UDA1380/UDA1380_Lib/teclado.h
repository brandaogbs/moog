#ifndef __TECLADO
#define __TECLADO

#include "stm32f4xx_conf.h"

void Inicia_Pino(GPIO_TypeDef *porta,uint16_t pino, GPIOMode_TypeDef mode, GPIOSpeed_TypeDef speed, GPIOPuPd_TypeDef pupd, uint32_t RCC_APB2Periph_GPIOx);
void EXTI_conf (void);
void NVIC_conf (void);

#endif
