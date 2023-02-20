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
// File : composite.c
// Contains: Composite video signal generator
//
// This file is part of rf-tools.
//
// Written by: Jean-François DEL NERO
//
// Copyright (C) 2023 Jean-François DEL NERO
//
// You are free to do what you want with this code.
// A credit is always appreciated if you use it into your product :)
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "composite.h"

#define EQUALIZING_PULSE_LEN 2.35
#define LINE_PULSE_LEN 4.7
#define ASSERTED_LEVEL 10
#define DEASSERTED_LEVEL 100

// PAL timings
pulses_state vertical_blanking[]=
{
	// First sequence of equalizing pulses
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, EQUALIZING_PULSE_LEN/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},
	// Sequence of synchronizing pulses (Line 1 count)
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, 27.3/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},
	// Duration of second sequence of equalizing pulses
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, EQUALIZING_PULSE_LEN/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},

	// Lines (6 > 310)
	{305, ASSERTED_LEVEL,   DEASSERTED_LEVEL, LINE_PULSE_LEN/(double)1E6   ,((COMPOSITE_LINE_PERIOD_US)/(double)1E6),0},

	// First sequence of equalizing pulses
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, EQUALIZING_PULSE_LEN/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},
	// Sequence of synchronizing pulses (Line 1 count)
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, 27.3/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},
	// Duration of second sequence of equalizing pulses
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, EQUALIZING_PULSE_LEN/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},

	// -- end of line 318
	{  1, DEASSERTED_LEVEL, DEASSERTED_LEVEL, 0 ,(COMPOSITE_LINE_PERIOD_US/2)/(double)1E6,0},

	// Lines (319 > 622)
	{304, ASSERTED_LEVEL,   DEASSERTED_LEVEL, LINE_PULSE_LEN/(double)1E6   ,(COMPOSITE_LINE_PERIOD_US)/(double)1E6,0},

	// Line 623 - (half)
	{  1, ASSERTED_LEVEL,   DEASSERTED_LEVEL, LINE_PULSE_LEN/(double)1E6   ,((COMPOSITE_LINE_PERIOD_US)/2)/(double)1E6,0},

	// End
	{  0, 0, 0, 0  ,0,0}
};


void init_composite(composite_state * state, int sample_rate, int x_res, int y_res)
{
	state->sample_period = (double)1 / (double)sample_rate;

	state->buf_x_res = x_res;
	state->buf_y_res = y_res;

	state->video_buffer = malloc( x_res * y_res * sizeof(uint32_t) );
	if(state->video_buffer)
		memset(state->video_buffer,0,x_res * y_res * sizeof(uint32_t) );

	state->step_index = 0;
	state->cur_state_time = 0;
}

void gen_video_signal(composite_state * state, double * vid_signal, int buf_size)
{
	int i;
	double value;

	value = 0;
	i = 0;
	while(i < buf_size)
	{
		if( state->cur_state_time >= vertical_blanking[state->step_index].total_duration)
		{
			state->cur_state_time = 0;
			state->repeat_cnt++;

			if( state->repeat_cnt >= vertical_blanking[state->step_index].repeat )
			{
				state->repeat_cnt = 0;
				state->step_index++;
			}

			if(!vertical_blanking[state->step_index].repeat)
			{
				state->step_index = 0;
			}
		}

		if( state->cur_state_time >= vertical_blanking[state->step_index].first_duration )
		{
			value = vertical_blanking[state->step_index].end_val;
		}
		else
		{
			value = vertical_blanking[state->step_index].start_val;
		}

		vid_signal[i] = value;

		state->cur_state_time += state->sample_period;
		i++;
	}
}
