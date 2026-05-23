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
// File : pocsag.c
// Contains: a pocsag transmitter
//
// This file is part of rf-tools.
//
// Written by: Jean-François DEL NERO
//
// Copyright (C) 2022-2026 Jean-François DEL NERO
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
#include "serial.h"
#include "utils.h"

#define IQ_SAMPLE_RATE            2000000
#define CENTRAL_IF_FREQ           0

#define BUFFER_IQ_SAMPLES_SIZE (1024*8)

#define printf(fmt...) do { \
		if(!stdoutmode) \
		fprintf(stdout, fmt); \
	} while (0)

int stdoutmode;

#pragma pack(1)

typedef struct _poc_batch
{
	uint32_t sync;
	uint32_t words[8*2];
} poc_batch;

#pragma pack()

typedef struct _poc
{
	int preambule_cnt;
	int frm_cnt;
	int frm_end;
}poc;

int isOption(int argc, char* argv[],char * paramtosearch,char * argtoparam)
{
	int param=1;
	int i,j;

	char option[512];

	memset(option,0,sizeof(option));

	while(param<=argc)
	{
		if(argv[param])
		{
			if(argv[param][0]=='-')
			{
				memset(option,0,sizeof(option));

				j=0;
				i=1;
				while( argv[param][i] && argv[param][i]!=':' && ( j < (sizeof(option) - 1)) )
				{
					option[j]=argv[param][i];
					i++;
					j++;
				}

				if( !strcmp(option,paramtosearch) )
				{
					if(argtoparam)
					{
						argtoparam[0] = 0;

						if(argv[param][i]==':')
						{
							i++;
							j=0;
							while( argv[param][i] && j < (512 - 1) )
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
	printf("  -generate \t\t\t: Generate the IQ stream\n");
	printf("  -baud:[BAUD]\t\t\t: Baud rate setting (Default:1200)\n");
	printf("  -ric:[RIC]\t\t\t: RIC address (Default:8)\n");
	printf("  -func:[function_code]\t\t: Function code (Default:0 Min/Max:0-3)\n");
	printf("  -freqshift:[Hz]\t\t: FSK frequency shift (Default:4500 -> +4500Hz / -4500Hz)\n");
	printf("  -smprate:[Hz]\t\t\t: IQ Sample rate (Default:2000000\n");
	printf("  -alpha\t\t\t: Alphanumeric mode (Default: Numeric mode\n");

	printf("  -help \t\t\t: This help\n\n");
	printf("Example : ./pocsag -generate -stdout -ric:154232 -message:\"Hello\" -alpha | hackrf_transfer  -f 466200500 -t -  -x 10 -a 0 -s 2000000\n");
	printf("\n");
}

uint32_t adr_codeword(uint32_t ric_address, uint8_t func)
{
	uint32_t val;

	// Get the 18 bits most significant bits ric address part.
	// The 3 lower bits is the frame position

	val =   ( ( (ric_address>>3) & ((0x1<<18)-1) ) << 13 ) |
			( ( (uint32_t)(func&3)) << 11);

	return val;
}

// BCH(31,21) (21 bit data - 31 bit code word, without the parity)
// g(x) = x^10 + x^9 + x^8 + x^6 + x^5 + x^3 + 1

uint32_t calc_pocsag_bch(uint32_t codeword)
{
	uint32_t bit = 0;
	uint32_t parity = 0;
	uint32_t tmp_codeword;
	uint32_t shiftr;

	tmp_codeword = codeword & 0xFFFFF800;
	shiftr = tmp_codeword;

	// BCH bits
	for (bit = 1; bit <= 21; bit++)  // 21 bits of data
	{
		if (shiftr & 0x80000000)
		{
			shiftr ^= (0x769U << (32U - (31-21) - 1U));
		}
		shiftr <<= 1;
	}

	tmp_codeword |= (shiftr >> 21);

	// Parity bit
	shiftr = tmp_codeword;
	bit = 32;
	while ((shiftr != 0) && (bit > 0))
	{
		if (shiftr & 1)
		{
			parity++;
		}
		shiftr >>= 1;
		bit--;
	}

	return tmp_codeword | (parity&1);
}

unsigned char pocsag_nextbyte(poc * ctx, poc_batch * batches, int batchcnt)
{
	if(ctx->preambule_cnt < (72 + 32) )
	{
		// > 576 bits
		ctx->preambule_cnt++;
		return 0xAA;
	}

	if(ctx->frm_cnt < ( sizeof(poc_batch) * batchcnt ) )
	{
		uint8_t * ptr;

		ptr = (uint8_t *)batches;

		unsigned char val = *(ptr + ctx->frm_cnt);

		ctx->frm_cnt++;

		return val;
	}
	else
	{
		ctx->frm_end = 1;
		return 0x00;
	}


	return 0x00;
}

#define POCSAG_SYNC_CW 0x7CD215D8
#define POCSAG_IDLE_CW 0x7A89C197

void init_batch(poc_batch * batch)
{
	batch->sync = BIGENDIAN_DWORD( calc_pocsag_bch(POCSAG_SYNC_CW) );
	for(int i=0;i<16;i++)
	{
		batch->words[i] = BIGENDIAN_DWORD( calc_pocsag_bch(POCSAG_IDLE_CW) );
	}
}

int set_numerical_frame(poc_batch * batch, int batchcnt, unsigned int ric, unsigned int func, char * message)
{
	int ba,b,i,w,framecnt;
	int digitcnt;
	uint8_t charcode;
	uint32_t word;

	i = 0;
	while(message[i] != 0)
	{
		i++;
	}

	digitcnt = i;

	framecnt = digitcnt / 5;
	if( digitcnt % 5 )
		framecnt++;

	if( framecnt > batchcnt)
	{
		return -1;
	}

	ba = 0;
	w = (ric&7)*2;

	batch[ba].words[w++] = BIGENDIAN_DWORD( calc_pocsag_bch(adr_codeword(ric, func)));

	b = 0;

	digitcnt = 0;

	// 4 bits per digit - bits 30 <>11 -> 5 digits per code word
	word = 0x80000000;
	i = 0;
	while(message[i] != 0)
	{
		switch(message[i])
		{
			case 'U':
				charcode = 0xB;
			break;
			case ' ':
				charcode = 0xC;
			break;
			case '-':
				charcode = 0xD;
			break;
			case ']':
				charcode = 0xE;
			break;
			case '[':
				charcode = 0xF;
			break;
			default:
				if(message[i]>='0' && message[i] <= '9')
				{
					charcode = (message[i] - '0')&0xF;
				}
				else
				{
					charcode = 0xA;
				}
			break;
		}

		word |= (((uint32_t)(LUT_QuartetBitsInverter[charcode]))<<(27-((b%5)*4)) );

		b++;
		if(!(b%5))
		{
			batch[ba].words[w] = BIGENDIAN_DWORD( calc_pocsag_bch( word ) ); // Spaces

			w++;

			if( w >= 16)
			{
				w = 0;
				ba++;
			}

			word = 0x80000000;
		}
		i++;
	}

	if((b%5))
	{
		while((b%5))
		{
			word |= ( ((uint32_t)LUT_QuartetBitsInverter[0xC])<<(27-((b%5)*4)) );
			b++;
		}

		batch[ba].words[w] = BIGENDIAN_DWORD( calc_pocsag_bch( word ) );
	}

	if( ( batch[ba].words[15] != POCSAG_IDLE_CW ) && ( batch[ba].words[14] != POCSAG_IDLE_CW ) )
	{
		return ba+1+1;
	}
	else
	{
		return ba+1;
	}

	return ba+1;
}

int set_alphanum_frame(poc_batch * batch, int batchcnt, unsigned int ric, unsigned int func, char * message)
{
	int ba,b,i,w,framecnt;
	int digitcnt;
	uint8_t charcode;
	uint32_t word;
	unsigned char stream[80+1];
	int bitin,bitout;

	i = 0;
	while(message[i] != 0)
	{
		i++;
	}

	digitcnt = i;

	framecnt = (digitcnt*7) / 20;
	if( (digitcnt*7) % 20 )
		framecnt++;

	if( framecnt > batchcnt)
	{
		return -1;
	}

	memset(&stream,0,sizeof(stream));
	i = 0;
	bitin = 0;
	bitout = 0;
	while( message[bitin/7] && ((bitout>>3) < sizeof(stream)-1) )
	{
		if( LUT_ByteBitsInverter[message[bitin/7]<<1] & ( 0x40 >> (bitin%7) ) )
			stream[bitout>>3] |= ( 0x80 >> (bitout&7) );

		bitin++;
		bitout++;
	}

	stream[sizeof(stream)-1] = 0;

	digitcnt = bitout >> 2;
	if(bitout&3)
		digitcnt++;

	ba = 0;
	w = (ric&7)*2;

	batch[ba].words[w++] = BIGENDIAN_DWORD( calc_pocsag_bch(adr_codeword(ric, func)));

	b = 0;

	// 4 bits per digit - bits 30 <>11 -> 5 digits per code word
	word = 0x80000000;
	i = 0;
	while( i < digitcnt )
	{
		charcode = (stream[i>>1] >> (4*((i&1)^1)) & 0xF);
		if( charcode > 127 )
			charcode = ' ';

		word |= (((uint32_t)(charcode))<<(27-((b%5)*4)) );

		b++;
		if(!(b%5))
		{
			batch[ba].words[w] = BIGENDIAN_DWORD( calc_pocsag_bch( word ) ); // Spaces

			w++;

			if( w >= 16)
			{
				w = 0;
				ba++;
			}

			word = 0x80000000;
		}
		i++;
	}

	if((b%5))
	{
		while((b%5))
		{
			word |= ( ((uint32_t)0x0)<<(27-((b%5)*4)) );
			b++;
		}

		batch[ba].words[w] = BIGENDIAN_DWORD( calc_pocsag_bch( word ) );
	}

	if( ( batch[ba].words[15] != POCSAG_IDLE_CW ) && ( batch[ba].words[14] != POCSAG_IDLE_CW ) )
	{
		return ba+1+1;
	}
	else
	{
		return ba+1;
	}

	return ba+1;
}

#define MAXBATCHCNT 128

int main(int argc, char* argv[])
{
	char temp_str[512];
	unsigned int i,j;
	char message[512];
	int ric;
	int func;
	int baud;
	int smprate;
	int freqshift;
	int batchescnt;
	int alpha;
	int batchbruteforce;

	wave_io * wave1;

	iq_wave_gen iqgen;

	poc pocctx;
	poc_batch batches[MAXBATCHCNT];

	uint16_t iq_wave_buf[BUFFER_IQ_SAMPLES_SIZE];

	serial_gen ser;

	memset(&pocctx,0,sizeof(pocctx));

	stdoutmode = 0;
	if(isOption(argc,argv,"stdout",NULL)>0)
	{
		stdoutmode = 1;
	}

	baud = 1200;
	if(isOption(argc,argv,"baud",(char*)&temp_str)>0)
	{
		baud = atoi(temp_str);
	}

	ric = 8;
	if(isOption(argc,argv,"ric",(char*)&temp_str)>0)
	{
		ric = atoi(temp_str);
	}

	func = 0;
	if(isOption(argc,argv,"func",(char*)&temp_str)>0)
	{
		func = atoi(temp_str);
	}

	freqshift = 4500;
	if(isOption(argc,argv,"freqshift",(char*)&temp_str)>0)
	{
		freqshift = atoi(temp_str);
	}

	message[0] = 0;
	if(isOption(argc,argv,"message",(char*)&message)>0)
	{
	}

	alpha = 0;
	if(isOption(argc,argv,"alpha",NULL)>0)
	{
		alpha = 1;
	}

	smprate = IQ_SAMPLE_RATE;
	if(isOption(argc,argv,"smprate",(char*)&temp_str)>0)
	{
		smprate = atoi(temp_str);
	}

	batchbruteforce = 0;
	if(isOption(argc,argv,"batch",NULL)>0)
	{
		batchbruteforce = 1;
	}

	if(!stdoutmode)
	{
		printf("pocsag v0.0.1.1\n");
		printf("Copyright (C) 2026 Jean-Francois DEL NERO\n");
		printf("This program comes with ABSOLUTELY NO WARRANTY\n");
		printf("This is free software, and you are welcome to redistribute it\n");
		printf("under certain conditions;\n\n");
	}

	// help option...
	if(isOption(argc,argv,"help",0)>0)
	{
		printhelp(argv);
	}

	if(isOption(argc,argv,"generate",0)>0)
	{
		// IQ Modulator
		iqgen.phase = 0;
		iqgen.Frequency = CENTRAL_IF_FREQ;
		iqgen.Amplitude = 120;
		iqgen.sample_rate = smprate;

		if(stdoutmode)
		{
			// stdout / stream mode : IQ are outputed to the stdout -> use a pipe to hackrf_transfer
			wave1 = create_wave(NULL,iqgen.sample_rate,WAVE_FILE_FORMAT_RAW_8BITS_IQ);
		}
		else
		{
			// file mode : create iq + wav files
			wave1 = create_wave("out.sdriq",iqgen.sample_rate,WAVE_FILE_FORMAT_SDRIQ_8BITS_IQ);//WAVE_FILE_FORMAT_WAV_8BITS_STEREO);
		}

		if(wave1)
		{
			serial_init(&ser, 8, smprate, baud);

			memset(&pocctx,0,sizeof(pocctx));

			for(i=0;i<MAXBATCHCNT;i++)
			{
				init_batch((poc_batch *)&batches[i]);
			}

			if(batchbruteforce)
			{
				int ricbatch;

				ricbatch = ric & ~7;

				for(j=0;j<16;j++)
				{
					for(i=0;i<8;i++)
					{
						if( alpha )
							batchescnt =  set_alphanum_frame((poc_batch *)&batches[j], MAXBATCHCNT-1, ricbatch+i, func&3, (char*)&message);
						else
							batchescnt = set_numerical_frame((poc_batch *)&batches[j], MAXBATCHCNT-1, ricbatch+i, func&3, (char*)&message);
					}
					ricbatch += 8;
				}
				batchescnt += j;
			}
			else
			{
				if( alpha )
					batchescnt =  set_alphanum_frame((poc_batch *)&batches, MAXBATCHCNT-1, ric, func&3, (char*)&message);
				else
					batchescnt = set_numerical_frame((poc_batch *)&batches, MAXBATCHCNT-1, ric, func&3, (char*)&message);
			}

			if( batchescnt < 0 )
				exit(1);

			// Blank
			i = 0;
			while(i < (smprate/20)) // ~ 50 ms
			{
				for(j=0;j<BUFFER_IQ_SAMPLES_SIZE;j++)
				{
					iq_wave_buf[j] = 0;
				}
				write_wave(wave1, &iq_wave_buf,BUFFER_IQ_SAMPLES_SIZE);
				i += BUFFER_IQ_SAMPLES_SIZE;
			}

			// Main loop :  Generate the message
			while( !pocctx.frm_end || !serial_is_tx_reg_empty(&ser) )
			{
				j = 0;
				while( ( j < BUFFER_IQ_SAMPLES_SIZE ) && ( !pocctx.frm_end || !serial_is_tx_reg_empty(&ser) ) )
				{
					if(serial_is_tx_reg_empty(&ser))
					{
						unsigned char tmp = pocsag_nextbyte(&pocctx, (poc_batch *)&batches, batchescnt);
						if(!pocctx.frm_end)
							serial_tx_settxreg(&ser,tmp);
					}

					iqgen.Frequency = ((double)CENTRAL_IF_FREQ + ( freqshift + ( (serial_tx_getlinestate(&ser) ) * ( -freqshift * 2 ) ) ) );

					iq_wave_buf[j] = get_next_iq(&iqgen);
					j++;
				}

				write_wave(wave1, &iq_wave_buf,BUFFER_IQ_SAMPLES_SIZE);
			}

			// Blank
			i = 0;
			while(i < (smprate/20)) // ~ 50 ms
			{
				for(j=0;j<BUFFER_IQ_SAMPLES_SIZE;j++)
				{
					iq_wave_buf[j] = 0;
				}
				write_wave(wave1, &iq_wave_buf,BUFFER_IQ_SAMPLES_SIZE);
				i+= BUFFER_IQ_SAMPLES_SIZE;
			}

			close_wave(wave1);
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
