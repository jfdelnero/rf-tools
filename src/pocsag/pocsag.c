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
#include <math.h>

#include <stdint.h>

#include "cmd_param.h"

#include "wave.h"
#include "modulator.h"
#include "serial.h"
#include "utils.h"

#define IQ_SAMPLE_RATE            2000000
#define CENTRAL_IF_FREQ           0

//#define DEBUG 1

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

void printhelp(char* argv[])
{
	printf("Options:\n");
	printf("  -stdout \t\t\t: IQ stream send to stdout\n");
	printf("  -generate \t\t\t: Generate the IQ stream\n");
	printf("  -baud:[BAUD]\t\t\t: Baud rate setting (Default:1200)\n");
	printf("  -ric:[RIC]\t\t\t: RIC address (Default:8)\n");
	printf("  -func:[function_code]\t\t: Function code (Default:0 Min/Max:0-3)\n");
	printf("  -freqshift:[Hz]\t\t: FSK frequency shift (Default:4500 -> +4500Hz / -4500Hz)\n");
	printf("  -smprate:[Hz]\t\t\t: IQ Sample rate (Default:2000000)\n");
	printf("  -alpha\t\t\t: Alphanumeric mode (Default: Numeric mode)\n");
	printf("  -message:message_txt\t\t: Message to send\n");
	printf("  -fmessage:message_file.txt\t: File message to send\n");
	printf("  -stdin_message\t\t: Message to send from stdin\n");
	printf("  -level:[0-127]\t\t: Output level (Default: 126)\n");
	printf("  -settle_time:[0-100]\t\t: Frequency change settle time, in percent of a bit period (Default: 20)\n");
	printf("  -bits_stream_out\t\t: Enable bits stream output\n");
	printf("  -verbose \t\t\t: Verbose mode\n");
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

	if( ( batch[ba].words[15] != BIGENDIAN_DWORD( calc_pocsag_bch(POCSAG_IDLE_CW) ) ) )
	{
		return ba+2;
	}
	else
	{
		return ba+1;
	}

	return ba+1;
}

int set_alphanum_frame(poc_batch * batch, int batchcnt, unsigned int ric, unsigned int func, char * message, int message_size)
{
	int ba,b,i,w,framecnt;
	int digitcnt;
	uint8_t charcode;
	uint32_t word;
	unsigned char stream[80+1];
	int bitin,bitout;

	digitcnt = message_size;

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
	while( ((bitin/7)<message_size) && ((bitout>>3) < sizeof(stream)-1) )
	{
		if( LUT_ByteBitsInverter[(message[bitin/7]&0x7F)<<1] & ( 0x40 >> (bitin%7) ) )
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

	if( ( batch[ba].words[15] != BIGENDIAN_DWORD( calc_pocsag_bch(POCSAG_IDLE_CW) ) ) )
	{
		return ba+2;
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
	int wavout;
	int settle_time;
	int settle_size;
	int freqidx;
	int * settle_buf;
	double volinc;
	int level;
	int verbose;
	int message_size;
	int ser_state;
	int data_stream_mode;
	int quiet;

	wave_io * wave1;

	iq_wave_gen iqgen;

	poc pocctx;
	poc_batch batches[MAXBATCHCNT];

	uint16_t iq_wave_buf[BUFFER_IQ_SAMPLES_SIZE];

	serial_gen ser;

	quiet = 0;

	message_size = 0;

	memset(&pocctx,0,sizeof(pocctx));

	stdoutmode = 0;
	if(isOption(argc,argv,"stdout",NULL,NULL)>0)
	{
		stdoutmode = 1;
	}

	baud = 1200;
	if(isOption(argc,argv,"baud",(char*)&temp_str,NULL)>0)
	{
		baud = atoi(temp_str);
	}

	ric = 8;
	if(isOption(argc,argv,"ric",(char*)&temp_str,NULL)>0)
	{
		ric = atoi(temp_str);
	}

	func = 0;
	if(isOption(argc,argv,"func",(char*)&temp_str,NULL)>0)
	{
		func = atoi(temp_str);
	}

	freqshift = 4500;
	if(isOption(argc,argv,"freqshift",(char*)&temp_str,NULL)>0)
	{
		freqshift = atoi(temp_str);
	}

	data_stream_mode = 0;
	if(isOption(argc,argv,"bits_stream_out",NULL,NULL)>0)
	{
		data_stream_mode = 1;
		stdoutmode = 0;
	}

	message[0] = 0;
	if(isOption(argc,argv,"message",(char*)&message,NULL)>0)
	{
		message_size = strlen(message);
	}

	temp_str[0] = 0;
	if(isOption(argc,argv,"fmessage",(char*)&temp_str,NULL)>0)
	{
		if(strlen(temp_str))
		{
			FILE * f;
			int size;

			message_size = 0;
			f = fopen(temp_str,"rb");
			if(f)
			{
				memset( message,0, sizeof(message));

				fseek(f, 0, SEEK_END);

				size = ftell(f);

				if(size >= sizeof(message))
					size = sizeof(message) - 1;

				fseek(f, 0, SEEK_SET);
				if(fread(&message,size,1,f) != 1 )
				{
					fprintf(stderr,"Error : Can't read %s ...\n",temp_str);
				}
				else
				{
					message_size = size;
				}

				fclose(f);
			}
			else
			{
				fprintf(stderr,"Error : Can't open %s ...\n",temp_str);
			}
		}
	}

	if(isOption(argc,argv,"stdin_message",NULL,NULL)>0)
	{
		memset( message,0, sizeof(message));
		if(fgets(message, sizeof(message), stdin))
		{
			if (message[strlen(message)-1] == '\n')
			{
				message[strlen(message)-1] = 0;
			}
			message_size = strlen(message);
		}
	}

	alpha = 0;
	if(isOption(argc,argv,"alpha",NULL,NULL)>0)
	{
		alpha = 1;
	}

	smprate = IQ_SAMPLE_RATE;
	if(isOption(argc,argv,"smprate",(char*)&temp_str,NULL)>0)
	{
		smprate = atoi(temp_str);
	}

	batchbruteforce = 0;
	if(isOption(argc,argv,"batch",NULL,NULL)>0)
	{
		batchbruteforce = 1;
	}

	wavout = 0;
	if(isOption(argc,argv,"wav",NULL,NULL)>0)
	{
		wavout = 1;
	}

	settle_time = 20;
	if(isOption(argc,argv,"settle_time",(char*)&temp_str,NULL)>0)
	{
		settle_time = atoi(temp_str);
		if( settle_time < 0 )
			settle_time = 0;

		if( settle_time > 100 )
			settle_time = 100;
	}

	level = 126;
	if(isOption(argc,argv,"level",(char*)&temp_str,NULL)>0)
	{
		level = atoi(temp_str);
		if( level < 0 )
			level = 0;

		if( level > 127 )
			level = 127;
	}

	verbose = 0;
	if(isOption(argc,argv,"verbose",NULL,NULL)>0)
	{
		verbose = 1;
	}

	quiet = 0;
	if(isOption(argc,argv,"quiet",NULL,NULL)>0)
	{
		quiet = 1;
	}

	if(!stdoutmode && !quiet)
	{
		printf("pocsag v0.0.1.1\n");
		printf("Copyright (C) 2026 Jean-Francois DEL NERO\n");
		printf("This program comes with ABSOLUTELY NO WARRANTY\n");
		printf("This is free software, and you are welcome to redistribute it\n");
		printf("under certain conditions;\n\n");
	}

	// help option...
	if(isOption(argc,argv,"help",0,NULL)>0)
	{
		printhelp(argv);
	}

	settle_size = (int) ( (float)smprate / (float)baud*2 ) * ((float)settle_time/100.0);
	if(settle_size<2)
		settle_size = 2;


	if(isOption(argc,argv,"generate",0,NULL)>0)
	{
		if(!quiet)
			fprintf(stderr,"\nBaud:%d, RIC: %d, Function:%d, Alpha:%d, Message:%s\n", baud, ric, func, alpha, message );

		settle_buf = calloc( 1, settle_size * sizeof(int) );
		if(!settle_buf)
			exit(-1);

		for(i=0;i<settle_size;i++)
		{
			settle_buf[i] = cos( (double)i * ( (double)M_PI / (double)(settle_size-1)) ) * freqshift;
		}

		// IQ Modulator
		iqgen.phase = 0;
		iqgen.Frequency = CENTRAL_IF_FREQ;
		iqgen.Amplitude = 0;
		iqgen.sample_rate = smprate;

		wave1 = NULL;

		if( !data_stream_mode )
		{
			if(stdoutmode)
			{
				// stdout / stream mode : IQ are outputed to the stdout -> use a pipe to hackrf_transfer
				wave1 = create_wave(NULL,iqgen.sample_rate,WAVE_FILE_FORMAT_RAW_8BITS_IQ);
			}
			else
			{
				// file mode : create iq + wav files
				if(wavout)
					wave1 = create_wave("out.wav",iqgen.sample_rate,WAVE_FILE_FORMAT_WAV_8BITS_STEREO);
				else
					wave1 = create_wave("out.sdriq",iqgen.sample_rate,WAVE_FILE_FORMAT_SDRIQ_8BITS_IQ);
			}
		}

		if(wave1 || data_stream_mode)
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
							batchescnt =  set_alphanum_frame((poc_batch *)&batches[j], MAXBATCHCNT-1, ricbatch+i, func&3, (char*)&message, message_size);
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
					batchescnt =  set_alphanum_frame((poc_batch *)&batches, MAXBATCHCNT-1, ric, func&3, (char*)&message, message_size);
				else
					batchescnt = set_numerical_frame((poc_batch *)&batches, MAXBATCHCNT-1, ric, func&3, (char*)&message);
			}

			if(verbose)
			{
				fprintf(stderr,"\nFrames/batches data:\n");
				unsigned char * ptr;
				ptr = (unsigned char *)&batches;
				for(i=0;i<sizeof(poc_batch) * batchescnt;i++)
				{
					if(!(i&0xF))
						fprintf(stderr,"\n");
					fprintf(stderr,"%.2X ",*ptr++);
				}
				fprintf(stderr,"\n");
			}

			if( batchescnt < 0 )
			{
				free(settle_buf);
				exit(1);
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
				i += BUFFER_IQ_SAMPLES_SIZE;
			}

			freqidx = settle_size / 2;

			// Initial 750Hz tone
			serial_init(&ser, 8, smprate, 750*2); // 750 Hz tone

			iqgen.Amplitude = 0;

			// Fade in
			volinc = (float)level / (float)(smprate/60);

			i = 0;
			while(i < (smprate/5)) // ~ 200 ms
			{
				for(j=0;j<BUFFER_IQ_SAMPLES_SIZE;j++)
				{
					if(iqgen.Amplitude+volinc <= level)
						iqgen.Amplitude += volinc;

					if(serial_is_tx_reg_empty(&ser))
					{
						serial_tx_settxreg(&ser,0xAA);
					}

					ser_state = serial_tx_getlinestate(&ser);

					if( ser_state & 1 )
					{
						if(freqidx < settle_size - 1)
							freqidx++;
					}
					else
					{
						if(freqidx)
							freqidx--;
					}

					iqgen.Frequency = (double)CENTRAL_IF_FREQ + settle_buf[freqidx];
					iq_wave_buf[j] = get_next_iq(&iqgen);
				}

				write_wave(wave1, &iq_wave_buf,BUFFER_IQ_SAMPLES_SIZE);
				i += BUFFER_IQ_SAMPLES_SIZE;
			}

			serial_init(&ser, 8, smprate, baud);

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

					ser_state = serial_tx_getlinestate(&ser);

					if( ser_state & 1 )
					{
						if(freqidx < settle_size - 1)
							freqidx++;
					}
					else
					{
						if(freqidx)
							freqidx--;
					}

					if( (ser_state & 0x2) && data_stream_mode )
					{
						printf("%d", ser_state & 1);
					}

					iqgen.Frequency = (double)CENTRAL_IF_FREQ + settle_buf[freqidx];
					iq_wave_buf[j] = get_next_iq(&iqgen);
					j++;
				}

				write_wave(wave1, &iq_wave_buf,j);
			}

			// Fade out + Blank
			i = 0;
			volinc = iqgen.Amplitude / (smprate/40);
			while(i < (smprate/40)) // ~ 50 ms
			{
				for(j=0;j<BUFFER_IQ_SAMPLES_SIZE;j++)
				{
					if(iqgen.Amplitude >= volinc)
						iqgen.Amplitude -= volinc;
					else
						iqgen.Amplitude = 0;

					ser_state = serial_tx_getlinestate(&ser);

					if( ser_state & 1 )
					{
						if(freqidx < settle_size - 1)
							freqidx++;
					}
					else
					{
						if(freqidx)
							freqidx--;
					}

					iqgen.Frequency = (double)CENTRAL_IF_FREQ + settle_buf[freqidx];
					iq_wave_buf[j] = get_next_iq(&iqgen);
				}
				write_wave(wave1, &iq_wave_buf,BUFFER_IQ_SAMPLES_SIZE);
				i+= BUFFER_IQ_SAMPLES_SIZE;
			}

			close_wave(wave1);
		}

		free(settle_buf);
	}

	if( (isOption(argc,argv,"help",0,NULL)<=0) &&
		(isOption(argc,argv,"generate",0,NULL)<=0)
		)
	{
		printhelp(argv);
	}

	return 0;
}
