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
#define ASSERTED_LEVEL 0
#define DEASSERTED_LEVEL 40
#define COMPOSITE_LINE_PERIOD_US 64

// PAL / Secam : 768 x 576
// NTSC : 720 x 480 (Old : 640 x 480)

// PAL-L timings
pulses_state vertical_blanking[]=
{
	// First sequence of equalizing pulses
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, EQUALIZING_PULSE_LEN/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},
	// Sequence of synchronizing pulses (Line 1 count)
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, 27.3/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},
	// Duration of second sequence of equalizing pulses
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, EQUALIZING_PULSE_LEN/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},

	// Blank Lines (6 > 23)
	{18, ASSERTED_LEVEL,   DEASSERTED_LEVEL, LINE_PULSE_LEN/(double)1E6   ,((COMPOSITE_LINE_PERIOD_US)/(double)1E6),0},

	// Lines (24 > 310)
	{287, ASSERTED_LEVEL,   DEASSERTED_LEVEL, LINE_PULSE_LEN/(double)1E6   ,((COMPOSITE_LINE_PERIOD_US)/(double)1E6),1},

	// First sequence of equalizing pulses
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, EQUALIZING_PULSE_LEN/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},
	// Sequence of synchronizing pulses (Line 1 count)
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, 27.3/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},
	// Duration of second sequence of equalizing pulses
	{  5, ASSERTED_LEVEL,   DEASSERTED_LEVEL, EQUALIZING_PULSE_LEN/(double)1E6  ,((COMPOSITE_LINE_PERIOD_US/2)/(double)1E6),0},

	// -- end of line 318
	{  1, DEASSERTED_LEVEL, DEASSERTED_LEVEL, 0 ,(COMPOSITE_LINE_PERIOD_US/2)/(double)1E6,0},

	// Blank Lines (319 > 335)
	{17, ASSERTED_LEVEL,   DEASSERTED_LEVEL, LINE_PULSE_LEN/(double)1E6   ,(COMPOSITE_LINE_PERIOD_US)/(double)1E6,0},

	// Lines (336 > 622)
	{287, ASSERTED_LEVEL,   DEASSERTED_LEVEL, LINE_PULSE_LEN/(double)1E6   ,(COMPOSITE_LINE_PERIOD_US)/(double)1E6,3},

	// Line 623 - (half)
	{  1, ASSERTED_LEVEL,   DEASSERTED_LEVEL, LINE_PULSE_LEN/(double)1E6   ,((COMPOSITE_LINE_PERIOD_US)/2)/(double)1E6,0},

	// End
	{  0, 0, 0, 0  ,0,0}
};


void init_composite(composite_state * state, int sample_rate, int x_res, int y_res, uint32_t * bmp)
{
	int i;
	state->sample_period = (double)1 / (double)sample_rate;

	state->buf_x_res = x_res;
	state->buf_y_res = y_res;

	state->video_buffer = bmp;

	if(!state->video_buffer)
	{
		state->video_buffer = malloc( x_res * y_res * sizeof(uint32_t) );
		if(state->video_buffer)
		{
			memset(state->video_buffer,0,x_res * y_res * sizeof(uint32_t) );

			// Basic Mire test init

			for(i=0;i<x_res;i++)
				state->video_buffer[i] = 0xFFFFFF;

			for(i=0;i<x_res;i++)
				state->video_buffer[((y_res-1)*x_res) + i] = 0xFFFFFF;

			for(i=0;i<y_res;i++)
				state->video_buffer[((i)*x_res)] = 0xFFFFFF;

			for(i=0;i<y_res;i++)
				state->video_buffer[((i)*x_res) + (x_res - 1)] = 0xFFFFFF;

			for(i=0;i<x_res/2;i++)
			{
				state->video_buffer[(x_res/4) + (((y_res/4)-1)*x_res) + i] = 0xFFFFFF;
				state->video_buffer[(x_res/4) + (((y_res/4)-1)*x_res) + i + x_res] = 0xFFFFFF;

				state->video_buffer[(x_res/4) + (((y_res - (y_res/4))-1)*x_res) + i] = 0xFFFFFF;
				state->video_buffer[(x_res/4) + (((y_res - (y_res/4))-1)*x_res) + i + x_res] = 0xFFFFFF;

				state->video_buffer[(x_res/4) + (((y_res - (y_res/2))-1)*x_res) + i] = 0x6F6F6F;
				state->video_buffer[(x_res/4) + (((y_res - (y_res/2))-1)*x_res) + i + x_res] = 0x6F6F6F;
			}

			for(i=y_res/4;i<(y_res - (y_res/4));i++)
			{
				state->video_buffer[(x_res/4) + ((i)*x_res)] = 0xFFFFFF;
				state->video_buffer[(x_res/4) + ((i)*x_res) + (x_res/4)] = 0x6F6F6F;
				state->video_buffer[(x_res/2) + ((i)*x_res) + (x_res/4)] = 0xFFFFFF;
			}
		}
	}

	// RGB to Y table.
	for(i=0;i<256;i++)
	{
		state->r_yconv[i] = (double)i * 0.299;
		state->g_yconv[i] = (double)i * 0.587;
		state->b_yconv[i] = (double)i * 0.114;

		state->uconv[i]   = (double)i * 0.492;
		state->vconv[i]   = (double)i * 0.877;
	}

	state->step_index = 0;
	state->cur_state_time = 0;
}

void gen_video_signal(composite_state * state, double * vid_signal, int buf_size)
{
	int i,xpos;
	double value,yvalue,uvalue,vvalue;
	uint32_t rgb_word;

	value = 0;
	i = 0;
	while(i < buf_size)
	{
		if( state->cur_state_time >= vertical_blanking[state->step_index].total_duration)
		{
			state->cur_state_time = 0;
			state->repeat_cnt++;

			if(vertical_blanking[state->step_index].type & 1)
				state->cur_line_index++;
			else
				state->cur_line_index = 0;

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

		if(vertical_blanking[state->step_index].type & 1)
		{
			if(state->cur_state_time >= vertical_blanking[state->step_index].first_duration)
			{
				if(state->cur_state_time >= vertical_blanking[state->step_index].first_duration + ((3.5+4.7)/(double)1E6))
				{
					// Stream pixels line
					if(state->cur_state_time <= vertical_blanking[state->step_index].first_duration + ((3.5+4.7)/(double)1E6) + (45.6/(double)1E6) )
					{
						xpos = ((state->cur_state_time - ( vertical_blanking[state->step_index].first_duration + ((3.5+4.7)/(double)1E6) )) / (45.6/(double)1E6)) * state->buf_x_res;

						value = vertical_blanking[state->step_index].end_val;

						rgb_word = state->video_buffer[(state->cur_line_index * state->buf_x_res * 2) + ( state->buf_x_res * (((vertical_blanking[state->step_index].type>>1)^1)&1)) + xpos];

						// Y / Luminance
						yvalue = state->r_yconv[rgb_word & 0xFF] + state->g_yconv[( rgb_word >> 8 )& 0xFF] + state->b_yconv[( rgb_word >> 16 )& 0xFF];
						// UV
						uvalue = (double)((( rgb_word >> 16 )& 0xFF) - yvalue)*0.492;
						vvalue = (double)(( rgb_word & 0xFF) - yvalue)*0.877;

						value += (double)( yvalue * ((double)((double)100.0 - vertical_blanking[state->step_index].end_val)/(double)256));
					}
					else
					{
						value = vertical_blanking[state->step_index].end_val;
					}
				}
				else
				{
					// TODO : Color Burst
					value = vertical_blanking[state->step_index].end_val;
				}
			}
		}
		else
			state->cur_line_index = 0;

		vid_signal[i] = value;

		state->cur_state_time += state->sample_period;
		i++;
	}
}
