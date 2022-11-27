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
// File : rds.h
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

#define RDS_BIT_RATE 1187.5

typedef struct _rds_stat
{
	int sample_rate;
	float bit_rate;

	double old_pilot_phase;
	double phase;

	int current_data_state;
	int current_diff_dat_state;
	int ser_data_in;

	int test;
	int cycles_count;

	char station_name[256+1];
	int  station_name_index;

	uint32_t current_bloc;
	int current_bloc_index;

	int current_rds_code_index;

}rds_stat;

void init_rds_encoder(rds_stat * stat,int sample_rate);
double get_rds_bit_state(rds_stat * stat, double pilot_phase);
void set_rds_text(rds_stat * stat,char * text);
