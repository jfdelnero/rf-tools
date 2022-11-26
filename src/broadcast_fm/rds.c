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
#include "rds_stream_dump.h"

const uint16_t crc_group_xor[]=
{
	0xFC,  // A
	0x198, // B
	0x168, // C
	0x1B4, // D
	0x350  // C'
};

const uint16_t rds_codes[]=
{
	0xF849,
	0x0CE8,
	0xF849,
	0x0000  // Char code.
};

void init_rds_encoder(rds_stat * stat,int sample_rate)
{
	memset(stat,0,sizeof(rds_stat));
	stat->bit_rate = RDS_BIT_RATE;
	stat->old_pilot_phase = 0;
	stat->sample_rate = sample_rate;
	strcpy(stat->station_name,"- RDS - - RDS - - RDS - - RDS - --TEST----TEST----TEST----TEST----TEST----TEST--\n");
}

int rds_differential_encoder(rds_stat * stat)
{
	stat->current_diff_dat_state = ((stat->ser_data_in & 1) ^ stat->current_diff_dat_state);

	return stat->current_diff_dat_state;
}

uint32_t rds_crc (uint16_t data)
{
	int i;
	uint16_t crc = 0;
	int crc_msb;

	for(i=0; i<16; i++)
	{
		crc_msb = (crc >> (10-1)) & 1;

		crc <<= 1;
		if( crc_msb ^ (data >> 15 ) )
		{
			crc ^= 0x1B9;
		}

		data <<= 1;
	}

	return crc & 0x3FF;
}

uint32_t encode_rds_bloc(uint16_t data, int group)
{
	uint32_t word;

	word = ((uint32_t)data)<<10;
	word |= (rds_crc (data) ^ crc_group_xor[group]);

	//printf("rds(%d): dat:0x%.4X grp:0x%.4X 0x%.4X\n",group,data,word&0x3FF,word);

	return word;
}

int get_next_rds_bit(rds_stat * stat)
{
	int dat;
	uint16_t rdsc;

#ifdef USE_RDS_STREAM_BIT
	if(!rds_stream_dump[stat->current_rds_code_index])
	{
		stat->current_rds_code_index = 0;
		dat = rds_stream_dump[stat->current_rds_code_index];
	}
	else
		dat = rds_stream_dump[stat->current_rds_code_index];

	stat->current_rds_code_index++;

	return dat;
#endif

	stat->current_bloc_index--;

	if(stat->current_bloc_index<0)
	{
		// prepare next bloc

#ifdef USE_RDS_STREAM
		rdsc = rds_dump[stat->current_rds_code_index];
		stat->current_bloc = encode_rds_bloc(rdsc, stat->current_rds_code_index&3 );
		stat->current_rds_code_index++;

		if(stat->current_rds_code_index >= 327*4 )
		{
			stat->current_rds_code_index = 0;
		}
#else
		rdsc = rds_codes[stat->current_rds_code_index];
		if(rdsc)
		{
			if( stat->current_rds_code_index == 1)
			{
				stat->current_bloc = encode_rds_bloc(rdsc | (stat->station_name_index>>1),stat->current_rds_code_index );
				stat->current_rds_code_index++;
			}
			else
			{
				stat->current_bloc = encode_rds_bloc(rdsc,stat->current_rds_code_index);
				stat->current_rds_code_index++;
			}
		}
		else
		{
			rdsc = stat->station_name[stat->station_name_index++];
			rdsc <<= 8;
			rdsc |= stat->station_name[stat->station_name_index++];

			stat->current_bloc = encode_rds_bloc(rdsc,stat->current_rds_code_index);

			if(!stat->station_name[stat->station_name_index])
			{
				stat->station_name_index = 0;
			}

			stat->current_rds_code_index = 0;
		}
#endif

		stat->current_bloc_index = 25;
	}

	if( (stat->current_bloc >> (stat->current_bloc_index)) & 0x1 )
	{
		dat = 1;
	}
	else
	{
		dat = 0;
	}

	//printf("%d",dat);

	return dat;
}

double get_rds_bit_state(rds_stat * stat, double pilot_phase)
{
	double sample;
	double phase_diff;

	if(pilot_phase < stat->old_pilot_phase)
		phase_diff = ( 2.0 * PI - stat->old_pilot_phase) + pilot_phase;
	else
		phase_diff = pilot_phase - stat->old_pilot_phase;

	if( phase_diff >= ( ( 2.0 * PI ) / 3.0 ) ) // 19KHz period / 3 -> 57KHz
	{
		// New 57KHz tick clock
		stat->old_pilot_phase += ( ( 2.0 * PI ) / 3.0 );

		if(stat->old_pilot_phase >= ( 2.0 * PI ))
			stat->old_pilot_phase -= ( 2.0 * PI );

		stat->cycles_count++;
		if(stat->cycles_count >= 48)
		{
			stat->test++;

			stat->ser_data_in = get_next_rds_bit(stat);

			rds_differential_encoder(stat);

			stat->current_data_state = (stat->current_diff_dat_state & 0x1);

			stat->cycles_count = 0;
		}

		// Delayed signal
		if(stat->cycles_count == 24)
		{
			stat->current_data_state = stat->current_diff_dat_state ^ 1;
		}
	}

	#define FREQ_FILTER (1187.5*0.50)

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

	sample = ( cos( stat->phase ) * ( 5.0 ) );

	return sample;
}
