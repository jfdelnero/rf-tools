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
// File : rds.c
// Contains: rds engine
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
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "modulator.h"
#include "rds.h"

void init_rds_encoder(rds_stat * stat,int sample_rate)
{
	memset(stat,0,sizeof(rds_stat));
	stat->bit_rate = RDS_BIT_RATE;
	stat->old_pilot_phase = 0;
	stat->sample_rate = sample_rate;
}

int rds_differential_encoder(rds_stat * stat)
{
	stat->current_diff_dat_state = ((stat->ser_data_in & 1) ^ stat->current_diff_dat_state);

	return stat->current_diff_dat_state;
}

double get_rds_bit_state(rds_stat * stat, double pilot_phase)
{
	double sample;
	double phase_diff;

	if(pilot_phase < stat->old_pilot_phase)
		phase_diff = (2*PI + pilot_phase) - stat->old_pilot_phase;
	else
		phase_diff = pilot_phase - stat->old_pilot_phase;

	if( phase_diff >= ((2*PI) / 3) )
	{
		// New 57KHz tick clock
		stat->old_pilot_phase = pilot_phase;//+= ((2*PI) / 3);

		if(stat->old_pilot_phase >= (2*PI))
			stat->old_pilot_phase = 0;

		stat->cycles_count++;
		if(stat->cycles_count >= 16*2)
		{
			stat->test++;

			stat->ser_data_in = rand()&1;

			rds_differential_encoder(stat);

			stat->current_data_state = (stat->current_diff_dat_state & 0x1);

			stat->cycles_count = 0;
		}

		// Delayed signal
		if(stat->cycles_count == 8*2)
		{
			stat->current_data_state = stat->current_diff_dat_state ^ 1;
		}
	}

	#define FREQ_FILTER (1187.5*1.0)

	if(stat->current_data_state)
	{
		// Must go to 0.
		if( !((stat->phase >= ((2*PI) - ((2.0 * PI * (FREQ_FILTER*2))/ (double)stat->sample_rate) ) ) ||  // >=// 2PI - step
			(stat->phase < (((2.0 * PI * (FREQ_FILTER*2))/ (double)stat->sample_rate) ) ) )              // < // step
		)
		{
			stat->phase += ( (2.0 * PI * (FREQ_FILTER*2) ) / (double)stat->sample_rate );
			if(stat->phase >= 2.0 * PI)
				stat->phase -= (2.0 * PI);
		}
	}
	else
	{
		// Must go to PI.
		if( !((stat->phase >= ((PI) - ((2.0 * PI * (FREQ_FILTER*2))/ (double)stat->sample_rate) ) ) &&   // > PI - step
			(stat->phase < ((PI) + ((2.0 * PI * (FREQ_FILTER*2))/ (double)stat->sample_rate) ) ))        // < PI + step
		)
		{
			stat->phase += ( (2.0 * PI * (FREQ_FILTER*2) ) / (double)stat->sample_rate );
			if(stat->phase >= 2.0 * PI)
				stat->phase -= (2.0 * PI);
		}
	}

	sample = ( cos( stat->phase ) * 3.0 );

	return sample;
}
