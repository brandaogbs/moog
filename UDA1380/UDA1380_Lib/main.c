#include "math.h"
#include "uda1380.h"
#include "arm_math.h"
#include "fdacoefs.h"
#include "fdacoefs2.h"
#include "wavetable.h"
#include "tm_stm32f4_rng.h"
#include "usart.h"
#include "teclado.h"

// Os parametros abaixo estao nos valores minimos para reduzir latência e gasto de memória
#define QUEUE_LEN 2048
#define BLOCK_SIZE 64
#define SAMPLERATE 48000
#define WT_LEN 1024 			// Tamanho da wavetable (wavetable.h)
// Tipos de onda dos osciladores
typedef enum {
	square = 0, triangle, sawtooth, distosine, sine, cosine
} Wave_form;

typedef enum {
	attack = 0, sustain, decay, release, wait
} Envelope_state;

typedef enum {
	disable = 0, enable
} State;

typedef enum {
	solto = 0, pressionado
} State_botao;

const float32_t pitch_base[] = { 16.35f, 17.32f, 18.35f, 19.45f, 20.60f, 21.83f,
		23.12f, 24.50f, 25.96f, 27.50f, 29.14f, 30.87f };

typedef struct {
	State state;
	float32_t ampl;
	float32_t wtp_osc;
	float32_t wtp_inc;
	float32_t freq;
	float32_t range;
	float32_t tune;
	Wave_form wave;
} Oscillator;

typedef struct {
	Envelope_state state;
	float32_t output;
	float32_t inc_att;
	float32_t inc_dec;
	float32_t att;
	float32_t dec;
	float32_t sus;
} Envelope;

typedef struct {
	State state;
	Oscillator osc;
} LFO;

/* Low Pass Filter */
typedef struct {
	State state;

	float32_t fc;
	float32_t q;
	float32_t cutoff;
	float32_t contour;

} LPF;

typedef struct {
	/* LFO */
	LFO lfo1;
	LFO lfo2;

	/* LPF */
	LPF lpf1;

	/* Oscillator */
	Oscillator osc1;
	Oscillator osc2;
	Oscillator osc3;

	/* Envelope de volume */
	Envelope env1;

	/* Envelope de frequencia */
	Envelope env2;

	/* Noise */
	State noise_state;
	float32_t noise_ampl;

	/* Geral */
	uint8_t pitchi;
	State_botao botao_state;
} Synth;

/* Protótipos das funções */
float32_t osc_ops(Oscillator *osc);
float32_t noise(Synth *synth);
void shift_signal_buf(float32_t *signal_buf, float32_t signal);
float32_t lpf(LPF lpf, float32_t signal);
void volume_envelope(Envelope *vol_env);
float32_t lfo(LFO *lfo);
void read_pack(char *pack);

/*Variáveis Globais*/

Synth synth;

float32_t signal_buf_in[3] = { 0 }, signal_buf_out[3] = { 0 };

int16_t buf_in[QUEUE_LEN], buf_out[QUEUE_LEN];

float32_t buf_mono[BLOCK_SIZE];

volatile uint8_t iBuff_mono = 0;

float32_t osc_ops(Oscillator *osc) {
	uint32_t a, b;
	float32_t da, db, signal;

	if (osc->state == enable) {

		osc->wtp_inc = 1024.0f / 48000.0f;
		//Interpolacao linear
		a = (int) osc->wtp_osc;
		da = osc->wtp_osc - a;
		b = a + 1;
		db = b - osc->wtp_osc;

		if (b == WT_LEN)
			b = 0;

		switch (osc->wave) {
		case square:
			signal = db * squarewave[a] + da * squarewave[b];
			break;

		case triangle:
			signal = db * trianglewave[a] + da * trianglewave[b];
			break;

		case sawtooth:
			signal = db * sawtoothwave[a] + da * sawtoothwave[b];
			break;

		case distosine:
			signal = db * distosinewave[a] + da * distosinewave[b];
			break;

		case sine:
			signal = db * sinewave[a] + da * sinewave[b];
			break;

		case cosine:
			signal = db * cosinewave[a] + da * cosinewave[b];
			break;
		}
		osc->wtp_osc = osc->wtp_osc + osc->freq * osc->wtp_inc;

		if (osc->wtp_osc > WT_LEN) {
			osc->wtp_osc = osc->wtp_osc - WT_LEN;
		}
	} else {
		signal = 0;
	}

	return signal * osc->ampl;
}

float32_t noise(Synth *synth) {

	if (synth->noise_state == enable) {
		return ((TM_RNG_Get() % 10) / 10.0 / 10.0) * synth->noise_ampl;
	}

	return 0;
}

/* Realiza um shift unitário para os buffers de entrada e saida*/
void shift_signal_buf(float32_t *signal_buf, float32_t signal) {
	signal_buf[0] = signal_buf[1];
	signal_buf[1] = signal_buf[2];
	signal_buf[2] = signal;
}

/* Aplica filtro passa baixa no sinal, utiliza as duas ultimas amostras e a atual*/
float32_t lpf(LPF lpf, float32_t signal) {
	float32_t w0, cos_w0, sin_w0, alpha, ft[3][3];

	if (lpf.state == enable) {
		frequency_envelope(&synth.env2);
		w0 = 2.0f * (float32_t) M_PI * lpf.fc * (1.0f + lfo(&synth.lfo2))*(1 + synth.lpf1.contour * synth.env2.output)
				/ 48000.0f;
		cos_w0 = cosf(w0);

		sin_w0 = sinf(w0);
		alpha = sin_w0 / (2.0f * lpf.q);

		/* filter coef */
		ft[0][1] = (1.0f - cos_w0) / 2; //b0
		ft[1][1] = 1.0f - cos_w0; //b1
		ft[2][1] = (1.0f - cos_w0) / 2; //b2
		ft[0][0] = 1.0f + alpha; //a0
		ft[1][0] = -2.0f * cos_w0; //a1
		ft[2][0] = 1.0f - alpha; //a2

		signal_buf_out[2] = (ft[0][1] / ft[0][0]) * signal_buf_in[2]
				+ (ft[1][1] / ft[0][0]) * signal_buf_in[1]
				+ (ft[2][1] / ft[0][0]) * signal_buf_in[0]
				- (ft[1][0] / ft[0][0]) * signal_buf_out[1]
				- (ft[2][0] / ft[0][0]) * signal_buf_out[0];

	} else {
		signal_buf_out[2] = signal;
	}

	return signal_buf_out[2];
}

void volume_envelope(Envelope *vol_env) {

	switch (vol_env->state) {
	case wait:
		if (synth.botao_state == pressionado)
			vol_env->state = attack;

		synth.osc1.wtp_osc = 0;
		synth.osc2.wtp_osc = 0;
		synth.osc3.wtp_osc = 0;
		break;

	case attack:

		if (synth.botao_state == solto)
			vol_env->state = release;

		if (vol_env->output >= 1) {
			vol_env->state = decay;
		}
		vol_env->output = vol_env->output + vol_env->inc_att;
		break;

	case decay:

		if (synth.botao_state == solto)
			vol_env->state = release;

		if (vol_env->output <= vol_env->sus) {
			vol_env->state = sustain;
		}
		vol_env->output = vol_env->output - vol_env->inc_dec;
		break;

	case sustain:

		if (synth.botao_state == solto)
			vol_env->state = release;

		vol_env->output = vol_env->sus;
		break;

	case release:
		if (vol_env->output <= 0) {
			vol_env->state = wait;
			vol_env->output = 0;
		}
		vol_env->output = vol_env->output - vol_env->inc_dec;
		break;

	}
}

void frequency_envelope(Envelope *freq_env) {

	switch (freq_env->state) {
	case wait:
		if (synth.botao_state == pressionado)
			freq_env->state = attack;
		break;

	case attack:

		if (synth.botao_state == solto)
			freq_env->state = release;

		if (freq_env->output >= 1) {
			freq_env->state = decay;
		}
		freq_env->output = freq_env->output + freq_env->inc_att;
		break;

	case decay:

		if (synth.botao_state == solto)
			freq_env->state = release;

		if (freq_env->output <= freq_env->sus) {
			freq_env->state = sustain;
		}
		freq_env->output = freq_env->output - freq_env->inc_dec;
		break;

	case sustain:

		if (synth.botao_state == solto)
			freq_env->state = release;

		freq_env->output = freq_env->sus;
		break;

	case release:
		if (freq_env->output <= 0) {
			freq_env->state = wait;
			freq_env->output = 0;
		}
		freq_env->output = freq_env->output - freq_env->inc_dec;
		break;

	}
}

/* LFO */

float32_t lfo(LFO *lfo) {

	float32_t signal = 1;

	if (lfo->state == enable) {
		signal = osc_ops(&lfo->osc);
	}

	return signal;
	// apos chamar a função convoluir signal*lfo
}

void read_pack(char *pack) {
	const float32_t a_att = 9.1116E-4, b_att = 0.093034, a_dec = 0.0036495,
			b_dec = 0.091685, a_lfo = 0.94789, b_lfo = 0.053518;

	switch ((int8_t) pack[0]) {

	// Oscilador 1
	case 0x01:
		synth.osc1.state = (uint8_t) pack[1] ? enable : disable;
		synth.osc1.ampl = (float32_t) pack[2] / 100.0f;
		synth.osc1.tune = ((float32_t) pack[3] - 5.0f);
		synth.osc1.range = (float32_t) pack[4];
		synth.osc1.wave = (Wave_form) pack[5] - 1;
		break;

		// Oscilador 2
	case 0x02:
		synth.osc2.state = (uint8_t) pack[1] ? enable : disable;
		synth.osc2.ampl = (float32_t) pack[2] / 100.0f;
		synth.osc2.tune = ((float32_t) pack[3] - 5.0f);
		synth.osc2.range = (float32_t) pack[4];
		synth.osc2.wave = (Wave_form) pack[5] - 1;
		break;

		// Oscilador 3
	case 0x03:
		synth.osc3.state = (uint8_t) pack[1] ? enable : disable;
		synth.osc3.ampl = (float32_t) pack[2] / 100.0f;
		synth.osc3.tune = ((float32_t) pack[3] - 5.0f);
		synth.osc3.range = (float32_t) pack[4];
		synth.osc3.wave = (Wave_form) pack[5] - 1;
		break;

		// Noise
	case 0x04:
		synth.noise_state = (uint8_t) pack[1] ? enable : disable;
		synth.noise_ampl = (float32_t) pack[2] / 100.0f;
		break;

		// Filter
	case 0x05:
		synth.lpf1.state = (uint8_t) pack[1] ? enable : disable;
		synth.lpf1.cutoff = ((float32_t) pack[2] - 5.0f);
		synth.lpf1.q = (float32_t) pack[3];
		synth.lpf1.contour = (float32_t) pack[4]/100.0f;
		synth.env2.inc_att = 1
				/ (powf(M_E, pack[5] * b_att) * a_att * 48000.0f);
		synth.env2.inc_dec = 1
				/ (powf(M_E, pack[6] * b_dec) * a_dec * 48000.0f);
		synth.env2.sus = (float32_t) pack[7] / 100.0f;

		break;

		// Envelope de volume
	case 0x06:
		synth.env1.inc_att = 1
				/ (powf(M_E, pack[1] * b_att) * a_att * 48000.0f);
		synth.env1.inc_dec = 1
				/ (powf(M_E, pack[2] * b_dec) * a_dec * 48000.0f);
		synth.env1.sus = (float32_t) pack[3] / 100.0f;
		break;

		// LFO volume
	case 0x07:
		synth.lfo1.state = (uint8_t) pack[1] ? enable : disable;
		synth.lfo1.osc.ampl = (float32_t) pack[2] / 100.0f;
		synth.lfo1.osc.freq = powf(M_E, pack[3] * b_lfo) * a_lfo;
		break;

	case 0x08:
		synth.lfo2.state = (uint8_t) pack[1] ? enable : disable;
		synth.lfo2.osc.ampl = (float32_t) pack[2] / 100.0f;
		synth.lfo2.osc.freq = powf(M_E, pack[3] * b_lfo) * a_lfo;
		break;
	}

}

void processamento(void) {
	float32_t signal = 0, osc_weight = 0.99;

	// Synth
	synth.noise_state = disable;
	synth.noise_ampl = 1;

	// Oscilador 1
	synth.osc1.state = enable;
	synth.osc1.wtp_osc = 0;
	synth.osc1.wtp_inc = 0;
	synth.osc1.tune = 0;
	synth.osc1.range = 2;
	synth.osc1.ampl = 1;
	synth.osc1.wave = square;

	// Oscilador 2
	synth.osc2.state = disable;
	synth.osc2.wtp_osc = 0;
	synth.osc2.wtp_inc = 0;
	synth.osc2.tune = 0;
	synth.osc2.range = 2;
	synth.osc2.ampl = 1;
	synth.osc2.wave = square;

	// Oscilador 3
	synth.osc3.state = disable;
	synth.osc3.wtp_osc = 0;
	synth.osc3.wtp_inc = 0;
	synth.osc3.tune = 0;
	synth.osc3.range = 2;
	synth.osc3.ampl = 1;
	synth.osc3.wave = square;

	// Oscilador LFO volume
	synth.lfo1.osc.wtp_osc = 0;
	synth.lfo1.osc.wtp_inc = 0;
	synth.lfo1.osc.state = enable;
	synth.lfo1.osc.freq = 200;
	synth.lfo1.osc.wave = sine;
	synth.lfo1.osc.ampl = 1;

	// LFO volume;
	synth.lfo1.state = disable;

	// Oscilador LFO filtro
	synth.lfo2.osc.wtp_osc = 0;
	synth.lfo2.osc.wtp_inc = 0;
	synth.lfo2.osc.state = enable;
	synth.lfo2.osc.freq = 200;
	synth.lfo2.osc.wave = sine;
	synth.lfo2.osc.ampl = 1;

	// LFO filtro;
	synth.lfo2.state = disable;

	// Volume envelope
	synth.env1.output = 0;
	synth.env1.sus = 1;
	synth.env1.state = attack;
	synth.env1.inc_att = 1 / (1E-3 * 48000.0f);
	synth.env1.inc_dec = 1 / (4E-3 * 48000.0f);

	// Frequency envelope
	synth.env2.output = 0;
	synth.env2.sus = 1;
	synth.env2.state = attack;
	synth.env2.inc_att = 1 / (1E-3 * 48000.0f);
	synth.env2.inc_dec = 1 / (4E-3 * 48000.0f);

	// LPF
	synth.lpf1.cutoff = 0;
	synth.lpf1.q = 1;
	synth.lpf1.state = disable;
	synth.lpf1.contour = 1;

	synth.pitchi = 4;

	while (1) {

		/* Calcula os parametros que dependem da nota tocada */
		synth.osc1.freq = pitch_base[synth.pitchi] * synth.osc1.range
				* powf(2, synth.osc1.tune / 6);
		synth.osc2.freq = pitch_base[synth.pitchi] * synth.osc2.range
				* powf(2, synth.osc2.tune / 6);
		synth.osc3.freq = pitch_base[synth.pitchi] * synth.osc3.range
				* powf(2, synth.osc3.tune / 6);
		synth.lpf1.fc = pitch_base[synth.pitchi] * 2
				* powf(2, synth.lpf1.cutoff) * synth.osc1.range
				* powf(2, synth.osc1.tune / 6);

		/* Calcula o peso da amplitude dos osciladores ativos */
		switch ((uint8_t) synth.osc1.state + (uint8_t) synth.osc2.state
				+ (uint8_t) synth.osc3.state) {
		case 1:
			osc_weight = 0.99;
			break;

		case 2:
			osc_weight = 0.66;
			break;

		case 3:
			osc_weight = 0.33;
			break;
		}

		signal = osc_weight
				* (osc_ops(&synth.osc1) + osc_ops(&synth.osc2)
						+ osc_ops(&synth.osc3)) + noise(&synth);

		shift_signal_buf(signal_buf_in, signal);

		signal = lpf(synth.lpf1, signal);

		shift_signal_buf(signal_buf_out, signal);

		volume_envelope(&synth.env1);
		signal = signal * synth.env1.output * lfo(&synth.lfo1);

		if (iBuff_mono < BLOCK_SIZE) {

			buf_mono[iBuff_mono] = signal * 0.5; // o 0.5 evita que a amplitude da saída sature

			iBuff_mono++;

		} else {
			iBuff_mono = 0;

			UDA1380_SendSamplesFloatMono(buf_mono, BLOCK_SIZE);
		}
	}
}

int main(void) {
	TM_RNG_Init();

	configura_usart2();

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	EXTI_conf();
	NVIC_conf();

	Inicia_Pino(GPIOC, GPIO_Pin_1, GPIO_Mode_IN, GPIO_Speed_50MHz,
			GPIO_PuPd_NOPULL, RCC_AHB1Periph_GPIOC);

	Inicia_Pino(GPIOC, GPIO_Pin_2, GPIO_Mode_IN, GPIO_Speed_50MHz,
			GPIO_PuPd_NOPULL, RCC_AHB1Periph_GPIOC);

	Inicia_Pino(GPIOD, GPIO_Pin_3, GPIO_Mode_IN, GPIO_Speed_50MHz,
			GPIO_PuPd_NOPULL, RCC_AHB1Periph_GPIOD);

	Inicia_Pino(GPIOE, GPIO_Pin_4, GPIO_Mode_IN, GPIO_Speed_50MHz,
			GPIO_PuPd_NOPULL, RCC_AHB1Periph_GPIOE);

	Inicia_Pino(GPIOE, GPIO_Pin_5, GPIO_Mode_IN, GPIO_Speed_50MHz,
			GPIO_PuPd_NOPULL, RCC_AHB1Periph_GPIOE);

	Inicia_Pino(GPIOE, GPIO_Pin_6, GPIO_Mode_IN, GPIO_Speed_50MHz,
			GPIO_PuPd_NOPULL, RCC_AHB1Periph_GPIOE);

	Inicia_Pino(GPIOE, GPIO_Pin_7, GPIO_Mode_IN, GPIO_Speed_50MHz,
			GPIO_PuPd_NOPULL, RCC_AHB1Periph_GPIOE);

	Inicia_Pino(GPIOE, GPIO_Pin_8, GPIO_Mode_IN, GPIO_Speed_50MHz,
			GPIO_PuPd_NOPULL, RCC_AHB1Periph_GPIOE);

	UDA1380_InitTypeDef UDA1380_InitStructure;

	UDA1380_StructInit(&UDA1380_InitStructure);
	UDA1380_InitStructure.UDA1380_Queue_Length = QUEUE_LEN;
	UDA1380_InitStructure.UDA1380_Buffer_In = buf_in;
	UDA1380_InitStructure.UDA1380_Buffer_Out = buf_out;
	UDA1380_InitStructure.UDA1380_Callback = processamento;
	UDA1380_InitStructure.UDA1380_DAC_Enable = ENABLE;
	UDA1380_InitStructure.UDA1380_ADC_Enable = DISABLE;
	UDA1380_Init(&UDA1380_InitStructure);

	while (1)
		;
}

void USART3_IRQHandler(void) {
	static char data_string[50], usart_data;
	static uint8_t i = 0, usart_flag = 0;

	usart_data = USART_ReceiveData(USART3);

	if (usart_data == 0xFF) {
		usart_flag = 1;
	} else if (usart_flag == 1) {
		if (usart_data != 0xAA) {
			data_string[i] = usart_data;
			i++;
		} else {
			data_string[i] = '\0';
			i = 0;
			usart_flag = 0;

			read_pack(data_string);
		}
	}

	USART_ClearFlag(USART3, USART_IT_RXNE);
}

void EXTI1_IRQHandler(void) {

	if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1) == solto) {
		synth.botao_state = solto;
	} else if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1) == pressionado) {
		synth.pitchi = 0;
		synth.botao_state = pressionado;
	}

	EXTI_ClearITPendingBit(EXTI_Line1);
}

void EXTI2_IRQHandler(void) {

	if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == solto) {
		synth.botao_state = solto;
	} else if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == pressionado) {
		synth.pitchi = 1;
		synth.botao_state = pressionado;
	}

	EXTI_ClearITPendingBit(EXTI_Line2);
}

void EXTI3_IRQHandler(void) {

	if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_3) == solto) {
		synth.botao_state = solto;
	} else if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_3) == pressionado) {
		synth.pitchi = 2;
		synth.botao_state = pressionado;
	}

	EXTI_ClearITPendingBit(EXTI_Line3);
}

void EXTI4_IRQHandler(void) {

	if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == solto) {
		synth.botao_state = solto;
	} else if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == pressionado) {
		synth.pitchi = 3;
		synth.botao_state = pressionado;
	}

	EXTI_ClearITPendingBit(EXTI_Line4);
}

//funcionando

void EXTI9_5_IRQHandler(void) {

	if (EXTI_GetITStatus(EXTI_Line5) != RESET) {
		if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5) == solto) {
			synth.botao_state = solto;

		} else if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5) == pressionado) {
			synth.pitchi = 4;
			synth.botao_state = pressionado;
		}
		EXTI_ClearITPendingBit(EXTI_Line5);

	}
	if (EXTI_GetITStatus(EXTI_Line6) != RESET) {

		if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_6) == solto) {
			synth.botao_state = solto;

		} else if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_6) == pressionado) {
			synth.pitchi = 5;
			synth.botao_state = pressionado;
		}
		EXTI_ClearITPendingBit(EXTI_Line6);

	}
	if (EXTI_GetITStatus(EXTI_Line7) != RESET) {

		if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_7) == solto) {
			synth.botao_state = solto;

		} else if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_7) == pressionado) {
			synth.pitchi = 6;
			synth.botao_state = pressionado;
		}
		EXTI_ClearITPendingBit(EXTI_Line7);

	}
	if (EXTI_GetITStatus(EXTI_Line8) != RESET) {

		if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_8) == solto) {
			synth.botao_state = solto;

		} else if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_8) == pressionado) {
			synth.pitchi = 7;
			synth.botao_state = pressionado;
		}
		EXTI_ClearITPendingBit(EXTI_Line8);

	}
}
