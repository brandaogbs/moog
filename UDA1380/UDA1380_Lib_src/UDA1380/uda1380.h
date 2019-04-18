#ifndef UDA1380_H
#define UDA1380_H

#include "stm32f4xx.h"

typedef enum
{
	UDA1380_Input_Microphone, UDA1380_Input_LineIn
} UDA1380_InputTypeDef;

typedef enum
{
	UDA1380_ADC_48000_DAC_48000,
	UDA1380_ADC_48000_DAC_32000,
	UDA1380_ADC_48000_DAC_16000,
	UDA1380_ADC_48000_DAC_8000,

	UDA1380_ADC_32000_DAC_48000,
	UDA1380_ADC_32000_DAC_32000,
	UDA1380_ADC_32000_DAC_16000,
	UDA1380_ADC_32000_DAC_8000,

	UDA1380_ADC_16000_DAC_48000,
	UDA1380_ADC_16000_DAC_32000,
	UDA1380_ADC_16000_DAC_16000,
	UDA1380_ADC_16000_DAC_8000,

	UDA1380_ADC_8000_DAC_48000,
	UDA1380_ADC_8000_DAC_32000,
	UDA1380_ADC_8000_DAC_16000,
	UDA1380_ADC_8000_DAC_8000,

	UDA1380_ADC_44100_DAC_44100,
	UDA1380_ADC_44100_DAC_22050,
	UDA1380_ADC_44100_DAC_11025,

	UDA1380_ADC_22050_DAC_44100,
	UDA1380_ADC_22050_DAC_22050,
	UDA1380_ADC_22050_DAC_11025,

	UDA1380_ADC_11025_DAC_44100,
	UDA1380_ADC_11025_DAC_22050,
	UDA1380_ADC_11025_DAC_11025,
} UDA1380_SampleRateTypeDef;

typedef struct
{
	FunctionalState UDA1380_ADC_Enable;
	FunctionalState UDA1380_DAC_Enable;
	FunctionalState UDA1380_Real_Time_Errors;
	UDA1380_InputTypeDef UDA1380_Input;
	UDA1380_SampleRateTypeDef UDA1380_Sample_Rate;
	float UDA1380_Volume_Att;
	int16_t* UDA1380_Buffer_In;
	int16_t* UDA1380_Buffer_Out;
	int UDA1380_Queue_Length;
	void (*UDA1380_Callback)();
} UDA1380_InitTypeDef;


void UDA1380_StructInit(UDA1380_InitTypeDef*);
void UDA1380_Init(UDA1380_InitTypeDef*);

void UDA1380_SendSamples(const int16_t samples_L[], const int16_t samples_R[], int num_samples);
void UDA1380_SendSamplesFloat(const float samples_L[], const float samples_R[], int num_samples);
void UDA1380_SendSamplesMono(const int16_t samples[], int num_samples);
void UDA1380_SendSamplesFloatMono(const float samples[], int num_samples);

void UDA1380_ReceiveSamples(int16_t samples_L[], int16_t samples_R[], int num_samples);
void UDA1380_ReceiveSamplesFloat(float samples_L[], float samples_R[], int num_samples);
void UDA1380_ReceiveSamplesMono(int16_t samples[], int num_samples);
void UDA1380_ReceiveSamplesFloatMono(float samples[], int num_samples);

#endif // UDA1380_H


// TODO: testar todos os erros possiveis, um por um, com -Og, -O0 e -O3, e lembrar de checar se nao ocorreu estouro de pilha na UDA1380_error_task
