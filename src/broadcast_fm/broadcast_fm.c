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
// File : broadcast_fm.c
// Contains: a broadcast FM Stereo modulator
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
// Example from the FCC (United states) :
//
// Part 15 Devices
//
// Unlicensed operation on the AM and FM radio broadcast bands is permitted for
// some extremely low powered devices covered under Part 15 of the FCC's rules.
// On FM frequencies, these devices are limited to an effective service range
// of approximately 200 feet (61 meters).
// See 47 CFR (Code of Federal Regulations) Section 15.239, and the July 24,
// 1991 Public Notice (still in effect).
//
// On the AM broadcast band, these devices are limited to an effective service
// range of approximately 200 feet (61 meters). See 47 CFR Sections 15.207,
// 15.209, 15.219, and 15.221.  These devices must accept any interference
// caused by any other operation, which may further limit the effective service
// range.
//
// For more information on Part 15 devices, please see OET Bulletin No. 63
// ("Understanding the FCC Regulations for Low-Power, Non-Licensed Transmitters").
// Questions not answered by this Bulletin can be directed to the FCC's Office
// of Engineering and Technology, Customer Service Branch, at the Columbia,
// Maryland office, phone (301) 362 - 3000.
//
// [...]
//
// Penalties for Operation Without A Permit or License
//
// The Commission considers unauthorized broadcast operation to be a serious matter.
// Presently, the maximum penalty for operating an unlicensed or "pirate" broadcast
// station (one which is not permitted under Part 15 or is not a Carrier Current
// Station or Campus Radio Station) is set at $10,000 for a single violation or a
// single day of operation, up to a total maximum amount of $75,000.
//
// Adjustments may be made upwards or downwards depending on the circumstances
// involved. Equipment used for an unauthorized operation may also be confiscated.
// There are also criminal penalties (fine and/or imprisonment) for
// "willfully and knowingly" operating a radio station without a license.
// DON'T DO IT!
//
// More at : https://www.fcc.gov/media/radio/low-power-radio-general-information
//
// --------------------------------------------------------------------------------

//
// Broadcast FM subcarriers
//
//      ________            Stereo L - R modulation
//     /        \                38KHz DSB-SC
//    /          \           _______      _______
//   /    MONO    \  Pilot  /       \    /       \      RDS
//  /     L + R    \   |   /         \  /         \    _  _
// /                \__|__/           \/           \__/ \/ \___
//30Hz           15Khz |  |            |            |    |
//                     |  |            |            |  57KHz 5%
//                  19KHz 23KHz       38KHz         53KHz
//                    10%
//

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Current software design :
//
// Mod player  /  Left Audio  -> Preemphasis (FIR) -> \  / -> Left + Right -> 15KHz low pass (FIR) >------------------------------|
// Sound                                               \/                                                                         |
// generator                                           /\                                                                         |
//             \  Right Audio -> Preemphasis (FIR) -> /  \ -> Left - Right -> 15KHz low pass (FIR)                                |
//                                                                                                |                               |
// Sample      |  Sample Rate at 200KHz                                                           V                               |
// Rate at     |                                                                                |MUL| (38KHz DSB-SC modulated)->  + ----> I+Q Modulator --> RF hardware transceiver
// 50KHz       |                                                                                  ^                               |     (2MHz sample rate)
//             |                                                                                  |                               |
//             |                                                                               38KHz Osc                          |
//             |                                                                                 (|) (These oscillators           |
//             |                                                                                 (|)  must be kept in phase)      |
//             |                                                                               19KHz Pilot Osc  >-----------------+
//             |                                                                                 (|)                              |
//             |                                                                                 (|)      (57KHz DSB-SC modulated)|
//             |                                                                               57KHz Osc-->|MUL|>-----------------+
//                                                                                                |          ^
//                                                                                           ____ v_____     |
//                                                                                          |           |    |
//                                                                                          |    RDS    |>----
//                                                                                          | Generator |
//                                                                                          |___________|
//
//
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

#include "rds.h"

#include "utils.h"

#include "fir_filters/FIR_Audio_Filter_Filter.h"
#include "fir_filters/AudioPreemphasis_Filter.h"

#include "hxcmod/hxcmod.h"

#define IQ_SAMPLE_RATE           2000000
#define SUBCARRIERS_SAMPLE_RATE   200000

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
	printf("  -mono \t\t\t: Mono FM mode\n");
	printf("  -mod_file:[MODFILE.MOD]\t: MOD music file to play\n");
	printf("  -generate \t\t\t: Generate the IQ stream\n");
	printf("  -help \t\t\t: This help\n\n");
	printf("Example : 100.9MHz broadcasting with an hackrf\n./broadcast_fm -generate -stdout -mod_file:meo-sleeping_waste.mod  | hackrf_transfer  -f 100900000 -t -  -x 47 -a 1 -s 2000000\n");
	printf("\n");
}

int main(int argc, char* argv[])
{
	unsigned int i,j,k;

	FILE *f;
	char filename[512];

	int mod_data_size = 0;
	unsigned char * mod_data;

	wave_io * wave1,*wave2;

	double audio_sample_l;
	double audio_sample_r;
	double audio_sample_final;
	double audio_sample_r_car;
	double pilot_sample;
	double rds_sample,rds_mod_sample;
	double balance_sample;
	double fm_mod;

	double old_freq,interpolation_step;
	double leftplusright_audio,leftminusright_audio;

	modcontext modctx;

	FIR_Audio_Filter_Filter leftplusright_audio_filter,leftminusright_audio_filter;
	AudioPreemphasis_Filter preamphasis_left_filter,preamphasis_right_filter;
	//FM_Baseband_Filter fmband_filter;

	iq_wave_gen iqgen;

	wave_gen audio_l_gen;
	wave_gen audio_r_gen;

	wave_gen balance_ctrl;

	wave_gen audiow_stereo38KHz_gen;
	wave_gen stereo_pilot_gen;
	wave_gen rds_carrier_57KHz_gen;

	rds_stat rdsstat;

	uint16_t iq_wave_buf[ BUFFER_SAMPLES_SIZE * (IQ_SAMPLE_RATE/SUBCARRIERS_SAMPLE_RATE)];
	int16_t  mod_wave_buf[(BUFFER_SAMPLES_SIZE*2)/4];
	int16_t  subcarriers_dbg_wave_buf[BUFFER_SAMPLES_SIZE];
	double   subcarriers_float_wave_buf[BUFFER_SAMPLES_SIZE];

	int monomode;

	stdoutmode = 0;
	monomode = 0;

	if(isOption(argc,argv,"stdout",NULL)>0)
	{
		stdoutmode = 1;
	}

	if(!stdoutmode)
	{
		printf("broadcast_fm v0.0.1.1\n");
		printf("Copyright (C) 2022 Jean-Francois DEL NERO\n");
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
	// Input file name option
	if(isOption(argc,argv,"mod_file",(char*)&filename)>0)
	{
		printf("Input file : %s\n",filename);
	}

	if(isOption(argc,argv,"mono",NULL)>0)
	{
		monomode = 1;
	}

	if(isOption(argc,argv,"generate",0)>0)
	{
		// Init the .mod player and load the mod file.
		hxcmod_init(&modctx);

		hxcmod_setcfg(&modctx, SUBCARRIERS_SAMPLE_RATE/4, 1, 0); //(50000 samples/s, 25000 Hz)

		mod_data_size = 0;
		mod_data = NULL;
		f = fopen(filename,"r");
		if(f)
		{
			fseek(f,0,SEEK_END);
			mod_data_size = ftell(f);
			fseek(f,0,SEEK_SET);

			mod_data = malloc(mod_data_size);

			if(fread(mod_data,mod_data_size,1,f) == 1)
			{
				hxcmod_load(&modctx, mod_data, mod_data_size );
			}
			else
			{
				free(mod_data);
				mod_data = NULL;
				mod_data_size = 0;
			}

			fclose(f);
		}

		// Init all filters
		AudioPreemphasis_Filter_init(&preamphasis_left_filter);
		AudioPreemphasis_Filter_init(&preamphasis_right_filter);
		FIR_Audio_Filter_Filter_init(&leftplusright_audio_filter);
		FIR_Audio_Filter_Filter_init(&leftminusright_audio_filter);

		//FM_Baseband_Filter_init(&fmband_filter);

		// Init oscillators

		// Left and Right audio freq (used if no .mod music file)
		audio_l_gen.phase = 0;
		audio_l_gen.Frequency = 700;
		audio_l_gen.Amplitude = 21.25;
		audio_l_gen.sample_rate = SUBCARRIERS_SAMPLE_RATE;

		audio_r_gen.phase = 0;
		audio_r_gen.Frequency = 1000;
		audio_r_gen.Amplitude = 21.25;
		audio_r_gen.sample_rate = SUBCARRIERS_SAMPLE_RATE;

		// Left <> Right balance LFO (used if no .mod music file)
		balance_ctrl.phase = 0;
		balance_ctrl.Frequency = 1.2;
		balance_ctrl.Amplitude = 12.5;
		balance_ctrl.sample_rate = SUBCARRIERS_SAMPLE_RATE;

		// Stereo Pilot
		stereo_pilot_gen.phase = 0;
		stereo_pilot_gen.Frequency = 19000;
		stereo_pilot_gen.Amplitude = 10.0;  // 10%
		stereo_pilot_gen.sample_rate = SUBCARRIERS_SAMPLE_RATE;

		// 38KHz +/- 15KHz stereo modulator
		audiow_stereo38KHz_gen.phase = 0;
		audiow_stereo38KHz_gen.Frequency = 38000;
		audiow_stereo38KHz_gen.Amplitude = 1;
		audiow_stereo38KHz_gen.sample_rate = SUBCARRIERS_SAMPLE_RATE;

		// 57KHz RDS carrier
		rds_carrier_57KHz_gen.phase = 0;
		rds_carrier_57KHz_gen.Frequency = 57000;
		rds_carrier_57KHz_gen.Amplitude = 1;
		rds_carrier_57KHz_gen.sample_rate = SUBCARRIERS_SAMPLE_RATE;

		init_rds_encoder(&rdsstat,SUBCARRIERS_SAMPLE_RATE);

		// IQ Modulator
		iqgen.phase = 0;
		iqgen.Frequency = IF_FREQ;
		iqgen.Amplitude = 127;
		iqgen.sample_rate = IQ_SAMPLE_RATE;

		audio_sample_r = 0;
		audio_sample_l = 0;

		if(stdoutmode)
		{
			// stdout / stream mode : IQ are outputed to the stdout -> use a pipe to hackrf_transfer
			wave1 = create_wave(NULL,iqgen.sample_rate,WAVE_FILE_FORMAT_RAW_8BITS_IQ);
			wave2 = NULL;
		}
		else
		{
			// file mode : create iq + wav files
			wave1 = create_wave("broadcast_fm.iq",iqgen.sample_rate,WAVE_FILE_FORMAT_RAW_8BITS_IQ);
			wave2 = create_wave("broadcast_fm.wav",SUBCARRIERS_SAMPLE_RATE,WAVE_FILE_FORMAT_WAV_16BITS_MONO);
		}

		if(wave1)
		{
			old_freq = IF_FREQ;

			// Main loop...
			for(i=0;(i<1024) || stdoutmode ;i++)
			{
				printf("%d / %d...\n",i,1024);

				hxcmod_fillbuffer( &modctx, (msample*)&mod_wave_buf, BUFFER_SAMPLES_SIZE/4, NULL );

				for(j=0;j<BUFFER_SAMPLES_SIZE;j++)
				{
					if(!mod_data_size)
					{
						// No music module loaded -> Play left/right tones.

						// Dynamically set volumes for left and right oscillators.
						balance_sample = f_get_next_sample(&balance_ctrl);

						audio_l_gen.Amplitude = balance_sample;
						audio_r_gen.Amplitude = -balance_sample;
						if(audio_r_gen.Amplitude < 0) audio_r_gen.Amplitude = 0;
						if(audio_l_gen.Amplitude < 0) audio_l_gen.Amplitude = 0;

						// Get the left and right samples.
						audio_sample_l = f_get_next_sample(&audio_l_gen);
						audio_sample_r = f_get_next_sample(&audio_r_gen);
					}
					else
					{
						if(!(j&3))
						{
							audio_sample_l = ((double)((mod_wave_buf[((j/4)*2)]) / (double)32768)) * (double)22.5;
							audio_sample_r = ((double)((mod_wave_buf[((j/4)*2)+1]) / (double)32768)) * (double)22.5;

							// Left & Right Preamphasis filter.
							AudioPreemphasis_Filter_put(&preamphasis_left_filter, audio_sample_l );
							audio_sample_l = AudioPreemphasis_Filter_get(&preamphasis_left_filter);

							AudioPreemphasis_Filter_put(&preamphasis_right_filter, audio_sample_r );
							audio_sample_r = AudioPreemphasis_Filter_get(&preamphasis_right_filter);
						}
					}

					// Main / Mono channel : Left + Right
					leftplusright_audio = audio_sample_l + audio_sample_r;

					// 0KHz<->15KHz pass band/low pass filter
					FIR_Audio_Filter_Filter_put(&leftplusright_audio_filter, leftplusright_audio);
					leftplusright_audio = FIR_Audio_Filter_Filter_get(&leftplusright_audio_filter);

					if(!monomode)
					{
						// Stereo Channel : Left - Right
						leftminusright_audio = audio_sample_l - audio_sample_r;

						// 0KHz<->15KHz pass band/low pass filter
						FIR_Audio_Filter_Filter_put(&leftminusright_audio_filter, leftminusright_audio);
						leftminusright_audio = FIR_Audio_Filter_Filter_get(&leftminusright_audio_filter);

						// Keep the 19KHz pilot and the 38KHz clock in phase :
						audiow_stereo38KHz_gen.phase = (stereo_pilot_gen.phase * 2) + PI/2;

						rds_carrier_57KHz_gen.phase = (stereo_pilot_gen.phase * 3) + PI/2;

						rds_mod_sample = get_rds_bit_state(&rdsstat,stereo_pilot_gen.phase);

						// 38KHz DSB-SC (Double-sideband suppressed-carrier) modulation
						audio_sample_r_car = f_get_next_sample(&audiow_stereo38KHz_gen);      // Get the 38KHz carrier
						audio_sample_r_car = (audio_sample_r_car * leftminusright_audio );    // And multiply it with the left - right sample.

						// 57KHz DSB-SC (Double-sideband suppressed-carrier) modulation
						rds_sample = f_get_next_sample(&rds_carrier_57KHz_gen);               // Get the 57KHz carrier
						rds_sample = ( rds_sample * rds_mod_sample );                         // And multiply it with the rds sample.

						// 19KHz pilot
						pilot_sample = f_get_next_sample(&stereo_pilot_gen);
					}
					else
					{
						// Mono : No pilot nor 38KHz modulation...
						audio_sample_r_car = 0;
						pilot_sample = 0;
						rds_sample = 0;
					}

					// Mix all signals sources :
					//                             42.5%                  42.5%             10%           5%
					audio_sample_final = ((leftplusright_audio) + audio_sample_r_car + pilot_sample + rds_sample);

					// Main carrier frequency modulation : +/- 75KHz
					fm_mod = ((audio_sample_final / (double)(100.0)) * (double)(75000));

					// Low pass filter <100KHz
					// Note : Not needed here since we use a 200KHz sample rate.
					//FM_Baseband_Filter_put(&fmband_filter, fm_mod );
					//fm_mod = FM_Baseband_Filter_get(&fmband_filter);

					subcarriers_dbg_wave_buf[j] = fm_mod/4;
					subcarriers_float_wave_buf[j] = fm_mod;
				}

				// Sub carriers sample rate to carrier IQ rate modulation + resampling
				for(j=0;j<BUFFER_SAMPLES_SIZE;j++)
				{
					// linear interpolation. (TODO ?: Cubic interpolation)
					interpolation_step = (subcarriers_float_wave_buf[j] - old_freq) / (double)(IQ_SAMPLE_RATE/SUBCARRIERS_SAMPLE_RATE);

					for(k=0;k<(IQ_SAMPLE_RATE/SUBCARRIERS_SAMPLE_RATE);k++)
					{
						old_freq += interpolation_step;

						iqgen.Frequency = ((double)IF_FREQ + old_freq);
						iq_wave_buf[(j*(IQ_SAMPLE_RATE/SUBCARRIERS_SAMPLE_RATE))+k] = get_next_iq(&iqgen);
					}

					old_freq = subcarriers_float_wave_buf[j];
				}

				write_wave(wave1, &iq_wave_buf,BUFFER_SAMPLES_SIZE*(IQ_SAMPLE_RATE/SUBCARRIERS_SAMPLE_RATE));
				write_wave(wave2, &subcarriers_dbg_wave_buf,BUFFER_SAMPLES_SIZE);
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
