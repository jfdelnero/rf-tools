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
// File : rf_jammer.c
// Contains: rf jammer tool
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

#include "modulator.h"

#include "utils.h"

#include "rand_gen.h"

#define printf(fmt...) do { \
		if(!stdoutmode) \
		fprintf(stdout, fmt); \
	} while (0)

#define IQ_SAMPLE_RATE           (10000000UL)

#define IF_FREQ                     0

int verbose;
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
	printf("  -stdout\t\t\t: IQ stream send to stdout\n");
	printf("  -iq_rate\t\t\t: IQ rate (default : 10000000)\n");
	printf("  -freq_bw:[Hz]\t\t\t: bandwith (Hz)\n");
	printf("  -ping_pong_freq:[centi Hz]\t: ping pong freq (per step of 0.01 Hz)\n");
	printf("  -jam_mode:[Mode id]\t\t: Mode. 0=ping pong, 1=random, 2=fixed frequency, 3=full iq random\n");
	printf("  -jam_interval:[uS]\t\t: Interval (uS)\n");
	printf("  -jam_duration:[uS]\t\t: pulses duration (uS)\n");
	printf("  -jam_long_interval:[uS]\t: pulses long interval (uS)\n");
	printf("  -jam_group_pulses:[cnt]\t: pulses group count\n");
	printf("  -rand_interval\t\t: random interval mode\n");
	printf("  -rand_duration\t\t: random duration mode\n");
	printf("  -jam_total_duration:[uS]\t: total duration\n");
	printf("  -generate\t\t\t: Generate the IQ stream\n");
	printf("  -help\t\t\t\t: This help\n");
	printf("\n  Examples :\n");
	printf("    ./rf_jammer -generate -stdout -iq_rate:10000000 -freq_bw:50000 -jam_mode:1 -jam_interval:625 -jam_duration:10 | hackrf_transfer  -f 5500000000 -t -  -x 47 -a 1 -s 10000000\n");
	printf("    ./rf_jammer -generate -stdout -iq_rate:2000000 -jam_mode:2 -jam_interval:613 -jam_duration:12 -jam_long_interval:250000 -jam_group_pulses:15  | hackrf_transfer  -f 5500000000 -t -  -x 47 -a 1 -s 2000000\n");
	printf("\n");
}

#define BUFFER_SAMPLES_SIZE (2048*8)

int us2ticks(int samplerate, int us)
{
	return (us * (samplerate / 1000000));
}

int main(int argc, char* argv[])
{
	char temp_str[512];

	wave_io * wave1,*wave2;
	uint16_t iq_wavebuf[ BUFFER_SAMPLES_SIZE ];

	int16_t wavebuf_dbg[BUFFER_SAMPLES_SIZE];
	double  wavebuf_dbg2[BUFFER_SAMPLES_SIZE];

	unsigned int i,j;
	int freq_bw;
	int pulses_cnt;
	int ping_pong_freq;
	int jam_mode;

	int pulses_interval, pulses_interval_cnt;
	int pulses_long_interval,pulses_short_interval;
	int pulses_duration, pulses_duration_cnt;
	int pulses_group_nb;
	int jam_total_duration,total_duration;

	int rand_interval,rand_duration;

	double ping_pong_sample;
	double fm_mod;

	uint32_t randdword;

	rand_gen_state randgen;

	iq_wave_gen iqgen;

	wave_gen ping_pong_gen;

	verbose=0;
	stdoutmode = 0;
	jam_mode = 0;
	freq_bw = 200000;

	ping_pong_freq = 1000; // 10.00 Hz

	pulses_duration_cnt = 0;
	pulses_interval_cnt = 0;
	rand_interval = 0;
	rand_duration = 0;
	pulses_cnt = 0;

	if(isOption(argc,argv,"stdout",NULL)>0)
	{
		stdoutmode = 1;
	}

	if(!stdoutmode)
	{
		printf("rf_jammer v0.0.0.2\n");
		printf("Copyright (C) 2022 Jean-Francois DEL NERO\n");
		printf("This program comes with ABSOLUTELY NO WARRANTY\n");
		printf("This is free software, and you are welcome to redistribute it\n");
		printf("under certain conditions;\n\n");
	}

	// Verbose option...
	if(isOption(argc,argv,"verbose",0)>0)
	{
		printf("verbose mode\n");
		verbose=1;
	}

	// help option...
	if(isOption(argc,argv,"help",0)>0)
	{
		printhelp(argv);
	}

	pulses_interval = 0;
	pulses_short_interval = 0;
	pulses_long_interval = 0;
	pulses_duration = 2000000;

	if(isOption(argc,argv,"freq_bw",(char*)&temp_str)>0)
	{
		freq_bw = atoi(temp_str);
	}

	if(isOption(argc,argv,"ping_pong_freq",(char*)&temp_str)>0)
	{
		ping_pong_freq = atoi(temp_str);
	}

	if(isOption(argc,argv,"jam_mode",(char*)&temp_str)>0)
	{
		jam_mode = atoi(temp_str) ;
	}

	iqgen.sample_rate = IQ_SAMPLE_RATE;
	if(isOption(argc,argv,"iq_rate",(char*)&temp_str)>0)
	{
		iqgen.sample_rate = atoi(temp_str);
	}

	if(isOption(argc,argv,"jam_interval",(char*)&temp_str)>0)
	{
		pulses_interval = us2ticks(iqgen.sample_rate, atoi(temp_str));
		pulses_short_interval = pulses_interval;
	}

	if(isOption(argc,argv,"jam_duration",(char*)&temp_str)>0)
	{
		pulses_duration = us2ticks(iqgen.sample_rate, atoi(temp_str));
	}

	if(isOption(argc,argv,"jam_long_interval",(char*)&temp_str)>0)
	{
		pulses_long_interval = us2ticks(iqgen.sample_rate, atoi(temp_str));
	}

	pulses_group_nb = 0;
	if(isOption(argc,argv,"jam_group_pulses",(char*)&temp_str)>0)
	{
		pulses_group_nb = atoi(temp_str);
	}

	if(isOption(argc,argv,"rand_interval",NULL)>0)
	{
		rand_interval = 1;
	}

	if(isOption(argc,argv,"rand_duration",NULL)>0)
	{
		rand_duration = 1;
	}

	jam_total_duration = -1;
	if(isOption(argc,argv,"jam_total_duration",(char*)&temp_str)>0)
	{
		jam_total_duration = us2ticks(iqgen.sample_rate, atoi(temp_str));
	}

	if(isOption(argc,argv,"generate",0)>0)
	{
		memset(iq_wavebuf, 0, BUFFER_SAMPLES_SIZE * sizeof(uint16_t) );

		rand_gen_init(&randgen, 0xC279DCEF );

		// Init oscillators

		// ping pong freq
		ping_pong_gen.phase = 0;
		ping_pong_gen.Frequency = ((double)ping_pong_freq) / 100.0;
		ping_pong_gen.Amplitude = 100;
		ping_pong_gen.sample_rate = iqgen.sample_rate;

		// IQ Modulator
		iqgen.phase = 0;
		iqgen.Frequency = IF_FREQ;
		iqgen.Amplitude = 127;

		if(stdoutmode)
		{
			// stdout / stream mode : IQ are outputed to the stdout -> use a pipe to hackrf_transfer
			wave1 = create_wave(NULL,iqgen.sample_rate,WAVE_FILE_FORMAT_RAW_8BITS_IQ);
			wave2 = NULL;
		}
		else
		{
			// file mode : create iq + wav files
			wave1 = create_wave("test.iq",iqgen.sample_rate,WAVE_FILE_FORMAT_RAW_8BITS_IQ);
			wave2 = create_wave("test.wav",iqgen.sample_rate,WAVE_FILE_FORMAT_WAV_16BITS_MONO);
		}

		if(wave1)
		{
			total_duration = 0;

			// Main loop...
			for(i=0;(i<8) || stdoutmode ;i++)
			{
				if(jam_mode != 3)
				{
					for(j=0;j<BUFFER_SAMPLES_SIZE;j++)
					{
						switch( jam_mode )
						{
							case 0: // freq ping pong
								ping_pong_sample = f_get_next_sample(&ping_pong_gen);

								fm_mod = ((ping_pong_sample / (double)(100.0)) * (double)(freq_bw));

								wavebuf_dbg[j] = fm_mod;
								wavebuf_dbg2[j] = fm_mod;
							break;
							case 1: // rand frequency
								randdword = rand_gen_get_next_word(&randgen);

								fm_mod = (((double)(randdword & 0x7FFFFFFF) / (double)(0x7FFFFFFF)) * (double)(freq_bw));

								if(randdword & 0x80000000)
									fm_mod = -fm_mod;

								wavebuf_dbg[j] = fm_mod;
								wavebuf_dbg2[j] = fm_mod;
							break;
							case 2: // pulse mode / fixed frequency
								fm_mod = 0;
								wavebuf_dbg[j] = fm_mod;
								wavebuf_dbg2[j] = fm_mod;
							break;

							default:
							break;
						}

						iqgen.Frequency = ((double)IF_FREQ + wavebuf_dbg2[j]);

						if( pulses_interval_cnt < pulses_interval )
						{
							pulses_interval_cnt++;
							iq_wavebuf[j] = 0x0000;
						}
						else
						{
							iq_wavebuf[j] = get_next_iq(&iqgen);//iq_sample;

							if(pulses_duration_cnt < pulses_duration)
							{
								pulses_duration_cnt++;
							}
							else
							{
								pulses_duration_cnt = 0;
								pulses_interval_cnt = 0;
								pulses_cnt++;

								if(pulses_group_nb)
								{
									if(pulses_cnt >= pulses_group_nb)
									{
										pulses_interval = pulses_long_interval;
										pulses_cnt = 0;
									}
									else
									{
										pulses_interval = pulses_short_interval;
									}
								}
								else
								{
									pulses_interval = pulses_short_interval;
								}

								if(rand_interval)
									pulses_interval = (rand_gen_get_next_word(&randgen) & 0x7F) * (iqgen.sample_rate / 1000000);

								if(rand_duration)
									pulses_duration = (rand_gen_get_next_word(&randgen) & 0x7F) * (iqgen.sample_rate / 1000000);
							}
						}

						total_duration++;

						if(jam_total_duration >= 0)
						{
							if(total_duration >= jam_total_duration)
							{
								i = 8;
								stdoutmode = 0;
								break;
							}
						}
					}
				}
				else
				{
					for(j=0;j<BUFFER_SAMPLES_SIZE;j++)
					{
						randdword = rand_gen_get_next_word(&randgen);

						if( pulses_interval_cnt < pulses_interval )
						{
							pulses_interval_cnt++;
							iq_wavebuf[j] = 0x0000;
						}
						else
						{
							iq_wavebuf[j] = (randdword ^ (randdword>>16));

							if(pulses_duration_cnt < pulses_duration)
							{
								pulses_duration_cnt++;
							}
							else
							{
								pulses_duration_cnt = 0;
								pulses_interval_cnt = 0;
								pulses_cnt++;

								if(pulses_group_nb)
								{
									if(pulses_cnt >= pulses_group_nb)
									{
										pulses_interval = pulses_long_interval;
										pulses_cnt = 0;
									}
									else
									{
										pulses_interval = pulses_short_interval;
									}
								}
								else
								{
									pulses_interval = pulses_short_interval;
								}

								if(rand_interval)
									pulses_interval = (rand_gen_get_next_word(&randgen) & 0x7F) * (iqgen.sample_rate / 1000000);

								if(rand_duration)
									pulses_duration = (rand_gen_get_next_word(&randgen) & 0x7F) * (iqgen.sample_rate / 1000000);

							}
						}

						total_duration++;

						if(jam_total_duration >= 0)
						{
							if(total_duration >= jam_total_duration)
							{
								i = 8;
								stdoutmode = 0;
								break;
							}
						}
					}
				}

				write_wave(wave1, &iq_wavebuf,BUFFER_SAMPLES_SIZE);
				write_wave(wave2, &wavebuf_dbg,BUFFER_SAMPLES_SIZE);
			}

			close_wave(wave1);
			close_wave(wave2);
		}
	}


	if( (isOption(argc,argv,"help",0)<=0) &&
		(isOption(argc,argv,"generate",0)<=0)
		)
	{
		printhelp(argv);
	}

	return 0;
}
