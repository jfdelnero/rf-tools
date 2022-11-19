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
// File : modulator.h
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

#ifndef __MODULATOR_H__
#define __MODULATOR_H__

#if defined(M_PI)
#define PI M_PI
#else
#define PI 3.1415926535897932384626433832795
#endif

typedef struct iq_wave_gen_
{
	double phase;

	double Frequency,OldFrequency;

	int    sample_rate;
	double Amplitude;
}iq_wave_gen;

typedef struct wave_gen_
{
	double phase;

	double Frequency,OldFrequency;

	int    sample_rate;
	double Amplitude;
}wave_gen;

int find_zero_phase_index(short * buf, int size);
double find_phase(iq_wave_gen * wg, double sinoffset,short samplevalue,short * buf, int size);
int Fill_IQ_Wave_Buffer(iq_wave_gen * wg,unsigned short * wave_buf, int Size);
uint16_t get_next_iq(iq_wave_gen * wg);

int get_next_sample(wave_gen * wg);

double f_get_next_sample(wave_gen * wg);

#endif
