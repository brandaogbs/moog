#ifndef __SINTETIZADOR

#define __SINTETIZADOR

#include "math.h"
#include "uda1380.h"
#include "arm_math.h"
#include "fdacoefs.h"
#include "fdacoefs2.h"
#include "wavetable.h"

// Tipos de onda dos osciladores
typedef enum {
	square = 0, triangle, sawtooth, distosine, sine, cosine
} Wave_form;

// Oscilador
typedef struct {
	float32_t wtp_osc;
	float32_t wtp_inc;
	Wave_form wave;
} Oscillator;

float32_t osc_ops(Oscillator *osc);

#endif
