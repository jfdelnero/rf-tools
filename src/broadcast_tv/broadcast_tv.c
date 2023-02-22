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
// File : broadcast_tv.c
// Contains: a broadcast TV modulator
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

//
// Disclaimer / Legal warning : Radio spectrum and the law
//
// In most countries the use of any radio transmitting device is required to be
// either licensed or specifically exempted from licensing under the local regulator.
// Other than as used in accordance with a licence (or exemption),
// the use of radio equipment is illegal.
//
// So take care to limit the emitting range and power when testing this software !
//
// --------------------------------------------------------------------------------

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

#include "modulator.h"

#include "utils.h"

#include "composite.h"

#include "bmp_file.h"

#define IQ_SAMPLE_RATE           16000000

#define IF_FREQ                        0

#define BUFFER_SAMPLES_SIZE (1024*8)

#define printf(fmt...) do { \
		if(!stdoutmode) \
		fprintf(stdout, fmt); \
	} while (0)

int stdoutmode;

int isOption(int argc, char* argv[],char * paramtosearch,char * argtoparam)
{
	int param=1;
	int i,j;

	char option[512];

	memset(option,0,512);
	while(param<=argc)
	{
		if(argv[param])
		{
			if(argv[param][0]=='-')
			{
				memset(option,0,512);

				j=0;
				i=1;
				while( argv[param][i] && argv[param][i]!=':')
				{
					option[j]=argv[param][i];
					i++;
					j++;
				}

				if( !strcmp(option,paramtosearch) )
				{
					if(argtoparam)
					{
						if(argv[param][i]==':')
						{
							i++;
							j=0;
							while( argv[param][i] )
							{
								argtoparam[j]=argv[param][i];
								i++;
								j++;
							}
							argtoparam[j]=0;
							return 1;
						}
						else
						{
							return -1;
						}
					}
					else
					{
						return 1;
					}
				}
			}
		}
		param++;
	}

	return 0;
}

void printhelp(char* argv[])
{
	printf("Options:\n");
	printf("  -stdout \t\t\t: IQ stream send to stdout\n");
	printf("  -bmp_file:[BMPFILE.BMP]\t: BMP image file to display\n");
	printf("  -generate \t\t\t: Generate the video IQ stream\n");
	printf("  -help \t\t\t: This help\n\n");
	printf("Example : 525MHz TV broadcasting with an hackrf (currently B&W with PAL-L/SECAM timings and polarity)\n./broadcast_tv -generate -stdout -bmp_file:Philips_PM5544.bmp | hackrf_transfer  -f 525000000 -t -  -x 47 -a 1 -s 16000000\n");
	printf("\n");
}

int main(int argc, char* argv[])
{
	unsigned int i,j;

	int ret;
	char filename[512];

	wave_io * wave1,*wave2;

	double  video_buf[BUFFER_SAMPLES_SIZE];
	composite_state vid_stat;
	int16_t  subcarriers_dbg_wave_buf[BUFFER_SAMPLES_SIZE];
	uint16_t iq_wave_buf[ BUFFER_SAMPLES_SIZE];

	iq_wave_gen iqgen;
	bitmap_data bmp_data;

	stdoutmode = 0;

	if(isOption(argc,argv,"stdout",NULL)>0)
	{
		stdoutmode = 1;
	}

	if(!stdoutmode)
	{
		printf("broadcast_tv v0.0.1.1\n");
		printf("Copyright (C) 2023 Jean-Francois DEL NERO\n");
		printf("This program comes with ABSOLUTELY NO WARRANTY\n");
		printf("This is free software, and you are welcome to redistribute it\n");
		printf("under certain conditions;\n\n");
	}

	// help option...
	if(isOption(argc,argv,"help",0)>0)
	{
		printhelp(argv);
	}

	memset(filename,0,sizeof(filename));

	if(isOption(argc,argv,"bmp_file",(char*)&filename)>0)
	{
		printf("Input file : %s\n",filename);
	}

	memset(&bmp_data,0,sizeof(bmp_data));
	if(strlen(filename))
	{
		ret = bmp_load(filename,&bmp_data);
		if( ret >= 0)
		{
			printf("BMP file loaded successfully\n");
		}
		else
		{
			printf("BMP load error %d\n",ret);
		}
	}

	if(isOption(argc,argv,"generate",0)>0)
	{
		// IQ Modulator
		iqgen.phase = 0;
		iqgen.Frequency = IF_FREQ;
		iqgen.Amplitude = 127;
		iqgen.sample_rate = IQ_SAMPLE_RATE;

		init_composite(&vid_stat, 16000000, 768, 576,bmp_data.data);

		if(stdoutmode)
		{
			// stdout / stream mode : IQ are outputed to the stdout -> use a pipe to hackrf_transfer
			wave1 = create_wave(NULL,iqgen.sample_rate,WAVE_FILE_FORMAT_RAW_8BITS_IQ);
			wave2 = NULL;
		}
		else
		{
			// file mode : create iq + wav files
			wave1 = create_wave("broadcast_tv.iq",iqgen.sample_rate,WAVE_FILE_FORMAT_RAW_8BITS_IQ);
			wave2 = create_wave("broadcast_tv.wav",16000000,WAVE_FILE_FORMAT_WAV_16BITS_MONO);
		}

		for(i=0;i<1024*16*2 || stdoutmode;i++)
		{
			gen_video_signal(&vid_stat, (double*)&video_buf, BUFFER_SAMPLES_SIZE);
			for(j=0;j<BUFFER_SAMPLES_SIZE;j++)
			{
				subcarriers_dbg_wave_buf[j] = (int)((video_buf[j]/(double)100) * (double)32767);

				iqgen.Amplitude = ((double)video_buf[j]);
				iq_wave_buf[j] = get_next_iq(&iqgen);
			}

			write_wave(wave1, &iq_wave_buf,BUFFER_SAMPLES_SIZE);
			write_wave(wave2, &subcarriers_dbg_wave_buf,BUFFER_SAMPLES_SIZE);
		}

		close_wave(wave1);
		close_wave(wave2);
	}

	if( (isOption(argc,argv,"help",0)<=0) &&
		(isOption(argc,argv,"generate",0)<=0)
		)
	{
		printhelp(argv);
	}

	return 0;
}
