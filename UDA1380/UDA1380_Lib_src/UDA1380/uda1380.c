//Data: 01/10/2014

#include <stdio.h>
#include <math.h>
#include "uda1380.h"
#include "semihosting.h"
#include "cpal_i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#ifdef STACK_CHECK
// stacks[0] = UDA1380_button_task
// stacks[1] = UDA1380_error_task
// stacks[2] = UDA1380_timeout
// stacks[3] = heap
volatile int stacks[4] = { 0, 0, 0, 0 };
#endif

//Habilita/desabilita o botao para conectar a entrada na saida sem processamento
#define BUTTON_PASSTHROUGH	//comente esta linha para desabilitar este recurso

#define UNROLL_FACTOR 4
#define STR_DEFINE2(x) #x
#define STR_DEFINE(x) STR_DEFINE2(x)

#if UNROLL_FACTOR != 1 && UNROLL_FACTOR != 2 && UNROLL_FACTOR != 4 && UNROLL_FACTOR != 8
#error UNROLL_FACTOR deve ser um dos seguintes valores: 1, 2, 4 ou 8
#endif

#ifdef PERFORMANCE_TEST
// TEST_DMA1_STREAM0 - PA1
// TEST_DMA1_STREAM4 - PA2
// TEST_UDA1380_SENDSAMPLES - PA3
// TEST_UDA1380_SENDSAMPLESFLOAT - PA3
// TEST_UDA1380_RECEIVESAMPLES - PC4
// TEST_UDA1380_RECEIVESAMPLESFLOAT - PC4
// TEST_ARM_Q15_TO_FLOAT - PC5
// TEST_ARM_FLOAT_TO_Q15 - PB1

#define TEST_DMA1_STREAM0
#define TEST_DMA1_STREAM4
#define TEST_UDA1380_SENDSAMPLES
#define TEST_UDA1380_SENDSAMPLESFLOAT
#define TEST_UDA1380_RECEIVESAMPLES
#define TEST_UDA1380_RECEIVESAMPLESFLOAT
#define TEST_ARM_Q15_TO_FLOAT
#define TEST_ARM_FLOAT_TO_Q15
#endif

#define UDA1380_I2C_ADDRESS 0x30
#define I2C_CLKSPEED 400000
#define I2C_DEVSTRUCTURE I2C1_DevStructure

#define UDA1380_assert_param(expr,erro) ((expr) ? (void)0 : UDA1380_Erro(erro))


#define IS_UDA1380_ADC_ENABLE(ADC_ENABLE) (((ADC_ENABLE) == ENABLE ) || \
                                           ((ADC_ENABLE) == DISABLE))

#define IS_UDA1380_DAC_ENABLE(DAC_ENABLE) (((DAC_ENABLE) == ENABLE ) || \
                                           ((DAC_ENABLE) == DISABLE))

#define IS_UDA1380_RT_ERRORS(RT_ERRORS)  (((RT_ERRORS) == ENABLE ) || \
                                          ((RT_ERRORS) == DISABLE))

#define IS_UDA1380_INPUT(INP)             (((INP) == UDA1380_Input_Microphone) || \
		                                   ((INP) == UDA1380_Input_LineIn))

#define IS_UDA1380_SAMPLE_RATE(INP)       (((INP) == UDA1380_ADC_48000_DAC_48000) || \
		                                   ((INP) == UDA1380_ADC_48000_DAC_32000) || \
                                           ((INP) == UDA1380_ADC_48000_DAC_16000) || \
                                           ((INP) == UDA1380_ADC_48000_DAC_8000)  || \
                                                                                     \
                                           ((INP) == UDA1380_ADC_32000_DAC_48000) || \
                                           ((INP) == UDA1380_ADC_32000_DAC_32000) || \
                                           ((INP) == UDA1380_ADC_32000_DAC_16000) || \
                                           ((INP) == UDA1380_ADC_32000_DAC_8000)  || \
                                                                                     \
                                           ((INP) == UDA1380_ADC_16000_DAC_48000) || \
                                           ((INP) == UDA1380_ADC_16000_DAC_32000) || \
                                           ((INP) == UDA1380_ADC_16000_DAC_16000) || \
                                           ((INP) == UDA1380_ADC_16000_DAC_8000)  || \
                                                                                     \
                                           ((INP) == UDA1380_ADC_8000_DAC_48000)  || \
                                           ((INP) == UDA1380_ADC_8000_DAC_32000)  || \
                                           ((INP) == UDA1380_ADC_8000_DAC_16000)  || \
                                           ((INP) == UDA1380_ADC_8000_DAC_8000)   || \
                                                                                     \
                                           ((INP) == UDA1380_ADC_44100_DAC_44100) || \
                                           ((INP) == UDA1380_ADC_44100_DAC_22050) || \
                                           ((INP) == UDA1380_ADC_44100_DAC_11025) || \
                                                                                     \
                                           ((INP) == UDA1380_ADC_22050_DAC_44100) || \
                                           ((INP) == UDA1380_ADC_22050_DAC_22050) || \
                                           ((INP) == UDA1380_ADC_22050_DAC_11025) || \
                                                                                     \
                                           ((INP) == UDA1380_ADC_11025_DAC_44100) || \
                                           ((INP) == UDA1380_ADC_11025_DAC_22050) || \
                                           ((INP) == UDA1380_ADC_11025_DAC_11025))

#define UDA1380_REG00_MSB_DEFAULT 	0x05
#define UDA1380_REG00_LSB_DEFAULT 	0x10	//0x10 is the mask for WSPLL in DAC

#define UDA1380_REG00_EN_ADC 		0x08
#define UDA1380_REG00_EN_DAC 		0x02

#define UDA1380_REG00_PLL_00		0x00	//6.25 to 12.5
#define UDA1380_REG00_PLL_01		0x01	//12.5 to 25
#define UDA1380_REG00_PLL_10		0x02	//25 to 50
#define UDA1380_REG00_PLL_11		0x03	//50 to 100

#define UDA1380_REG01_MSB_DEFAULT 	0x00
#define UDA1380_REG01_LSB_DEFAULT 	0x00

#define UDA1380_REG02_MSB_DEFAULT 	0xA5	//0x80 is the mask for PON_PLL
#define UDA1380_REG02_LSB_DEFAULT 	0xDF

#define UDA1380_REG10_MSB_DEFAULT 	0x00
#define UDA1380_REG10_LSB_DEFAULT 	0x00

#define UDA1380_REG11_MSB_DEFAULT 	0x00
#define UDA1380_REG11_LSB_DEFAULT 	0xFF

#define UDA1380_REG13_MSB_DEFAULT 	0x00
#define UDA1380_REG13_LSB_DEFAULT 	0x00

#define UDA1380_REG14_MSB_DEFAULT 	0x00
#define UDA1380_REG14_LSB_DEFAULT 	0x00

#define UDA1380_REG21_MSB_DEFAULT 	0x00
#define UDA1380_REG21_LSB_DEFAULT 	0x00

#define UDA1380_REG22_MSB_DEFAULT 	0x00
#define UDA1380_REG22_LSB_DEFAULT 	0x02

#define UDA1380_REG22_SEL_LNA 		0x08
#define UDA1380_REG22_SEL_MIC 		0x04

// variaveis globais
static int16_t *vet_in1, *vet_in2, *vet_out1, *vet_out2;
static volatile int pos_send=0,pos_receive=0,pos_DMA_in=0,pos_DMA_out=0;
static volatile int vet_in_DMA=1,vet_in_func=-1,vet_out_DMA=-1,vet_out_func=1;
static int queue_length;

static const int matriz_sample_rate[][2] =
{
	{ 48000, 48000 }, 	//UDA1380_ADC_48000_DAC_48000,
	{ 48000, 32000 }, 	//UDA1380_ADC_48000_DAC_32000,
	{ 48000, 16000 }, 	//UDA1380_ADC_48000_DAC_16000,
	{ 48000, 8000 }, 	//UDA1380_ADC_48000_DAC_8000,

	{ 32000, 48000 }, 	//UDA1380_ADC_32000_DAC_48000,
	{ 32000, 32000 },	//UDA1380_ADC_32000_DAC_32000,
	{ 32000, 16000 }, 	//UDA1380_ADC_32000_DAC_16000,
	{ 32000, 8000 }, 	//UDA1380_ADC_32000_DAC_8000,

	{ 16000, 48000 }, 	//UDA1380_ADC_16000_DAC_48000,
	{ 16000, 32000 }, 	//UDA1380_ADC_16000_DAC_32000,
	{ 16000, 16000 }, 	//UDA1380_ADC_16000_DAC_16000,
	{ 16000, 8000 }, 	//UDA1380_ADC_16000_DAC_8000,

	{ 8000, 48000 }, 	//UDA1380_ADC_8000_DAC_48000,
	{ 8000, 32000 }, 	//UDA1380_ADC_8000_DAC_32000,
	{ 8000, 16000 }, 	//UDA1380_ADC_8000_DAC_16000,
	{ 8000, 8000 }, 	//UDA1380_ADC_8000_DAC_8000,

	{ 44100, 44100 }, 	//UDA1380_ADC_44100_DAC_44100,
	{ 44100, 22050 }, 	//UDA1380_ADC_44100_DAC_22050,
	{ 44100, 11025 }, 	//UDA1380_ADC_44100_DAC_11025,

	{ 22050, 44100 }, 	//UDA1380_ADC_22050_DAC_44100,
	{ 22050, 22050 }, 	//UDA1380_ADC_22050_DAC_22050,
	{ 22050, 11025 }, 	//UDA1380_ADC_22050_DAC_11025,

	{ 11025, 44100 }, 	//UDA1380_ADC_11025_DAC_44100,
	{ 11025, 22050 }, 	//UDA1380_ADC_11025_DAC_22050,
	{ 11025, 11025 }, 	//UDA1380_ADC_11025_DAC_11025,
};

static const int matriz_pll[][2] =
{
	{ 344, 2 }, 	//UDA1380_ADC_48000_DAC_48000,
	{ 344, 2 }, 	//UDA1380_ADC_48000_DAC_32000,
	{ 344, 2 }, 	//UDA1380_ADC_48000_DAC_16000,
	{ 344, 4 }, 	//UDA1380_ADC_48000_DAC_8000,

	{ 344, 2 }, 	//UDA1380_ADC_32000_DAC_48000,
	{ 344, 2 },		//UDA1380_ADC_32000_DAC_32000,
	{ 344, 2 }, 	//UDA1380_ADC_32000_DAC_16000,
	{ 213, 2 }, 	//UDA1380_ADC_32000_DAC_8000,

	{ 344, 2 }, 	//UDA1380_ADC_16000_DAC_48000,
	{ 344, 2 }, 	//UDA1380_ADC_16000_DAC_32000,
	{ 344, 2 }, 	//UDA1380_ADC_16000_DAC_16000,
	{ 344, 4 }, 	//UDA1380_ADC_16000_DAC_8000,

	{ 344, 4 }, 	//UDA1380_ADC_8000_DAC_48000,
	{ 213, 2 }, 	//UDA1380_ADC_8000_DAC_32000,
	{ 344, 4 }, 	//UDA1380_ADC_8000_DAC_16000,
	{ 344, 4 }, 	//UDA1380_ADC_8000_DAC_8000,

	{ 271, 2 }, 	//UDA1380_ADC_44100_DAC_44100,
	{ 271, 2 }, 	//UDA1380_ADC_44100_DAC_22050,
	{ 271, 2 }, 	//UDA1380_ADC_44100_DAC_11025,

	{ 271, 2 }, 	//UDA1380_ADC_22050_DAC_44100,
	{ 271, 2 }, 	//UDA1380_ADC_22050_DAC_22050,
	{ 271, 2 }, 	//UDA1380_ADC_22050_DAC_11025,

	{ 271, 2 }, 	//UDA1380_ADC_11025_DAC_44100,
	{ 271, 2 }, 	//UDA1380_ADC_11025_DAC_22050,
	{ 271, 2 }, 	//UDA1380_ADC_11025_DAC_11025,
};

static FunctionalState ADC_StateFlag, DAC_StateFlag;

#ifdef BUTTON_PASSTHROUGH
static SemaphoreHandle_t sem_button;
#endif

static QueueHandle_t queue_erro;
static CPAL_TransferTypeDef CPALTx_Structure, CPALRx_Structure;
TaskHandle_t CPAL_timeout_task_handle;
int CPAL_timeout_init = 0;
static UDA1380_InitTypeDef UDA1380_InitStructure_global;

// prototipos
static void Delay(volatile int d);
static void Inicializacao_I2C();
static void I2C_escrever_reg(int, int, uint8_t*, int);
void FreeRTOS_Init(void);
void CPAL_timeout_task(void*);
static void UDA1380_error_task(void*);
static void UDA1380_Erro(const char*);
static void UDA1380_ErroFromISR(const char*);
static void UDA1380_task(void*);

#ifdef BUTTON_PASSTHROUGH
static void UDA1380_button_task(void*);
#endif

void erro_catastrofico(const char* str)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
		SH_SendString(str);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	while(1)
	{
		GPIO_SetBits(GPIOD,GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
		Delay(5000000);
		GPIO_ResetBits(GPIOD,GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
		Delay(5000000);
	}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
	erro_catastrofico("Ocorreu um estouro de pilha, causa provavel: excesso de variaveis locais na funcao de processamento de sinais, substitua algumas (especialmente arrays) por variaveis globais\n");
}

void vApplicationMallocFailedHook()
{
	erro_catastrofico("Aconteceu um erro grave, chame o professor: Erro 1\n");
}

static void Delay(volatile int d)
{
	while(d-- > 0);
}

static void Inicializacao_I2C()
{
	CPAL_I2C_DeInit(&I2C_DEVSTRUCTURE);

	CPAL_I2C_StructInit(&I2C_DEVSTRUCTURE);
	I2C_DEVSTRUCTURE.CPAL_ProgModel = CPAL_PROGMODEL_INTERRUPT;
	I2C_DEVSTRUCTURE.pCPAL_TransferTx = &CPALTx_Structure;
	I2C_DEVSTRUCTURE.pCPAL_TransferRx = &CPALRx_Structure;
	I2C_DEVSTRUCTURE.pCPAL_I2C_Struct->I2C_ClockSpeed = I2C_CLKSPEED;
	CPAL_I2C_Init(&I2C_DEVSTRUCTURE);
}

static void I2C_escrever_reg(int endereco, int registrador, uint8_t* dado, int tam_dado)
{
I2C_escrever_reg_inicio:
	I2C_DEVSTRUCTURE.pCPAL_TransferTx->pbBuffer = dado;
	I2C_DEVSTRUCTURE.pCPAL_TransferTx->wAddr1 = endereco;
	I2C_DEVSTRUCTURE.pCPAL_TransferTx->wAddr2 = registrador;
	I2C_DEVSTRUCTURE.pCPAL_TransferTx->wNumData = tam_dado;

	if(CPAL_I2C_Write(&I2C_DEVSTRUCTURE) != CPAL_PASS)
	{
		//erro
		while(1);
	}

	while (I2C_DEVSTRUCTURE.CPAL_State != CPAL_STATE_READY)
	{
		if (I2C_DEVSTRUCTURE.CPAL_State == CPAL_STATE_ERROR)
		{
			Inicializacao_I2C();

			goto I2C_escrever_reg_inicio;
		}
	}
}

static int buf_DMA;

static void I2S_DMA_Rx_Config(UDA1380_InitTypeDef* UDA1380_InitStruct)
{
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Configure the DMA1 Stream 0 - SPI3_Rx*/
	DMA_Cmd(DMA1_Stream0, DISABLE);
	DMA_DeInit(DMA1_Stream0);
	/* Set the parameters to be configured */
	DMA_StructInit(&DMA_InitStructure);
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(SPI3->DR);
	if (UDA1380_InitStruct->UDA1380_ADC_Enable)
	{
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)vet_in1;
		DMA_InitStructure.DMA_BufferSize = UDA1380_InitStruct->UDA1380_Queue_Length;
	}
	else
	{
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&buf_DMA;
		DMA_InitStructure.DMA_BufferSize = 2;
	}
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_Init(DMA1_Stream0, &DMA_InitStructure);

	if (UDA1380_InitStruct->UDA1380_ADC_Enable)
	{
		/* I2S DMA1 IRQ Channel configuration */
		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream0_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 14;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_Init(&NVIC_InitStructure);
	}
}

void FreeRTOS_Init(void)
{
#ifdef BUTTON_PASSTHROUGH
	sem_button = xSemaphoreCreateBinary();
#endif

	queue_erro = xQueueCreate( 1, sizeof( char* ) );
	// tratamento de erro

	xTaskCreate(UDA1380_error_task, "Err", 64, NULL, configMAX_PRIORITIES - 1, NULL);

#ifdef BUTTON_PASSTHROUGH
	xTaskCreate(UDA1380_button_task, "But", 64, NULL, 1, NULL);
#endif
}

void CPAL_timeout_task(void* pvParameters)
{
	while(1)
	{
		CPAL_I2C_TIMEOUT_Manager();

#ifdef STACK_CHECK
		stacks[2] = uxTaskGetStackHighWaterMark(NULL);
		stacks[3] = xPortGetFreeHeapSize();
#endif

		vTaskDelay(1);
	}
}

static void UDA1380_error_task(void* pvParameters)
{
	const char* erro;

	GPIO_InitTypeDef GPIO_InitStructure;

	xQueueReceive(queue_erro, &erro, portMAX_DELAY);

#ifdef STACK_CHECK
	stacks[1] = uxTaskGetStackHighWaterMark(NULL);
#endif

	if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
		SH_SendString(erro);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	while(1)
	{
		GPIO_SetBits(GPIOD,GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
		Delay(5000000);
		GPIO_ResetBits(GPIOD,GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
		Delay(5000000);
	}
}

static void UDA1380_Erro(const char* erro)
{
	xQueueSendToBack(queue_erro, &erro, portMAX_DELAY);
	while(1);
}

static void UDA1380_ErroFromISR(const char* erro)
{
	xQueueSendToBackFromISR(queue_erro, &erro, NULL);
}

static void UDA1380_task(void* pvParameters)
{
	uint8_t dado[2];

	UDA1380_InitTypeDef* UDA1380_InitStruct = pvParameters;

	DMA_InitTypeDef DMA_InitStructure;
	I2S_InitTypeDef I2S_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

#ifdef BUTTON_PASSTHROUGH
	EXTI_InitTypeDef EXTI_InitStructure;
#endif

	RCC_ClearFlag();

	ADC_StateFlag = UDA1380_InitStruct->UDA1380_ADC_Enable;
	DAC_StateFlag = UDA1380_InitStruct->UDA1380_DAC_Enable;

	UDA1380_assert_param(IS_UDA1380_ADC_ENABLE(UDA1380_InitStruct->UDA1380_ADC_Enable),"Voce preencheu errado o campo UDA1380_ADC_Enable na struct UDA1380_InitStructure, valores validos: ENABLE ou DISABLE\n");
	UDA1380_assert_param(IS_UDA1380_DAC_ENABLE(UDA1380_InitStruct->UDA1380_DAC_Enable),"Voce preencheu errado o campo UDA1380_DAC_Enable na struct UDA1380_InitStructure, valores validos: ENABLE ou DISABLE\n");
	UDA1380_assert_param(IS_UDA1380_RT_ERRORS(UDA1380_InitStruct->UDA1380_Real_Time_Errors),"Voce preencheu errado o campo UDA1380_Real_Time_Errors na struct UDA1380_InitStructure, valores validos: ENABLE ou DISABLE\n");
	UDA1380_assert_param(IS_UDA1380_INPUT(UDA1380_InitStruct->UDA1380_Input),"Voce preencheu errado o campo UDA1380_Input na struct UDA1380_InitStructure, valores validos: UDA1380_Input_Microphone ou UDA1380_Input_LineIn\n");
	UDA1380_assert_param(IS_UDA1380_SAMPLE_RATE(UDA1380_InitStruct->UDA1380_Sample_Rate),"Voce preencheu errado o campo UDA1380_Sample_Rate na struct UDA1380_InitStructure, valores validos: verificar no uda1380.h\n");
	UDA1380_assert_param(UDA1380_InitStruct->UDA1380_Volume_Att <= 0.0 && UDA1380_InitStruct->UDA1380_Volume_Att >= -100.0,"Voce preencheu errado o campo UDA1380_Volume_Att na struct UDA1380_InitStructure, valores validos: entre 0.0 e -100.0");
	UDA1380_assert_param(UDA1380_InitStruct->UDA1380_Callback != NULL,"Voce esqueceu de atribuir um callback na struct UDA1380_InitStructure\n");
	UDA1380_assert_param(UDA1380_InitStruct->UDA1380_Queue_Length > 0, "Preencha corretamente o campo UDA1380_Queue_Length na struct UDA1380_InitStructure, sugestao de valor: 1024\n");
	UDA1380_assert_param((UDA1380_InitStruct->UDA1380_Queue_Length % (2*UNROLL_FACTOR)) == 0, "O campo UDA1380_Queue_Length na struct UDA1380_InitStructure deve ser multiplo de " STR_DEFINE(2*UNROLL_FACTOR) "\n");

	if(ADC_StateFlag)
		UDA1380_assert_param(UDA1380_InitStruct->UDA1380_Buffer_In,"Voce esqueceu de passar um ponteiro para um vetor de int16_t de UDA1380_Queue_Length elementos no campo UDA1380_Buffer_In da struct UDA1380_InitStructure\n");
	if(DAC_StateFlag)
		UDA1380_assert_param(UDA1380_InitStruct->UDA1380_Buffer_Out,"Voce esqueceu de passar um ponteiro para um vetor de int16_t de UDA1380_Queue_Length elementos no campo UDA1380_Buffer_Out da struct UDA1380_InitStructure\n");


	vet_in1 = UDA1380_InitStruct->UDA1380_Buffer_In;
	vet_out1 = UDA1380_InitStruct->UDA1380_Buffer_Out;

	queue_length = UDA1380_InitStruct->UDA1380_Queue_Length;

	vet_in2 = &vet_in1[queue_length/2];
	vet_out2 = &vet_out1[queue_length/2];

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	//GPIOs para PERFORMANCE_TEST e CPAL_TEST
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOB, GPIO_Pin_1);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOC, GPIO_Pin_4 | GPIO_Pin_5);

	//Configuração do Clock da I2S
	RCC_I2SCLKConfig(RCC_I2S2CLKSource_PLLI2S);
	RCC_PLLI2SConfig(matriz_pll[(int) UDA1380_InitStruct->UDA1380_Sample_Rate][0], matriz_pll[(int) UDA1380_InitStruct->UDA1380_Sample_Rate][1]);
	RCC_PLLI2SCmd(ENABLE);

	//GPIO - SPI3 - receive data
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_SPI3);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI3);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_SPI3);

	if(DAC_StateFlag == ENABLE)
	{
		//GPIO - SPI2 - send data
		GPIO_StructInit(&GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_Init(GPIOC, &GPIO_InitStructure);
		GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI2);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
		GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_SPI2);
		GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_SPI2);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
		GPIO_Init(GPIOC, &GPIO_InitStructure);
		GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_SPI2);
	}

	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	Inicializacao_I2C();

#ifdef BUTTON_PASSTHROUGH
	if(ADC_StateFlag == ENABLE && DAC_StateFlag == ENABLE)
	{
		/* Enable SYSCFG clock */
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

		// Push-button
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		/* Connect EXTI Line0 to PA0 pin */
		SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);

		/* Configure EXTI Line0 */
		EXTI_InitStructure.EXTI_Line = EXTI_Line0;
		EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
		EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
		EXTI_Init(&EXTI_InitStructure);

		/* Enable and set EXTI Line0 Interrupt to the lowest priority */
		NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}
#endif

	/* Enable the DMA clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

	I2S_DMA_Rx_Config(UDA1380_InitStruct);

	DMA_Cmd(DMA1_Stream0,ENABLE);

	if(DAC_StateFlag == ENABLE)
	{
		/* Configure the DMA1 Stream 4 - SPI2_Tx*/
		DMA_Cmd(DMA1_Stream4, DISABLE);
		DMA_DeInit(DMA1_Stream4);
		/* Set the parameters to be configured */
		DMA_StructInit(&DMA_InitStructure);
		DMA_InitStructure.DMA_Channel = DMA_Channel_0;
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(SPI2->DR);
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)vet_out1;
		DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
		DMA_InitStructure.DMA_BufferSize = UDA1380_InitStruct->UDA1380_Queue_Length;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
		DMA_InitStructure.DMA_Priority = DMA_Priority_High;
		DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
		DMA_Init(DMA1_Stream4, &DMA_InitStructure);

		/* Enable DMA1 interrupts */
		DMA_ITConfig(DMA1_Stream4, DMA_IT_TC | DMA_IT_HT, ENABLE);

		DMA_Cmd(DMA1_Stream4,DISABLE);

		/* I2S DMA1 IRQ Channel configuration */
		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream4_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 14;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_Init(&NVIC_InitStructure);
	}

	I2S_StructInit(&I2S_InitStructure);
	I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;
	I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
	I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;

	/* Enable the CODEC_I2S3 peripheral clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

	/* CODEC_I2S3 peripheral configuration */
	SPI_I2S_DeInit(SPI3);

	I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
	I2S_InitStructure.I2S_AudioFreq = matriz_sample_rate[(int) UDA1380_InitStruct->UDA1380_Sample_Rate][0];
	I2S_InitStructure.I2S_Mode = I2S_Mode_MasterRx;

	/* Initialize the I2S3 peripheral with the structure above */
	I2S_Init(SPI3, &I2S_InitStructure);

	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Rx, ENABLE);

	if(DAC_StateFlag == ENABLE)
	{
		/* Enable the CODEC_I2S2 peripheral clock */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

		/* CODEC_I2S2 peripheral configuration */
		SPI_I2S_DeInit(SPI2);

		I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
		I2S_InitStructure.I2S_AudioFreq = matriz_sample_rate[(int) UDA1380_InitStruct->UDA1380_Sample_Rate][1];
		I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;

		/* Initialize the I2S2 peripheral with the structure above */
		I2S_Init(SPI2, &I2S_InitStructure);

		SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
	}

	// ATENCAO: e' porco, mas a I2S precisa ser ligada antes de iniciar as
	// configuracoes via I2C, pois de outra forma o UDA1380 nao grava as
	// configuracoes realizadas (finge que foi tudo bem, mas nao grava)
	I2S_Cmd(SPI3, ENABLE);

	if(DAC_StateFlag == ENABLE)
		I2S_Cmd(SPI2, ENABLE);

	vTaskDelay(1);

	I2C_DEVSTRUCTURE.pCPAL_TransferTx->wAddr1 = UDA1380_I2C_ADDRESS;
	while(CPAL_I2C_IsDeviceReady(&I2C_DEVSTRUCTURE) == CPAL_FAIL)
		;


	// Registrador 0x01: nao mexe

	// Inicializa registrador 0x02
	dado[0] = UDA1380_REG02_MSB_DEFAULT;
	dado[1] = UDA1380_REG02_LSB_DEFAULT;

	I2C_escrever_reg(UDA1380_I2C_ADDRESS, 0x02, dado, 2);

	// Registrador 0x03: nao mexe

	// Registrador 0x04: nao mexe

	// Inicializa registrador 0x10
	if(UDA1380_InitStruct->UDA1380_Volume_Att > -52.0)
		dado[0] = dado[1] = roundf((-UDA1380_InitStruct->UDA1380_Volume_Att)*4.0);
	else
		dado[0] = dado[1] = 0xFC;

	I2C_escrever_reg(UDA1380_I2C_ADDRESS, 0x10, dado, 2);

	// Inicializa registrador 0x11
	dado[0] = UDA1380_REG11_MSB_DEFAULT;
	dado[1] = UDA1380_REG11_LSB_DEFAULT;

	I2C_escrever_reg(UDA1380_I2C_ADDRESS, 0x11, dado, 2);

	// Registrador 0x12: nao mexe

	// Inicializa registrador 0x13
	dado[0] = UDA1380_REG13_MSB_DEFAULT;
	dado[1] = UDA1380_REG13_LSB_DEFAULT;

	I2C_escrever_reg(UDA1380_I2C_ADDRESS, 0x13, dado, 2);

	// Registrador 0x14:
	dado[0] = UDA1380_REG14_MSB_DEFAULT;
	dado[1] = UDA1380_REG14_LSB_DEFAULT;

	I2C_escrever_reg(UDA1380_I2C_ADDRESS, 0x14, dado, 2);

	// Registrador 0x20: nao mexe (volume do decimador)

	// Inicializa registrador 0x21
	dado[0] = UDA1380_REG21_MSB_DEFAULT;
	dado[1] = UDA1380_REG21_LSB_DEFAULT;

	I2C_escrever_reg(UDA1380_I2C_ADDRESS, 0x21, dado, 2);

	// Inicializa registrador 0x22
	dado[0] = UDA1380_REG22_MSB_DEFAULT;
	dado[1] = UDA1380_REG22_LSB_DEFAULT;

	if (UDA1380_InitStruct->UDA1380_Input == UDA1380_Input_Microphone)
		dado[1] |= UDA1380_REG22_SEL_LNA | UDA1380_REG22_SEL_MIC;

	I2C_escrever_reg(UDA1380_I2C_ADDRESS, 0x22, dado, 2);

	// Registrador 0x23: nao mexe

	// Inicializa registrador 0x00
	dado[0] = UDA1380_REG00_MSB_DEFAULT;
	dado[1] = UDA1380_REG00_LSB_DEFAULT;

	if (UDA1380_InitStruct->UDA1380_ADC_Enable == ENABLE)
		dado[0] |= UDA1380_REG00_EN_ADC;

	if (UDA1380_InitStruct->UDA1380_DAC_Enable == ENABLE)
		dado[0] |= UDA1380_REG00_EN_DAC;

	if (matriz_sample_rate[(int) UDA1380_InitStruct->UDA1380_Sample_Rate][1] < 12500)
		dado[1] |= UDA1380_REG00_PLL_00;
	else if (matriz_sample_rate[(int) UDA1380_InitStruct->UDA1380_Sample_Rate][1] < 25000)
		dado[1] |= UDA1380_REG00_PLL_01;
	else if (matriz_sample_rate[(int) UDA1380_InitStruct->UDA1380_Sample_Rate][1] < 50000)
		dado[1] |= UDA1380_REG00_PLL_10;
	else
		dado[1] |= UDA1380_REG00_PLL_11;

	I2C_escrever_reg(UDA1380_I2C_ADDRESS, 0x00, dado, 2);

	if (ADC_StateFlag == ENABLE)
	{
		I2S_DMA_Rx_Config(UDA1380_InitStruct);

		/* Enable DMA1 interrupts */
		DMA_ITConfig(DMA1_Stream0, DMA_IT_TC | DMA_IT_HT, ENABLE);

		DMA_Cmd(DMA1_Stream0,ENABLE);
	}

	UDA1380_InitStruct->UDA1380_Callback();
	UDA1380_Erro("Voce nunca pode retornar do callback\n");
}

#ifdef BUTTON_PASSTHROUGH
static void UDA1380_button_task(void* pvParameters)
{
	uint8_t dado[2];
	while(1)
	{
		xSemaphoreTake(sem_button,portMAX_DELAY);

		if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0))
		{
			// Modifica registrador 0x14
			dado[0] = 0x10;
			dado[1] = 0x00;
		}
		else
		{
			// Modifica registrador 0x14
			dado[0] = UDA1380_REG14_MSB_DEFAULT;
			dado[1] = UDA1380_REG14_LSB_DEFAULT;

		}
		I2C_escrever_reg(UDA1380_I2C_ADDRESS, 0x14, dado, 2);

#ifdef STACK_CHECK
		stacks[0] = uxTaskGetStackHighWaterMark(NULL);
#endif

		vTaskDelay(30);
	}
}
#endif

#ifdef BUTTON_PASSTHROUGH
void EXTI0_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line0) != RESET)
	{
		xSemaphoreGiveFromISR(sem_button, NULL);

		/* Clear the EXTI line 0 pending bit */
		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}
#endif

void DMA1_Stream0_IRQHandler(void)
{

#ifdef TEST_DMA1_STREAM0
	GPIOA->BSRRL = 2;
#endif

	DMA_ClearITPendingBit(DMA1_Stream0,DMA_IT_FEIF0);

	int pos_receive_real = pos_receive;

	if (UDA1380_InitStructure_global.UDA1380_Real_Time_Errors == ENABLE)
	{
		pos_DMA_in = queue_length - DMA_GetCurrDataCounter(DMA1_Stream0);

		if(vet_in_func == 2)
			pos_receive_real += queue_length/2;;

		if((pos_DMA_in + queue_length - pos_receive_real) % (queue_length) >= 7*queue_length/8)
		{
			UDA1380_ErroFromISR("Seu programa estourou o tempo limite para chamar UDA1380_ReceiveSamples(); Verifique a logica do programa ou aumente o valor do campo UDA1380_Queue_Length na struct UDA1380_InitStructure\n");
		}
	}

	/* Test on DMA Stream Half Transfer interrupt */
	if(DMA_GetITStatus(DMA1_Stream0,DMA_IT_HTIF0))
	{
		/* Clear DMA Stream Half Transfer interrupt pending bit */
		DMA_ClearITPendingBit(DMA1_Stream0,DMA_IT_HTIF0);

		//DO Something
		if(vet_in_func == -1)
			vet_in_func = vet_in_DMA;

		vet_in_DMA=2;
	}

	/* Test on DMA Stream Transfer Complete interrupt */
	if(DMA_GetITStatus(DMA1_Stream0,DMA_IT_TCIF0))
	{
		/* Clear DMA Stream Half Transfer interrupt pending bit */
		DMA_ClearITPendingBit(DMA1_Stream0,DMA_IT_TCIF0);

		//DO Something
		vet_in_DMA=1;
	}

#ifdef TEST_DMA1_STREAM0
	GPIOA->BSRRH = 2;
#endif

}

void DMA1_Stream4_IRQHandler(void)
{

#ifdef TEST_DMA1_STREAM4
	GPIOA->BSRRL = 4;
#endif

	int pos_send_real = pos_send;

	if (UDA1380_InitStructure_global.UDA1380_Real_Time_Errors == ENABLE)
	{
		pos_DMA_out = queue_length - DMA_GetCurrDataCounter(DMA1_Stream4);

		if(vet_out_func == 2)
			pos_send_real += queue_length/2;

		if((pos_send_real + queue_length - pos_DMA_out) % queue_length <= queue_length/8)
			UDA1380_ErroFromISR("Seu programa estourou o tempo limite para chamar UDA1380_SendSamples(); Verifique a logica do programa ou aumente o valor do campo UDA1380_Queue_Length na struct UDA1380_InitStructure\n");
	}

	/* Test on DMA Stream Half Transfer interrupt */
	if(DMA_GetITStatus(DMA1_Stream4,DMA_IT_HTIF4))
	{
		/* Clear DMA Stream Half Transfer interrupt pending bit */
		DMA_ClearITPendingBit(DMA1_Stream4,DMA_IT_HTIF4);

		//DO Something
		vet_out_DMA=2;
	}

	/* Test on DMA Stream Transfer Complete interrupt */
	if(DMA_GetITStatus(DMA1_Stream4,DMA_IT_TCIF4))
	{
		/* Clear DMA Stream Half Transfer interrupt pending bit */
		DMA_ClearITPendingBit(DMA1_Stream4,DMA_IT_TCIF4);

		//DO Something
		vet_out_DMA=1;
	}

#ifdef TEST_DMA1_STREAM4
	GPIOA->BSRRH = 4;
#endif

}

void UDA1380_StructInit(UDA1380_InitTypeDef* UDA1380_InitStruct)
{
	UDA1380_InitStruct->UDA1380_ADC_Enable = ENABLE;
	UDA1380_InitStruct->UDA1380_DAC_Enable = ENABLE;
	UDA1380_InitStruct->UDA1380_Real_Time_Errors = ENABLE;
	UDA1380_InitStruct->UDA1380_Input = UDA1380_Input_LineIn;
	UDA1380_InitStruct->UDA1380_Sample_Rate = UDA1380_ADC_48000_DAC_48000;
	UDA1380_InitStruct->UDA1380_Volume_Att = -10.0;
	UDA1380_InitStruct->UDA1380_Buffer_In = NULL;
	UDA1380_InitStruct->UDA1380_Buffer_Out = NULL;
	UDA1380_InitStruct->UDA1380_Queue_Length = -1;
	UDA1380_InitStruct->UDA1380_Callback = NULL;
}

void UDA1380_Init(UDA1380_InitTypeDef* UDA1380_InitStruct)
{
	UDA1380_InitStructure_global = *UDA1380_InitStruct;

	FreeRTOS_Init();

	xTaskCreate(UDA1380_task, "UDA", 256, &UDA1380_InitStructure_global, 1, NULL);

	vTaskStartScheduler();

	UDA1380_Erro("Aconteceu um erro grave, chame o professor: Erro 0\n");
}

inline int16_t float_to_fixed_point(float flo)
{
	int16_t res;
	asm("vcvt.s16.f32 %[value], %[value], #15\n"
	    "vmov.f32 %[result], %[value]\n"
	    "ssat %[result], #16, %[result]"
	    : [result] "=r" (res)
	    : [value] "w" (flo));
	return res;
}


inline float fixed_point_to_float(int16_t fix)
{
	float res;
	asm("vmov.f32 %[result], %[value]\n"
	    "vcvt.f32.s16 %[result], %[result], #15"
	    : [result] "=w" (res)
	    : [value] "r" (fix));
	return res;
}

void UDA1380_ReceiveSamples(int16_t samples_L[], int16_t samples_R[], int num_samples)
{
	int i;
	int16_t* vet;
	int index;

#ifdef TEST_UDA1380_RECEIVESAMPLES
	GPIOC->BSRRL = 16;
#endif

	UDA1380_assert_param(ADC_StateFlag == ENABLE, "Voce tentou receber amostras sem habilitar o UDA1380_ADC_Enable na UDA1380_InitStructure\n");
	UDA1380_assert_param((num_samples % UNROLL_FACTOR) == 0, "Nas funcoes para enviar e receber amostras, num_samples deve ser multiplo de " STR_DEFINE(UNROLL_FACTOR) "\n");
	UDA1380_assert_param(num_samples <= queue_length/4, "Voce tentou receber uma quantidade muito grande de amostras de uma vez; reduza numero de amostras ou aumente o valor do campo UDA1380_Queue_Length na struct UDA1380_InitStructure\n");

	while(vet_in_func == -1);

	if (vet_in_func == 1)
		vet = &vet_in1[pos_receive];
	else
		vet = &vet_in2[pos_receive];

	index = (queue_length/2 - pos_receive)/2;

	if(index >= num_samples)
		index = num_samples;

	for(i = 0; i < index; i += UNROLL_FACTOR)
	{
#if UNROLL_FACTOR >= 1
		samples_L[i] = *vet++;
		samples_R[i] = *vet++;
#if UNROLL_FACTOR >= 2
		samples_L[i+1] = *vet++;
		samples_R[i+1] = *vet++;
#if UNROLL_FACTOR >= 4
		samples_L[i+2] = *vet++;
		samples_R[i+2] = *vet++;

		samples_L[i+3] = *vet++;
		samples_R[i+3] = *vet++;
#if UNROLL_FACTOR >= 8
		samples_L[i+4] = *vet++;
		samples_R[i+4] = *vet++;

		samples_L[i+5] = *vet++;
		samples_R[i+5] = *vet++;

		samples_L[i+6] = *vet++;
		samples_R[i+6] = *vet++;

		samples_L[i+7] = *vet++;
		samples_R[i+7] = *vet++;
#endif
#endif
#endif
#endif
		pos_receive += 2*UNROLL_FACTOR;
	}

	if(index < num_samples)
	{

#ifdef TEST_UDA1380_RECEIVESAMPLES
		GPIOC->BSRRH = 16;
#endif

		while(vet_in_func != vet_in_DMA);

#ifdef TEST_UDA1380_RECEIVESAMPLES
		GPIOC->BSRRL = 16;
#endif

		__disable_irq();
		vet_in_func = 3-vet_in_func;
		pos_receive = 0;
		__enable_irq();

		if (vet_in_func == 1)
			vet = vet_in1;
		else
			vet = vet_in2;

		for(i = index; i < num_samples; i += UNROLL_FACTOR)
		{
#if UNROLL_FACTOR >= 1
			samples_L[i] = *vet++;
			samples_R[i] = *vet++;
#if UNROLL_FACTOR >= 2
			samples_L[i+1] = *vet++;
			samples_R[i+1] = *vet++;
#if UNROLL_FACTOR >= 4
			samples_L[i+2] = *vet++;
			samples_R[i+2] = *vet++;

			samples_L[i+3] = *vet++;
			samples_R[i+3] = *vet++;
#if UNROLL_FACTOR >= 8
			samples_L[i+4] = *vet++;
			samples_R[i+4] = *vet++;

			samples_L[i+5] = *vet++;
			samples_R[i+5] = *vet++;

			samples_L[i+6] = *vet++;
			samples_R[i+6] = *vet++;

			samples_L[i+7] = *vet++;
			samples_R[i+7] = *vet++;
#endif
#endif
#endif
#endif
			pos_receive += 2*UNROLL_FACTOR;
		}
	}

#ifdef TEST_UDA1380_RECEIVESAMPLES
	GPIOC->BSRRH = 16;
#endif

}

void UDA1380_ReceiveSamplesMono(int16_t samples[], int num_samples)
{
	int i;
	int16_t* vet;
	int index;

#ifdef TEST_UDA1380_RECEIVESAMPLESMONO
	GPIOC->BSRRL = 16;
#endif

	UDA1380_assert_param(ADC_StateFlag == ENABLE, "Voce tentou receber amostras sem habilitar o UDA1380_ADC_Enable na UDA1380_InitStructure\n");
	UDA1380_assert_param((num_samples % UNROLL_FACTOR) == 0, "Nas funcoes para enviar e receber amostras, num_samples deve ser multiplo de " STR_DEFINE(UNROLL_FACTOR) "\n");
	UDA1380_assert_param(num_samples <= queue_length/4, "Voce tentou receber uma quantidade muito grande de amostras de uma vez; reduza numero de amostras ou aumente o valor do campo UDA1380_Queue_Length na struct UDA1380_InitStructure\n");

	while(vet_in_func == -1);

	if (vet_in_func == 1)
		vet = &vet_in1[pos_receive];
	else
		vet = &vet_in2[pos_receive];

	index = (queue_length/2 - pos_receive)/2;

	if(index >= num_samples)
		index = num_samples;

	for(i = 0; i < index; i += UNROLL_FACTOR)
	{
#if UNROLL_FACTOR >= 1
		samples[i] = *vet;
		vet += 2;
#if UNROLL_FACTOR >= 2
		samples[i+1] = *vet;
		vet += 2;
#if UNROLL_FACTOR >= 4
		samples[i+2] = *vet;
		vet += 2;

		samples[i+3] = *vet;
		vet += 2;
#if UNROLL_FACTOR >= 8
		samples[i+4] = *vet;
		vet += 2;

		samples[i+5] = *vet;
		vet += 2;

		samples[i+6] = *vet;
		vet += 2;

		samples[i+7] = *vet;
		vet += 2;
#endif
#endif
#endif
#endif
		pos_receive += 2*UNROLL_FACTOR;
	}

	if(index < num_samples)
	{

#ifdef TEST_UDA1380_RECEIVESAMPLESMONO
		GPIOC->BSRRH = 16;
#endif

		while(vet_in_func != vet_in_DMA);

#ifdef TEST_UDA1380_RECEIVESAMPLESMONO
		GPIOC->BSRRL = 16;
#endif

		__disable_irq();
		vet_in_func = 3-vet_in_func;
		pos_receive = 0;
		__enable_irq();

		if (vet_in_func == 1)
			vet = vet_in1;
		else
			vet = vet_in2;

		for(i = index; i < num_samples; i += UNROLL_FACTOR)
		{
#if UNROLL_FACTOR >= 1
			samples[i] = *vet;
			vet += 2;

#if UNROLL_FACTOR >= 2
			samples[i+1] = *vet;
			vet += 2;
#if UNROLL_FACTOR >= 4
			samples[i+2] = *vet;
			vet += 2;

			samples[i+3] = *vet;
			vet += 2;
#if UNROLL_FACTOR >= 8
			samples[i+4] = *vet;
			vet += 2;

			samples[i+5] = *vet;
			vet += 2;

			samples[i+6] = *vet;
			vet += 2;

			samples[i+7] = *vet;
			vet += 2;
#endif
#endif
#endif
#endif
			pos_receive += 2*UNROLL_FACTOR;
		}
	}

#ifdef TEST_UDA1380_RECEIVESAMPLESMONO
	GPIOC->BSRRH = 16;
#endif

}

void UDA1380_ReceiveSamplesFloat(float samples_L[], float samples_R[], int num_samples)
{
	int i;
	int16_t* vet;
	int index;

#ifdef TEST_UDA1380_RECEIVESAMPLESFLOAT
	GPIOC->BSRRL = 16;
#endif

	UDA1380_assert_param(ADC_StateFlag == ENABLE, "Voce tentou receber amostras sem habilitar o UDA1380_ADC_Enable na UDA1380_InitStructure\n");
	UDA1380_assert_param((num_samples % UNROLL_FACTOR) == 0, "Nas funcoes para enviar e receber amostras, num_samples deve ser multiplo de " STR_DEFINE(UNROLL_FACTOR) "\n");
	UDA1380_assert_param(num_samples <= queue_length/4, "Voce tentou receber uma quantidade muito grande de amostras de uma vez; reduza numero de amostras ou aumente o valor do campo UDA1380_Queue_Length na struct UDA1380_InitStructure\n");

	while(vet_in_func == -1);

	if (vet_in_func == 1)
		vet = &vet_in1[pos_receive];
	else
		vet = &vet_in2[pos_receive];

	index = (queue_length/2 - pos_receive)/2;

	if(index >= num_samples)
		index = num_samples;

	for(i = 0; i < index; i += UNROLL_FACTOR)
	{
#if UNROLL_FACTOR >= 1
		samples_L[i] = fixed_point_to_float(*vet++);
		samples_R[i] = fixed_point_to_float(*vet++);
#if UNROLL_FACTOR >= 2
		samples_L[i+1] = fixed_point_to_float(*vet++);
		samples_R[i+1] = fixed_point_to_float(*vet++);
#if UNROLL_FACTOR >= 4
		samples_L[i+2] = fixed_point_to_float(*vet++);
		samples_R[i+2] = fixed_point_to_float(*vet++);

		samples_L[i+3] = fixed_point_to_float(*vet++);
		samples_R[i+3] = fixed_point_to_float(*vet++);
#if UNROLL_FACTOR >= 8
		samples_L[i+4] = fixed_point_to_float(*vet++);
		samples_R[i+4] = fixed_point_to_float(*vet++);

		samples_L[i+5] = fixed_point_to_float(*vet++);
		samples_R[i+5] = fixed_point_to_float(*vet++);

		samples_L[i+6] = fixed_point_to_float(*vet++);
		samples_R[i+6] = fixed_point_to_float(*vet++);

		samples_L[i+7] = fixed_point_to_float(*vet++);
		samples_R[i+7] = fixed_point_to_float(*vet++);
#endif
#endif
#endif
#endif
		pos_receive += 2*UNROLL_FACTOR;
	}

	if(index < num_samples)
	{

#ifdef TEST_UDA1380_RECEIVESAMPLESFLOAT
	GPIOC->BSRRH = 16;
#endif

		while(vet_in_func != vet_in_DMA);

#ifdef TEST_UDA1380_RECEIVESAMPLESFLOAT
	GPIOC->BSRRL = 16;
#endif

		__disable_irq();
		vet_in_func = 3-vet_in_func;
		pos_receive = 0;
		__enable_irq();

		if (vet_in_func == 1)
			vet = vet_in1;
		else
			vet = vet_in2;

		for(i = index; i < num_samples; i += UNROLL_FACTOR)
		{
#if UNROLL_FACTOR >= 1
			samples_L[i] = fixed_point_to_float(*vet++);
			samples_R[i] = fixed_point_to_float(*vet++);
#if UNROLL_FACTOR >= 2
			samples_L[i+1] = fixed_point_to_float(*vet++);
			samples_R[i+1] = fixed_point_to_float(*vet++);
#if UNROLL_FACTOR >= 4
			samples_L[i+2] = fixed_point_to_float(*vet++);
			samples_R[i+2] = fixed_point_to_float(*vet++);

			samples_L[i+3] = fixed_point_to_float(*vet++);
			samples_R[i+3] = fixed_point_to_float(*vet++);
#if UNROLL_FACTOR >= 8
			samples_L[i+4] = fixed_point_to_float(*vet++);
			samples_R[i+4] = fixed_point_to_float(*vet++);

			samples_L[i+5] = fixed_point_to_float(*vet++);
			samples_R[i+5] = fixed_point_to_float(*vet++);

			samples_L[i+6] = fixed_point_to_float(*vet++);
			samples_R[i+6] = fixed_point_to_float(*vet++);

			samples_L[i+7] = fixed_point_to_float(*vet++);
			samples_R[i+7] = fixed_point_to_float(*vet++);
#endif
#endif
#endif
#endif
			pos_receive += 2*UNROLL_FACTOR;
		}
	}

#ifdef TEST_UDA1380_RECEIVESAMPLESFLOAT
	GPIOC->BSRRH = 16;
#endif

}

void UDA1380_ReceiveSamplesFloatMono(float samples[], int num_samples)
{
	int i;
	int16_t* vet;
	int index;

#ifdef TEST_UDA1380_RECEIVESAMPLESFLOATMONO
	GPIOC->BSRRL = 16;
#endif

	UDA1380_assert_param(ADC_StateFlag == ENABLE, "Voce tentou receber amostras sem habilitar o UDA1380_ADC_Enable na UDA1380_InitStructure\n");
	UDA1380_assert_param((num_samples % UNROLL_FACTOR) == 0, "Nas funcoes para enviar e receber amostras, num_samples deve ser multiplo de " STR_DEFINE(UNROLL_FACTOR) "\n");
	UDA1380_assert_param(num_samples <= queue_length/4, "Voce tentou receber uma quantidade muito grande de amostras de uma vez; reduza numero de amostras ou aumente o valor do campo UDA1380_Queue_Length na struct UDA1380_InitStructure\n");

	while(vet_in_func == -1);

	if (vet_in_func == 1)
		vet = &vet_in1[pos_receive];
	else
		vet = &vet_in2[pos_receive];

	index = (queue_length/2 - pos_receive)/2;

	if(index >= num_samples)
		index = num_samples;

	for(i = 0; i < index; i += UNROLL_FACTOR)
	{
#if UNROLL_FACTOR >= 1
		samples[i] = fixed_point_to_float(*vet);
		vet += 2;
#if UNROLL_FACTOR >= 2
		samples[i+1] = fixed_point_to_float(*vet);
		vet += 2;
#if UNROLL_FACTOR >= 4
		samples[i+2] = fixed_point_to_float(*vet);
		vet += 2;

		samples[i+3] = fixed_point_to_float(*vet);
		vet += 2;
#if UNROLL_FACTOR >= 8
		samples[i+4] = fixed_point_to_float(*vet);
		vet += 2;

		samples[i+5] = fixed_point_to_float(*vet);
		vet += 2;

		samples[i+6] = fixed_point_to_float(*vet);
		vet += 2;

		samples[i+7] = fixed_point_to_float(*vet);
		vet += 2;
#endif
#endif
#endif
#endif
		pos_receive += 2*UNROLL_FACTOR;
	}

	if(index < num_samples)
	{

#ifdef TEST_UDA1380_RECEIVESAMPLESFLOATMONO
	GPIOC->BSRRH = 16;
#endif

		while(vet_in_func != vet_in_DMA);

#ifdef TEST_UDA1380_RECEIVESAMPLESFLOATMONO
	GPIOC->BSRRL = 16;
#endif

		__disable_irq();
		vet_in_func = 3-vet_in_func;
		pos_receive = 0;
		__enable_irq();

		if (vet_in_func == 1)
			vet = vet_in1;
		else
			vet = vet_in2;

		for(i = index; i < num_samples; i += UNROLL_FACTOR)
		{
#if UNROLL_FACTOR >= 1
			samples[i] = fixed_point_to_float(*vet);
			vet += 2;
#if UNROLL_FACTOR >= 2
			samples[i+1] = fixed_point_to_float(*vet);
			vet += 2;
#if UNROLL_FACTOR >= 4
			samples[i+2] = fixed_point_to_float(*vet);
			vet += 2;

			samples[i+3] = fixed_point_to_float(*vet);
			vet += 2;
#if UNROLL_FACTOR >= 8
			samples[i+4] = fixed_point_to_float(*vet);
			vet += 2;

			samples[i+5] = fixed_point_to_float(*vet);
			vet += 2;

			samples[i+6] = fixed_point_to_float(*vet);
			vet += 2;

			samples[i+7] = fixed_point_to_float(*vet);
			vet += 2;
#endif
#endif
#endif
#endif
			pos_receive += 2*UNROLL_FACTOR;
		}
	}

#ifdef TEST_UDA1380_RECEIVESAMPLESFLOATMONO
	GPIOC->BSRRH = 16;
#endif

}

void UDA1380_SendSamples(const int16_t samples_L[], const int16_t samples_R[], int num_samples)
{
	int i;
	int16_t* vet;
	int index;

#ifdef TEST_UDA1380_SENDSAMPLES
	GPIOA->BSRRL = 8;
#endif

	UDA1380_assert_param(DAC_StateFlag == ENABLE, "Voce tentou enviar amostras sem habilitar o UDA1380_DAC_Enable na UDA1380_InitStructure\n");
	UDA1380_assert_param((num_samples % UNROLL_FACTOR) == 0, "Nas funcoes para enviar e receber amostras, num_samples deve ser multiplo de " STR_DEFINE(UNROLL_FACTOR) "\n");
	UDA1380_assert_param(num_samples <= queue_length/4, "Voce tentou enviar uma quantidade muito grande de amostras de uma vez; reduza numero de amostras ou aumente o valor do campo UDA1380_Queue_Length na struct UDA1380_InitStructure\n");

	if (vet_out_func == 1)
		vet = &vet_out1[pos_send];
	else
		vet = &vet_out2[pos_send];

	index = (queue_length/2 - pos_send)/2;

	if(index >= num_samples)
		index = num_samples;

	for(i = 0; i < index; i += UNROLL_FACTOR)
	{
#if UNROLL_FACTOR >= 1
		*vet++ = samples_L[i];
		*vet++ = samples_R[i];
#if UNROLL_FACTOR >= 2
		*vet++ = samples_L[i+1];
		*vet++ = samples_R[i+1];
#if UNROLL_FACTOR >= 4
		*vet++ = samples_L[i+2];
		*vet++ = samples_R[i+2];

		*vet++ = samples_L[i+3];
		*vet++ = samples_R[i+3];
#if UNROLL_FACTOR >= 8
		*vet++ = samples_L[i+4];
		*vet++ = samples_R[i+4];

		*vet++ = samples_L[i+5];
		*vet++ = samples_R[i+5];

		*vet++ = samples_L[i+6];
		*vet++ = samples_R[i+6];

		*vet++ = samples_L[i+7];
		*vet++ = samples_R[i+7];
#endif
#endif
#endif
#endif
		pos_send += 2*UNROLL_FACTOR;
	}

	if(index < num_samples)
	{
		if(vet_out_DMA==-1)
		{
			DMA_Cmd(DMA1_Stream4,ENABLE);
			vet_out_DMA = vet_out_func;
		}
		else
		{

#ifdef TEST_UDA1380_SENDSAMPLES
		GPIOA->BSRRH = 8;
#endif

			while(vet_out_func != vet_out_DMA);

#ifdef TEST_UDA1380_SENDSAMPLES
		GPIOA->BSRRL = 8;
#endif

		}

		__disable_irq();
		vet_out_func = 3-vet_out_func;
		pos_send = 0;
		__enable_irq();

		if (vet_out_func == 1)
			vet = vet_out1;
		else
			vet = vet_out2;

		for(i = index; i < num_samples; i += UNROLL_FACTOR)
		{
#if UNROLL_FACTOR >= 1
			*vet++ = samples_L[i];
			*vet++ = samples_R[i];
#if UNROLL_FACTOR >= 2
			*vet++ = samples_L[i+1];
			*vet++ = samples_R[i+1];
#if UNROLL_FACTOR >= 4
			*vet++ = samples_L[i+2];
			*vet++ = samples_R[i+2];

			*vet++ = samples_L[i+3];
			*vet++ = samples_R[i+3];
#if UNROLL_FACTOR >= 8
			*vet++ = samples_L[i+4];
			*vet++ = samples_R[i+4];

			*vet++ = samples_L[i+5];
			*vet++ = samples_R[i+5];

			*vet++ = samples_L[i+6];
			*vet++ = samples_R[i+6];

			*vet++ = samples_L[i+7];
			*vet++ = samples_R[i+7];
#endif
#endif
#endif
#endif
			pos_send += 2*UNROLL_FACTOR;
		}
	}

#ifdef TEST_UDA1380_SENDSAMPLES
		GPIOA->BSRRH = 8;
#endif

}

void UDA1380_SendSamplesMono(const int16_t samples[], int num_samples)
{
	int i;
	int16_t* vet;
	int index;

#ifdef TEST_UDA1380_SENDSAMPLESMONO
	GPIOA->BSRRL = 8;
#endif

	UDA1380_assert_param(DAC_StateFlag == ENABLE, "Voce tentou enviar amostras sem habilitar o UDA1380_DAC_Enable na UDA1380_InitStructure\n");
	UDA1380_assert_param((num_samples % UNROLL_FACTOR) == 0, "Nas funcoes para enviar e receber amostras, num_samples deve ser multiplo de " STR_DEFINE(UNROLL_FACTOR) "\n");
	UDA1380_assert_param(num_samples <= queue_length/4, "Voce tentou enviar uma quantidade muito grande de amostras de uma vez; reduza numero de amostras ou aumente o valor do campo UDA1380_Queue_Length na struct UDA1380_InitStructure\n");

	if (vet_out_func == 1)
		vet = &vet_out1[pos_send];
	else
		vet = &vet_out2[pos_send];

	index = (queue_length/2 - pos_send)/2;

	if(index >= num_samples)
		index = num_samples;

	for(i = 0; i < index; i += UNROLL_FACTOR)
	{
#if UNROLL_FACTOR >= 1
		*vet++ = samples[i];
		*vet++ = samples[i];
#if UNROLL_FACTOR >= 2
		*vet++ = samples[i+1];
		*vet++ = samples[i+1];
#if UNROLL_FACTOR >= 4
		*vet++ = samples[i+2];
		*vet++ = samples[i+2];

		*vet++ = samples[i+3];
		*vet++ = samples[i+3];
#if UNROLL_FACTOR >= 8
		*vet++ = samples[i+4];
		*vet++ = samples[i+4];

		*vet++ = samples[i+5];
		*vet++ = samples[i+5];

		*vet++ = samples[i+6];
		*vet++ = samples[i+6];

		*vet++ = samples[i+7];
		*vet++ = samples[i+7];
#endif
#endif
#endif
#endif
		pos_send += 2*UNROLL_FACTOR;
	}

	if(index < num_samples)
	{
		if(vet_out_DMA==-1)
		{
			DMA_Cmd(DMA1_Stream4,ENABLE);
			vet_out_DMA = vet_out_func;
		}
		else
		{

#ifdef TEST_UDA1380_SENDSAMPLESMONO
			GPIOA->BSRRH = 8;
#endif

			while(vet_out_func != vet_out_DMA);

#ifdef TEST_UDA1380_SENDSAMPLESMONO
			GPIOA->BSRRL = 8;
#endif

		}

		__disable_irq();
		vet_out_func = 3-vet_out_func;
		pos_send = 0;
		__enable_irq();

		if (vet_out_func == 1)
			vet = vet_out1;
		else
			vet = vet_out2;

		for(i = index; i < num_samples; i += UNROLL_FACTOR)
		{
#if UNROLL_FACTOR >= 1
			*vet++ = samples[i];
			*vet++ = samples[i];
#if UNROLL_FACTOR >= 2
			*vet++ = samples[i+1];
			*vet++ = samples[i+1];
#if UNROLL_FACTOR >= 4
			*vet++ = samples[i+2];
			*vet++ = samples[i+2];

			*vet++ = samples[i+3];
			*vet++ = samples[i+3];
#if UNROLL_FACTOR >= 8
			*vet++ = samples[i+4];
			*vet++ = samples[i+4];

			*vet++ = samples[i+5];
			*vet++ = samples[i+5];

			*vet++ = samples[i+6];
			*vet++ = samples[i+6];

			*vet++ = samples[i+7];
			*vet++ = samples[i+7];
#endif
#endif
#endif
#endif
			pos_send += 2*UNROLL_FACTOR;
		}
	}

#ifdef TEST_UDA1380_SENDSAMPLESMONO
			GPIOA->BSRRH = 8;
#endif

}

void UDA1380_SendSamplesFloat(const float samples_L[], const float samples_R[], int num_samples)
{
	int i;
	int16_t* vet;
	int index;

#ifdef TEST_UDA1380_SENDSAMPLESFLOAT
	GPIOA->BSRRL = 8;
#endif

	UDA1380_assert_param(DAC_StateFlag == ENABLE, "Voce tentou enviar amostras sem habilitar o UDA1380_DAC_Enable na UDA1380_InitStructure\n");
	UDA1380_assert_param((num_samples % UNROLL_FACTOR) == 0, "Nas funcoes para enviar e receber amostras, num_samples deve ser multiplo de " STR_DEFINE(UNROLL_FACTOR) "\n");
	UDA1380_assert_param(num_samples <= queue_length/4, "Voce tentou enviar uma quantidade muito grande de amostras de uma vez; reduza numero de amostras ou aumente o valor do campo UDA1380_Queue_Length na struct UDA1380_InitStructure\n");

	if (vet_out_func == 1)
		vet = &vet_out1[pos_send];
	else
		vet = &vet_out2[pos_send];

	index = (queue_length/2 - pos_send)/2;

	if(index >= num_samples)
		index = num_samples;

	for(i = 0; i < index; i += UNROLL_FACTOR)
	{
#if UNROLL_FACTOR >= 1
		*vet++ = float_to_fixed_point(samples_L[i]);
		*vet++ = float_to_fixed_point(samples_R[i]);
#if UNROLL_FACTOR >= 2
		*vet++ = float_to_fixed_point(samples_L[i+1]);
		*vet++ = float_to_fixed_point(samples_R[i+1]);
#if UNROLL_FACTOR >= 4
		*vet++ = float_to_fixed_point(samples_L[i+2]);
		*vet++ = float_to_fixed_point(samples_R[i+2]);

		*vet++ = float_to_fixed_point(samples_L[i+3]);
		*vet++ = float_to_fixed_point(samples_R[i+3]);
#if UNROLL_FACTOR >= 8
		*vet++ = float_to_fixed_point(samples_L[i+4]);
		*vet++ = float_to_fixed_point(samples_R[i+4]);

		*vet++ = float_to_fixed_point(samples_L[i+5]);
		*vet++ = float_to_fixed_point(samples_R[i+5]);

		*vet++ = float_to_fixed_point(samples_L[i+6]);
		*vet++ = float_to_fixed_point(samples_R[i+6]);

		*vet++ = float_to_fixed_point(samples_L[i+7]);
		*vet++ = float_to_fixed_point(samples_R[i+7]);
#endif
#endif
#endif
#endif
		pos_send += 2*UNROLL_FACTOR;
	}

	if(index < num_samples)
	{
		if(vet_out_DMA==-1)
		{
			DMA_Cmd(DMA1_Stream4,ENABLE);
			vet_out_DMA = vet_out_func;
		}
		else
		{

#ifdef TEST_UDA1380_SENDSAMPLESFLOAT
			GPIOA->BSRRH = 8;
#endif

			while(vet_out_func != vet_out_DMA);

#ifdef TEST_UDA1380_SENDSAMPLESFLOAT
			GPIOA->BSRRL = 8;
#endif

		}

		__disable_irq();
		vet_out_func = 3-vet_out_func;
		pos_send = 0;
		__enable_irq();

		if (vet_out_func == 1)
			vet = vet_out1;
		else
			vet = vet_out2;

		for(i = index; i < num_samples; i += UNROLL_FACTOR)
		{
#if UNROLL_FACTOR >= 1
			*vet++ = float_to_fixed_point(samples_L[i]);
			*vet++ = float_to_fixed_point(samples_R[i]);
#if UNROLL_FACTOR >= 2
			*vet++ = float_to_fixed_point(samples_L[i+1]);
			*vet++ = float_to_fixed_point(samples_R[i+1]);
#if UNROLL_FACTOR >= 4
			*vet++ = float_to_fixed_point(samples_L[i+2]);
			*vet++ = float_to_fixed_point(samples_R[i+2]);

			*vet++ = float_to_fixed_point(samples_L[i+3]);
			*vet++ = float_to_fixed_point(samples_R[i+3]);
#if UNROLL_FACTOR >= 8
			*vet++ = float_to_fixed_point(samples_L[i+4]);
			*vet++ = float_to_fixed_point(samples_R[i+4]);

			*vet++ = float_to_fixed_point(samples_L[i+5]);
			*vet++ = float_to_fixed_point(samples_R[i+5]);

			*vet++ = float_to_fixed_point(samples_L[i+6]);
			*vet++ = float_to_fixed_point(samples_R[i+6]);

			*vet++ = float_to_fixed_point(samples_L[i+7]);
			*vet++ = float_to_fixed_point(samples_R[i+7]);
#endif
#endif
#endif
#endif
			pos_send += 2*UNROLL_FACTOR;
		}
	}

#ifdef TEST_UDA1380_SENDSAMPLESFLOAT
			GPIOA->BSRRH = 8;
#endif

}

void UDA1380_SendSamplesFloatMono(const float samples[], int num_samples)
{
	int i;
	int16_t* vet;
	int index;
	int16_t temp;

#ifdef TEST_UDA1380_SENDSAMPLESFLOATMONO
	GPIOA->BSRRL = 8;
#endif

	UDA1380_assert_param(DAC_StateFlag == ENABLE, "Voce tentou enviar amostras sem habilitar o UDA1380_DAC_Enable na UDA1380_InitStructure\n");
	UDA1380_assert_param((num_samples % UNROLL_FACTOR) == 0, "Nas funcoes para enviar e receber amostras, num_samples deve ser multiplo de " STR_DEFINE(UNROLL_FACTOR) "\n");
	UDA1380_assert_param(num_samples <= queue_length/4, "Voce tentou enviar uma quantidade muito grande de amostras de uma vez; reduza numero de amostras ou aumente o valor do campo UDA1380_Queue_Length na struct UDA1380_InitStructure\n");

	if (vet_out_func == 1)
		vet = &vet_out1[pos_send];
	else
		vet = &vet_out2[pos_send];

	index = (queue_length/2 - pos_send)/2;

	if(index >= num_samples)
		index = num_samples;

	for(i = 0; i < index; i += UNROLL_FACTOR)
	{
#if UNROLL_FACTOR >= 1
		temp = float_to_fixed_point(samples[i]);
		*vet++ = temp;
		*vet++ = temp;
#if UNROLL_FACTOR >= 2
		temp = float_to_fixed_point(samples[i+1]);
		*vet++ = temp;
		*vet++ = temp;
#if UNROLL_FACTOR >= 4
		temp = float_to_fixed_point(samples[i+2]);
		*vet++ = temp;
		*vet++ = temp;

		temp = float_to_fixed_point(samples[i+3]);
		*vet++ = temp;
		*vet++ = temp;
#if UNROLL_FACTOR >= 8
		temp = float_to_fixed_point(samples[i+4]);
		*vet++ = temp;
		*vet++ = temp;

		temp = float_to_fixed_point(samples[i+5]);
		*vet++ = temp;
		*vet++ = temp;

		temp = float_to_fixed_point(samples[i+6]);
		*vet++ = temp;
		*vet++ = temp;

		temp = float_to_fixed_point(samples[i+7]);
		*vet++ = temp;
		*vet++ = temp;
#endif
#endif
#endif
#endif
		pos_send += 2*UNROLL_FACTOR;
	}

	if(index < num_samples)
	{
		if(vet_out_DMA==-1)
		{
			DMA_Cmd(DMA1_Stream4,ENABLE);
			vet_out_DMA = vet_out_func;
		}
		else
		{

#ifdef TEST_UDA1380_SENDSAMPLESFLOATMONO
			GPIOA->BSRRH = 8;
#endif

			while(vet_out_func != vet_out_DMA);

#ifdef TEST_UDA1380_SENDSAMPLESFLOATMONO
			GPIOA->BSRRL = 8;
#endif

		}

		__disable_irq();
		vet_out_func = 3-vet_out_func;
		pos_send = 0;
		__enable_irq();

		if (vet_out_func == 1)
			vet = vet_out1;
		else
			vet = vet_out2;

		for(i = index; i < num_samples; i += UNROLL_FACTOR)
		{
#if UNROLL_FACTOR >= 1
			temp = float_to_fixed_point(samples[i]);
			*vet++ = temp;
			*vet++ = temp;
#if UNROLL_FACTOR >= 2
			temp = float_to_fixed_point(samples[i+1]);
			*vet++ = temp;
			*vet++ = temp;
#if UNROLL_FACTOR >= 4
			temp = float_to_fixed_point(samples[i+2]);
			*vet++ = temp;
			*vet++ = temp;

			temp = float_to_fixed_point(samples[i+3]);
			*vet++ = temp;
			*vet++ = temp;
#if UNROLL_FACTOR >= 8
			temp = float_to_fixed_point(samples[i+4]);
			*vet++ = temp;
			*vet++ = temp;

			temp = float_to_fixed_point(samples[i+5]);
			*vet++ = temp;
			*vet++ = temp;

			temp = float_to_fixed_point(samples[i+6]);
			*vet++ = temp;
			*vet++ = temp;

			temp = float_to_fixed_point(samples[i+7]);
			*vet++ = temp;
			*vet++ = temp;
#endif
#endif
#endif
#endif
			pos_send += 2*UNROLL_FACTOR;
		}
	}

#ifdef TEST_UDA1380_SENDSAMPLESFLOATMONO
			GPIOA->BSRRH = 8;
#endif

}
