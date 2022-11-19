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
// File : rand_gen.h
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

#ifndef __RAND_GEN_H__
#define __RAND_GEN_H__

typedef struct _rand_gen_state
{
	uint32_t current_seed;
	int      byte_number;
} rand_gen_state;

void     rand_gen_init(rand_gen_state *state, uint32_t seed );
uint8_t  rand_gen_get_next_byte(rand_gen_state *state);
uint32_t rand_gen_get_next_word(rand_gen_state *state);

#endif /* __RAND_GEN_H__ */

