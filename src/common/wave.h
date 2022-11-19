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
// File : wave.h
// Contains: wave file helpers
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

#ifndef __WAVE_H__
#define __WAVE_H__

#pragma pack(1)

typedef struct wav_hdr_ //
{
	char                     RIFF[4];        // RIFF Header
	int                      ChunkSize;      // RIFF Chunk Size
	char                     WAVE[4];        // WAVE Header
	char                     fmt[4];         // FMT header
	int                      Subchunk1Size;  // Size of the fmt chunk
	short int                AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
	short int                NumOfChan;      // Number of channels 1=Mono 2=Stereo
	int                      SamplesPerSec;  // Sampling Frequency in Hz
	int                      bytesPerSec;    // bytes per second */
	short int                blockAlign;     // 2=16-bit mono, 4=16-bit stereo
	short int                bitsPerSample;  // Number of bits per sample
	char                     Subchunk2ID[4]; // "data"  string
	int                      Subchunk2Size;  // Sampled data length
}wav_hdr;

#pragma pack()

typedef struct wave_io_
{
	FILE * file;
	int type;
	int total_nb_samples;
	int sample_byte_size;
}wave_io;

#define WAVE_FILE_FORMAT_RAW_8BITS_IQ 0
#define WAVE_FILE_FORMAT_WAV_8BITS_STEREO 1
#define WAVE_FILE_FORMAT_WAV_16BITS_STEREO 2
#define WAVE_FILE_FORMAT_WAV_16BITS_MONO 3

wave_io * create_wave(char * path,int samplerate,int type);
void write_wave(wave_io * w_io, void * buffer, int nbsamples);
void close_wave(wave_io * w_io);

#endif
