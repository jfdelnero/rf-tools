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
// File : rand_gen.c
// Contains: random generators
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

#include "rand_gen.h"

uint8_t rand_gen_get_next_byte(rand_gen_state *state)
{
	uint8_t retbyte;
	uint32_t seed;
	uint32_t byte_number;

	seed = state->current_seed;
	byte_number = state->byte_number;

	if( byte_number > 3 )
	{
		byte_number = 0x00;

		/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;

		state->current_seed = seed;
	}

	retbyte = ( seed >> ( (3 - byte_number) << 3) ) & 0xFF;

	byte_number++;

	state->byte_number = byte_number;

	return retbyte;
}

uint32_t rand_gen_get_next_word(rand_gen_state *state)
{
	uint32_t retword;
	uint32_t seed;

	seed = state->current_seed;

	retword = seed;

	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	seed ^= seed << 13;
	seed ^= seed >> 17;
	seed ^= seed << 5;

	state->byte_number = 0x00;
	state->current_seed = seed;

	return retword;
}

void rand_gen_init(rand_gen_state *state, uint32_t seed )
{
	if(!state)
		return;

	if( seed )
	{
		state->current_seed = seed;
		state->byte_number = 0;
	}
	else
	{
		state->current_seed = 0x12E6C816;
		state->byte_number = 0;
	}
}

