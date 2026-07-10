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
// File : serial.c
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

#include <stdint.h>
#include <limits.h>

#include "serial.h"

void serial_init(serial_gen * ctx, int bitcnt, unsigned int sample_rate, unsigned int baud_rate)
{
	ctx->bits_per_word = bitcnt;
	ctx->start_bit_cnt = 0;
	ctx->stop_bit_cnt = 0;
	ctx->lsb_first = 0;

	ctx->mask = 0;

	ctx->sample_rate = sample_rate;

	ctx->inc_acc = (uint32_t)( ( (uint64_t)(0xFFFFFFFF) * (uint64_t)baud_rate ) / (uint64_t)sample_rate );

	ctx->acc = 0;
	ctx->cur_bit = 0;
}

int serial_tx_getlinestate(serial_gen * ctx)
{
	int update_flag;

	update_flag = 0;

	if ( ctx->acc > (((uint32_t)0xFFFFFFFF) - ctx->inc_acc) )
	{
		// Overflow... Next bit
		ctx->cur_bit = ctx->tx_reg & ctx->mask;
		// If mask == 0 -> shifter empty !
		ctx->mask >>= 1;

		update_flag = 0x2;
	}

	ctx->acc += ctx->inc_acc;

	if( ctx->cur_bit )
		return 1 | update_flag;
	else
		return 0 | update_flag;

}

int serial_is_tx_reg_empty(serial_gen * ctx)
{
	if(!ctx->mask)
		return 1;
	else
		return 0;
}

void serial_tx_settxreg(serial_gen * ctx,uint32_t data)
{
	ctx->mask = (0x1 << (ctx->bits_per_word - 1));
	ctx->tx_reg = data;
}

