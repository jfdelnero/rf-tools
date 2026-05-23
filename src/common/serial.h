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
// File : serial.h
// Contains: serial helpers
//
// This file is part of rf-tools.
//
// Written by: Jean-François DEL NERO
//
// Copyright (C) 2026 Jean-François DEL NERO
//
// You are free to do what you want with this code.
// A credit is always appreciated if you use it into your product :)
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#ifndef __SERIAL_H__
#define __SERIAL_H__

typedef struct serial_gen_
{
	int bits_per_word;
	int start_bit_cnt;
	int stop_bit_cnt;
	int lsb_first;

	uint32_t sample_rate;
	uint32_t baud_rate;

	uint32_t mask;
	uint32_t tx_reg;

	uint32_t acc;
	uint32_t inc_acc;
	uint32_t cur_bit;
}serial_gen;

void serial_init(serial_gen * ctx, int bitcnt, unsigned int sample_rate, unsigned int baud_rate);
int  serial_tx_getlinestate(serial_gen * ctx);
int  serial_is_tx_reg_empty(serial_gen * ctx);
void serial_tx_settxreg(serial_gen * ctx,uint32_t data);

#endif

