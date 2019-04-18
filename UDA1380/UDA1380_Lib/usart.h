#ifndef __USART
#define __USART

#include "stm32f4xx_conf.h"

void configura_usart2(void);
void enviachar_usart2(char c);
void enviastring_usart2(char *str);
char recebechar_usart2(void);

#endif
