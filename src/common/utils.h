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
// File : utils.h
// Contains: misc / utils functions
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

void get_filename(char * path,char * filename);
void printbuf(unsigned char * buf,int size);
int is_printable_char(unsigned char c);


#ifdef BIGENDIAN_HOST

// big endian
#define BIGENDIAN_WORD(wValue) (wValue)
#define BIGENDIAN_DWORD(dwValue) (dwValue)

// little endian

#define LITTLEENDIAN_WORD(wValue) ( (((wValue&0xFF)<<8)) | (((wValue)>>8)&0xFF) )
#define LITTLEENDIAN_DWORD(dwValue) ( (((dwValue&0xFF)<<24)) | (((dwValue&0xFF00)<<8)) | (((dwValue)>>8)&0xFF00) | (((dwValue)>>24)&0xFF) )

#else

//LITTLE ENDIAN HOST

// big endian
#define BIGENDIAN_WORD(wValue) ( (((wValue)<<8)&0xFF00) | (((wValue)>>8)&0xFF) )
#define BIGENDIAN_DWORD(dwValue) ( (((dwValue&0xFF)<<24)) | (((dwValue&0xFF00)<<8)) | (((dwValue)>>8)&0xFF00) | (((dwValue)>>24)&0xFF) )

// little endian

#define LITTLEENDIAN_WORD(wValue) (wValue)
#define LITTLEENDIAN_DWORD(dwValue) (dwValue)

#endif

extern const unsigned char LUT_ByteBitsInverter[];
extern const unsigned char LUT_QuartetBitsInverter[];