///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : modulator.c
// Contains: oscillator and IQ modulator
//
// This file is part of rf-tools.
//
// Written by: Jean-François DEL NERO
//
// Copyright (C) 2022 Jean-François DEL NERO
//
// You are free to do what you want with this code.
// A credit is always appreciated if you use it into your product :)
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <math.h>

#include "modulator.h"

//
// Note about the IQ
//
// If NCO enabled and If I="cos", Q="sin"     -> Output Freq = NCO Freq + IQ Freq
// If NCO enabled and If I="sin", Q="cos"     -> Output Freq = NCO Freq - IQ Freq
// If NCO enabled and If I="cos", Q="cos"     -> Output Freq = NCO Freq - IQ Freq AND NCO Freq + IQ Freq
// If NCO enabled and If I="const", Q="sin"   -> Output Freq = NCO Freq - IQ Freq AND NCO Freq AND NCO Freq + IQ Freq
// If NCO enabled and If I="const", Q="const" -> Output Freq = NCO Freq
//

uint16_t get_next_iq(iq_wave_gen * wg)
{
	int i_sample,q_sample;

	char i_ub_sample,q_ub_sample;

	i_sample = (int)(cos(wg->phase)*wg->Amplitude);
	q_sample = (int)(sin(wg->phase)*wg->Amplitude);

	wg->phase += ( (2.0 * PI * wg->Frequency) / (double)wg->sample_rate );

/*	if(wg->phase > (2.0 * PI) )
		wg->phase -= (2.0 * PI);

	if(wg->phase < (-2.0 * PI) )
		wg->phase += (2.0 * PI);
	*/

	i_ub_sample = (i_sample);
	q_ub_sample = (q_sample);

	return (unsigned short)((i_ub_sample)&0xFF) | ((((unsigned short)q_ub_sample)<<8)&0xFF00);
}

//
// 2*PI -> One cycle.
// Phase step = ( (2 * PI) / (SAMPLE_RATE / Frequency) ) -> ( (2 * PI) * Frequency ) / SAMPLE_RATE
//

double f_get_next_sample(wave_gen * wg)
{
	double sample;

	sample = ( cos( wg->phase ) * wg->Amplitude );

	wg->phase += ( (2.0 * PI * wg->Frequency) / (double)wg->sample_rate );

	if(wg->phase > (2.0 * PI) )
		wg->phase -= (2.0 * PI);

	if(wg->phase < (-2.0 * PI) )
		wg->phase += (2.0 * PI);

	return sample;
}
