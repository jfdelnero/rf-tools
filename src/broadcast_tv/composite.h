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
// File : composite.h
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

typedef struct composite_state_
{
	double sample_period;

	int buf_x_res;
	int buf_y_res;

	uint32_t * video_buffer;

	int cur_line_index;
	int step_index;
	int repeat_cnt;

	double cur_state_time;

}composite_state;

typedef struct pulses_state_
{
	int    repeat;
	double start_val;
	double end_val;
	double first_duration;
	double total_duration;
	int    type;
}pulses_state;


void init_composite(composite_state * state, int sample_rate, int x_res, int y_res, uint32_t * bmp);
void gen_video_signal(composite_state * state, double * vid_signal, int buf_size);
