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
// File : wave.c
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <stdint.h>

#include "wave.h"

#include "utils.h"

wave_io * create_wave(char * path,int samplerate, int type)
{
	wav_hdr wavhdr;
	wave_io * w_io;
	int sample_byte_size;

	w_io = malloc(sizeof(wave_io));
	if(w_io)
	{
		memset(w_io,0,sizeof(wave_io));

		if(!path)
			w_io->file = stdout;//fopen(stdout,"w+b");
		else
			w_io->file = fopen(path,"w+b");

		if(w_io->file)
		{
			w_io->total_nb_samples = 0;
			w_io->type = type;
			switch(w_io->type)
			{
				case WAVE_FILE_FORMAT_RAW_8BITS_IQ: // Raw / IQ
					w_io->sample_byte_size = 2; // I + Q
				break;

				case WAVE_FILE_FORMAT_WAV_8BITS_STEREO:  // Wave 8 bits stereo
				case WAVE_FILE_FORMAT_WAV_16BITS_STEREO: // Wave 16 bits stereo
				case WAVE_FILE_FORMAT_WAV_16BITS_MONO:   // Wave 16 bits mono
					memset(&wavhdr,0,sizeof(wavhdr));

					memcpy((char*)&wavhdr.RIFF,"RIFF",4);
					memcpy((char*)&wavhdr.WAVE,"WAVE",4);
					memcpy((char*)&wavhdr.fmt,"fmt ",4);
					wavhdr.Subchunk1Size = 16;
					wavhdr.AudioFormat = 1;

					wavhdr.NumOfChan = 2;  // I & Q
					if(w_io->type == WAVE_FILE_FORMAT_WAV_16BITS_MONO)
						wavhdr.NumOfChan = 1;

					wavhdr.SamplesPerSec = samplerate;
					if(w_io->type == WAVE_FILE_FORMAT_WAV_8BITS_STEREO)
						wavhdr.bitsPerSample = 8;
					else
						wavhdr.bitsPerSample = 16;

					sample_byte_size = ((wavhdr.bitsPerSample*wavhdr.NumOfChan)/8);

					w_io->sample_byte_size = sample_byte_size;

					wavhdr.bytesPerSec = ((samplerate*wavhdr.bitsPerSample)/8) * wavhdr.NumOfChan;
					wavhdr.blockAlign = sample_byte_size;
					memcpy((char*)&wavhdr.Subchunk2ID,"data",4);

					wavhdr.ChunkSize = (w_io->total_nb_samples * sample_byte_size) + sizeof(wav_hdr) - 8;
					wavhdr.Subchunk2Size = (w_io->total_nb_samples * sample_byte_size);

					fwrite((void*)&wavhdr,sizeof(wav_hdr),1,w_io->file);

					// Note about the following raw data format :
					// 8-bit samples are stored as unsigned bytes, ranging from 0 to 255.
					// 16-bit samples are stored as 2's-complement signed integers, ranging from -32768 to 32767.

				break;
			}
		}
	}

	return w_io;
}

void write_wave(wave_io * w_io, void * buffer, int nbsamples)
{
	if(w_io)
	{
		if(!w_io->file)
			return;
		
		fwrite(buffer,nbsamples * w_io->sample_byte_size,1, w_io->file);
		w_io->total_nb_samples += nbsamples;
	}
}

void close_wave(wave_io * w_io)
{
	wav_hdr wavhdr;

	if(w_io)
	{
		if(!w_io->file)
			return;

		switch(w_io->type)
		{
			case WAVE_FILE_FORMAT_RAW_8BITS_IQ: // Raw / IQ
				fclose(w_io->file);
			break;

			case WAVE_FILE_FORMAT_WAV_8BITS_STEREO:  // Wave 8 bits stereo
			case WAVE_FILE_FORMAT_WAV_16BITS_STEREO: // Wave 16 bits stereo
			case WAVE_FILE_FORMAT_WAV_16BITS_MONO:   // Wave 16 bits mono
				fseek(w_io->file,0,SEEK_SET);
				if(fread(&wavhdr,sizeof(wav_hdr),1,w_io->file) == 1)
				{
					wavhdr.ChunkSize += ( w_io->total_nb_samples * w_io->sample_byte_size );
					wavhdr.Subchunk2Size += ( w_io->total_nb_samples * w_io->sample_byte_size );

					fseek(w_io->file,0,SEEK_SET);
					fwrite(&wavhdr,sizeof(wav_hdr),1,w_io->file);
				}
				fclose(w_io->file);
			break;
		}

		free(w_io);
	}
}
