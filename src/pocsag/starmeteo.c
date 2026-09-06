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
// File : starmeteo.c
// Contains: star meteo frame encoder / decoder
//
// This file is part of rf-tools.
//
// Written by: Jean-François DEL NERO
//
// Copyright (C) 2026 Jean-François DEL NERO
//
// You are free to do what you want with this code.
// A credit is always appreciated if you use it into your product :)
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
//
// Star Météo protocol :
//
// The Star Météo protocol was rebuilt based on multiple observations, tests
// and team discussions with multiple people (ChrisJ, Jaymore, JeffHxC, obones, ...)
// on the TetraHub Forum, the largest French language online community and discussion
// board dedicated to radiocommunications, radio scanning, and digital radio systems
//
// Discussion starting point : https://forum.tetrahub.net/post91796.html#p91796
//
// Note : The following description use some VHDL syntaxes for the bitfields ("X downto x")
// ----
//
// Transport :
//
// POCSAG : 466.20625MHz, FSK (+/-4.5KHz, 0:+4.5KHz, 1:-4.5KHz), 1200 Bauds, 7 bits
//          RIC:25176, Alpha, Function:3
//
// Quartets Encoding :
//
//    Valid POCSAG Alpha characters :
//
//    0x20<>0x22,
//    0x24<>0x3F
//    0x41(A)<>0x5A(Z)
//    0x6B(k)<>0x70(p)
//    0x73(s)
//
// POCSAG Alpha character (c) to 6 bits raw conversion :
//    if c >= 'k' and c <= 'o' 6bitsraw = 59 + ( c - 'k' );
//    if c == 'p' 6bitsraw = 0x20;
//    if c == 's' 6bitsraw = 0x03;
//    else 6bitsraw = c - 0x20;
//
// Quartets output :
//    2 * 6 bits raw to conversion to 3 quartets
//    |543210||543210|... -> |5432|1054|3210|...
//
// -------------------------------------------------------------------------------
// Time Frame :
//
// | CURRENT_TIME_BLOCK | AREAS_ID_BLOCK |
//
// CURRENT_TIME_BLOCK :
//
// [0xF][HOUR][MINUTES_HIGH_BCD][MINUTES_LOW_BCD][MONTH][MONTH_DAY_YEAR_0][MONTH_DAY_YEAR_1][MONTH_DAY_YEAR_2][LOW_CHECKSUM]
//
// HOUR quartet encoding :
//
//    if hour <  10hour    then HOUR <= hour
//                         else HOUR <= hour - 10
//
// MINUTES_HIGH_BCD quartet encoding :
//
//    if hour >= 10hour and hour <= 19hour then MINUTES_HIGH_BCD <= (minutes high BCD digit) + 10;
//                                         else MINUTES_HIGH_BCD <= (minutes high BCD digit);
//
// MINUTES_LOW_BCD quartet encoding :
//
//    MINUTES_LOW_BCD <= (minutes low BCD digit)
//
// MONTH quartet encoding :
//
//    MONTH <= month - 1
//
// MONTH_DAY_YEAR_0,MONTH_DAY_YEAR_1,MONTH_DAY_YEAR_2 quartets encoding
//
//    MONTH_DAY_YEAR_0(3 downto 2) <= (month day high BCD digit)(1 downto 0)
//    MONTH_DAY_YEAR_0(1 downto 0) <= (month day low BCD digit) (3 downto 2)
//    MONTH_DAY_YEAR_1(3 downto 2) <= (month day low BCD digit) (1 downto 0)
//    MONTH_DAY_YEAR_1(1 downto 0) <= (year - 2000)(5 downto 4)
//    MONTH_DAY_YEAR_2             <= (year - 2000)(3 downto 0)
//
// LOW_CHECKSUM quartet encoding :
//
//    LOW_CHECKSUM <= 0x7 + sum of all previous quartets (from quartet [0xF] to [MONTH_DAY_YEAR_2])
//
// ----
//
// AREAS_ID_BLOCK :
//
// Warning : Most of the data in this frame/block are not aligned to the quartets.
//
// [0x0][0x0][0x0]<TRANSMISSIONS_INTERVAL (5 bits)><NUMBER_OF_AREAS (5 bits)><AREAS_ID_LIST (NUMBER_OF_AREAS*7 Bits)>[RFU][HIGH_CHECKSUM][LOW_CHECKSUM][0][0]
//
// TRANSMISSIONS_INTERVAL encoding :
//
//    5 bits length
//    TRANSMISSIONS_INTERVAL <= (minutes between the forecast transmissions)
//
//    The station listening reference times are 00:00 AM, 06:00 AM, 12:00 PM and 06:00 PM.
//    The listening 4 minutes window start time is : reference time + (TRANSMISSIONS_INTERVAL * AREA_INDEX).
//    example : The Area list is : 75 91 78 93 92
//             With TRANSMISSIONS_INTERVAL set to 12, if the station is set the to '93' area (index 3), the listening minutes is TRANSMISSIONS_INTERVAL * 3 = 36.
//             So the station will listen the radio at 0h36, 6h36, 12h36 and 18h36.
//
// NUMBER_OF_AREAS encoding :
//
//    5 bits length
//    NUMBER_OF_AREAS <= (number of 7 bits areas id following this field)
//
// AREAS_ID_LIST :
//
//    7 bits per ID. Total size : (7 bits * NUMBER_OF_AREAS) + pad to align the end of the list to the next quartet.
//    Pad the last list quartet with 0.
//
//    AREAS_ID_LIST[x] <= French area ID (example : 75 = Paris)
//
// RFU :
//
//    Filling quartet. Can be 0x1, 0x5, 0x7...
//
// HIGH_CHECKSUM quartet encoding :
//
//    HIGH_CHECKSUM <= (0x7 + sum of all previous quartets (from quartet [0x0] to [RFU]))(7 downto 4)
//
// LOW_CHECKSUM quartet encoding :
//
//    LOW_CHECKSUM  <= (0x7 + sum of all previous quartets (from quartet [0xF] to [RFU]))(3 downto 0)
//
// -------------------------------------------------------------------------------
// Forecasts frame
//
//    | HEADER_BLOCK (6 quartets) | n+0 FORECAST_BLOCK (15 quartets) | n+1 FORECAST_BLOCK (15 quartets) | ... | n+5 FORECAST_BLOCK (15 quartets) | RAIN_PROBABILITY_BLOCK (if TYPE==0x0) |
//
// ----
//
// HEADER_BLOCK :
//
//    [TYPE][AREA_ID_HIGH][AREA_ID_LOW][ALERT][0x4][LOW_CHECKSUM]
//
// TYPE encoding :
//    0x4 : Forecast without the rain probability forecast
//    0x0 : Extended forecast with rain probability
//
// AREA_ID_HIGH encoding :
//
//    AREA_ID_HIGH <= (area id)(7 downto 4)
//
// AREA_ID_LOW encoding :
//
//    AREA_ID_LOW <= (area id)(3 downto 0)
//
// Note : Default area id used by the station : 75 (Paris).
//
// LOW_CHECKSUM quartet encoding :
//
//    LOW_CHECKSUM <= 0x7 + sum of all previous quartets (from quartet [TYPE] to [0x4])
//
// ----
//
// FORECAST_BLOCK :
//
//    [LOW_TEMP_HIGH_BCD][LOW_TEMP_LOW_BCD][HIGH_TEMP_HIGH_BCD][HIGH_TEMP_LOW_BCD] ...
//    [PICTO_CODE_HIGH_0][PICTO_CODE_LOW_0][PICTO_CODE_HIGH_1][PICTO_CODE_LOW_1][PICTO_CODE_HIGH_2][PICTO_CODE_LOW_2][PICTO_CODE_HIGH_3][PICTO_CODE_LOW_3][PICTO_CODE_HIGH_4][PICTO_CODE_LOW_5][LOW_CHECKSUM]
//
// LOW_TEMP_HIGH_BCD encoding :
//
//    LOW_TEMP_HIGH_BCD <= (temperature + 40) high BCD digit
//
// LOW_TEMP_LOW_BCD encoding :
//
//    LOW_TEMP_LOW_BCD <= (temperature + 40) low BCD digit
//
// PICTO_CODE_HIGH_X encoding :
//
//    PICTO_CODE_HIGH_X(3 downto 2) <= text_id(1 downto 0)
//    PICTO_CODE_HIGH_X(1 downto 0) <= picto_id(5 downto 4)
//
// PICTO_CODE_LOW_X encoding :
//
//    PICTO_CODE_LOW_X <= picto_id(3 downto 0)
//
// LOW_CHECKSUM quartet encoding :
//
//    LOW_CHECKSUM <= 0x7 + sum of all previous quartets (from quartet [LOW_TEMP_HIGH_BCD] to [PICTO_CODE_LOW_5])
//
// ----
//
// RAIN_PROBABILITY_BLOCK :
//
// TODO
//

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

#include "cmd_param.h"

#define MAX_MSG_SIZE 512

typedef struct frame_
{
	unsigned char frm[MAX_MSG_SIZE];
	unsigned char dcodefrm[MAX_MSG_SIZE];
	unsigned char quartetfrm[MAX_MSG_SIZE*3];
	int quartets_cnt;
}frame;


void printbin(uint32_t val,int bitcnt)
{
	for(int j=(bitcnt-1);j>=0;j--)
	{
		printf("%d", (val>>j)&1);
	}
}

// Convert pocsag characters to 6 bits raw word (decoding)
unsigned char char2raw(unsigned char c)
{
	if( c >= 'k' && c <= 'o')
		return 59+(c-'k');

	switch( c )
	{
		case 'p':
			return 0x20;
		break;
		case 's':
			return 0x03;
		break;
	}

	c = c - 0x20;

	return c;
}

// Convert 6 bits word to char. (encoding)
unsigned char raw2char(unsigned char r)
{
	if( r >= 59 && r <= 63 )
		return 'k' + (r - 59);

	switch( r )
	{
		case 0x20:
			return 'p';
		break;
		case 0x03:
			return 's';
		break;
	}

	r = r + 0x20;

	return r;
}

// Get quartet from a decoded 6 bits words array
unsigned char get_quartet( unsigned char * buf, int idx)
{
	int i,j,bitidx;
	unsigned char val;

	bitidx = (idx * 4);
	val = 0;
	j = 0;
	while(j<4)
	{
		i = bitidx / 6;

		val = val << 1;
		if( (buf[i] >> (5-(bitidx%6))) & 1 )
		{
			val |= 1;
		}

		bitidx++;
		j++;
	}

	return val;
}

// Set quartet to a decoded 6 bits words array
void set_quartet( unsigned char * buf, int idx, unsigned char q)
{
	int i,j,bitidx;

	bitidx = (idx * 4);
	j = 0;
	while(j<4)
	{
		i = bitidx / 6;

		if( q & (0x8>>j) )
		{
			buf[i] = buf[i] | ( 0x01 << (5-(bitidx%6)));
		}
		else
		{
			buf[i] = buf[i] & ~( 0x01 << (5-(bitidx%6)));
		}

		bitidx++;
		j++;
	}

	return;
}

uint32_t get_field(unsigned char * quartets_array, int bitidx, int fieldsize, int quartets_array_size)
{
	int j;
	uint32_t val;

	val = 0;
	j = 0;
	while(j<fieldsize && (bitidx >> 2) < quartets_array_size )
	{
		val <<= 1;

		if ( quartets_array[bitidx >> 2] & (0x8 >> (bitidx&3)) )
		{
			val |= 0x1;
		}

		bitidx++;
		j++;
	}

	return val;
}

int set_field(unsigned char * quartets_array, int bitidx, int fieldsize, int quartets_array_size, uint32_t data)
{
	int j;
	uint32_t val;

	val = 0;
	j = 0;
	while(j<fieldsize && (bitidx >> 2) < quartets_array_size )
	{
		val <<= 1;

		if(data >> ( (fieldsize - j) -1 ) & 1 )
		{
			quartets_array[bitidx >> 2] |=  (0x8 >> (bitidx&3));
		}
		else
		{
			quartets_array[bitidx >> 2] &= ~(0x8 >> (bitidx&3));
		}

		bitidx++;
		j++;
	}

	return bitidx;
}

// Encode temperature to BCD+40
unsigned char dectemp_to_bcd(int temp)
{
	temp += 40;
	return (((temp / 10)&0xF) << 4) | ((temp%10) & 0xF);
}

// Generate and encode current time frame.
int gen_current_time(unsigned char * quartets)
{
	int i;
	time_t t = time(NULL);
	struct tm tm;

	tm = *localtime(&t);

	quartets[0] = 0xF;

	if( tm.tm_hour < 10)
	{
		quartets[1] = tm.tm_hour;
		quartets[2] = tm.tm_min/10;
		quartets[3] = tm.tm_min%10;
	}
	else
	{
		if( tm.tm_hour >= 10 &&  tm.tm_hour <= 19  )
		{
			quartets[1] = tm.tm_hour - 10;
			quartets[2] = (tm.tm_min/10) + 10;
			quartets[3] = tm.tm_min%10;
		}
		else
		{
			//20 <> 23
			quartets[1] = tm.tm_hour - 10;
			quartets[2] = (tm.tm_min/10);
			quartets[3] = tm.tm_min%10;
		}
	}

	quartets[4] =  tm.tm_mon + 1;
	quartets[5] =  ( ((tm.tm_mday/10)&3)<<2 ) | ( (((tm.tm_mday%10)>>2)&3) );
	quartets[6] =  ( ((tm.tm_mday%10)&3) )<<2;
	quartets[6] |= ( ((tm.tm_year-100)>>4) & 0x3);
	quartets[7] =  ( ((tm.tm_year-100)) & 0xF);

	int sum = 0x7;
	i = 0;
	while(i<8)
	{
		sum += quartets[i];
		i++;
	}
	quartets[8] = sum & 0xF;

	return 9;
}

// Generate and encode the areas ids array.
int gen_area_ids(unsigned char * quartets)
{
	int i,j,bitidx;

	i = 0;
	quartets[i++] = 0x0;
	quartets[i++] = 0x0;
	quartets[i++] = 0x0;

	bitidx = i << 2;

	// 12 Minutes interval
	bitidx = set_field(quartets, bitidx, 5, MAX_MSG_SIZE*3, 12);

	// Set 8 default regions ...
	bitidx = set_field(quartets, bitidx, 5, MAX_MSG_SIZE*3, 8);

	bitidx = set_field(quartets, bitidx, 7, MAX_MSG_SIZE*3, 75);
	bitidx = set_field(quartets, bitidx, 7, MAX_MSG_SIZE*3, 77);
	bitidx = set_field(quartets, bitidx, 7, MAX_MSG_SIZE*3, 78);
	bitidx = set_field(quartets, bitidx, 7, MAX_MSG_SIZE*3, 91);
	bitidx = set_field(quartets, bitidx, 7, MAX_MSG_SIZE*3, 92);
	bitidx = set_field(quartets, bitidx, 7, MAX_MSG_SIZE*3, 93);
	bitidx = set_field(quartets, bitidx, 7, MAX_MSG_SIZE*3, 94);
	bitidx = set_field(quartets, bitidx, 7, MAX_MSG_SIZE*3, 95);

	// Aligment to the next quartet
	if(bitidx&3)
		bitidx = set_field(quartets, bitidx, (4 - (bitidx&3)), MAX_MSG_SIZE*3, 0);

	i = bitidx >> 2;

	quartets[i++] = 0x1; // Not sure yet about the meaning of this quartet

	int sum = 0x7;
	j = 0;
	while(j<i)
	{
		sum += quartets[j];
		j++;
	}

	quartets[i++] = (sum >> 4) & 0xF;
	quartets[i++] = (sum     ) & 0xF;

	quartets[i++] = 0x0;
	quartets[i++] = 0x0;

	return i;
}

int gen_forecast(unsigned char * quartets, char * params)
{
	char *tmp_ptr,*tmp2_ptr;
	char tmp2[512];
	unsigned char b;
	int params_dec[32];
	int i,j;

	memset(params_dec,0,sizeof(params_dec));

	// ltemp_0,htemp_0,pic_0,pic_1,pic_2,pic_3,pic_4

	i = 0;
	tmp_ptr = params;
	while( (tmp2_ptr = strchr(tmp_ptr,',')) && i < 16)
	{
		memset(tmp2,0,sizeof(tmp2));
		strncpy(tmp2,tmp_ptr,tmp2_ptr - (char*)tmp_ptr);
		params_dec[i++] = atoi(tmp2);
		tmp_ptr = tmp2_ptr + 1;
	}
	params_dec[i] = atoi(tmp_ptr);

	if(i<2)
		i = 2;

	j = i;

	while(i<8)
	{
		params_dec[i] = params_dec[j];
		i++;
	}

	//for(int j=0;j<i;j++)
	//  printf(">> %d\n",params_dec[j]);

	// Low temp
	b = dectemp_to_bcd(params_dec[0]);
	quartets[0] = (b>>4);
	quartets[1] = (b&0xF);

	// High temp
	b = dectemp_to_bcd(params_dec[1]);
	quartets[2] = (b>>4);
	quartets[3] = (b&0xF);

	// Pictos
	for(j=0;j<5;j++)
	{
		quartets[4+(j*2)]     = (params_dec[2 + j] >> 4);
		quartets[4+(j*2) + 1] = (params_dec[2 + j] & 0xF);
	}

	// Checksum
	quartets[14] = 7;
	for(i=0;i<14;i++)
	{
		quartets[14] += quartets[i];
	}

	return 15;
}

// Load and decode a frame
int loadfrm(frame * frm, int idx, char *path)
{
	//int size;
	FILE *f;
	char str[512];
	char * strtmp;
	int len,i;

	//printf("%s\n",path);

	f = fopen(path,"rb");
	if(f)
	{
	//  fseek(f,0,SEEK_END);
	//  size = ftell(f);
		fseek(f,0,SEEK_SET);

		memset((char*)str,0,sizeof(str));

		if ( !fgets ((char*)&str, sizeof(str), f) )
		{
			fclose(f);
			frm[idx].frm[0] = '\0';
			frm[idx].quartets_cnt = 0;
			return 0;
		}

		strtmp = strchr(str,'\n');
		if(strtmp)
			*strtmp = 0;

		len = strlen((char*)str);

		if( 1 )
		{
			strcpy((char*)&frm[idx].frm,str);

			// chars to 6 bits words
			i = 0;
			while( frm[idx].frm[i] )
			{
				frm[idx].dcodefrm[i] = char2raw(frm[idx].frm[i]);
				i++;
			}

			// 6 bits words to quartets
			i = 0;
			while( i < (len*6)/4 )
			{
				frm[idx].quartetfrm[i] = get_quartet( (unsigned char*)(&frm[idx].dcodefrm), i);

				i++;
			}

			frm[idx].quartets_cnt = i;
			idx++;
		}

		fclose(f);
	}

	return idx;
}

int main(int argc, char* argv[])
{
	int i,idx,fidx;
	int quiet,verbose;
	int rpitx_outmode;
	int param_start_index;
	frame * genfrm;
	int sum;
	char tmp_str[512];
	int forecast_cnt;
	int prev_cnt,ck;

	verbose = 0;
	if(isOption(argc, argv,"verbose",NULL, NULL) )
	{
		verbose = 1;
	}

	quiet = 0;
	if(isOption(argc, argv,"quiet",NULL, NULL) )
	{
		quiet = 1;
	}

	if(!quiet)
		printf("startmeteo v0.2 -help format command line syntax.\n");

	if(isOption(argc, argv,"help",NULL, NULL) )
	{
		printf("Syntax:\n");
		printf("%s -decode [files]\n",argv[0]);
		printf("%s -encode:[HEX Quartets]\n",argv[0]);
		printf("%s -checksum     (Update checksum with \"-encode\")\n",argv[0]);
		printf("%s -curtime      Generate current date/hour frame\n",argv[0]);
		printf("%s -forecast:[LowTemp],[HighTemp],[MainPicto],[Picto_2],[Picto_3],[Picto_4],[Picto_5]\n",argv[0]);
		printf("%s -areaid:[idcode]\n",argv[0]);
		printf("%s -rpitx        rpitx output mode (RIC and function code specified at the string start)\n",argv[0]);
		printf("%s -quiet\n",argv[0]);
		printf("%s -verbose\n",argv[0]);
		printf("\n");
		printf("Example: %s -decode ../previsions_ok/*.txt\n",argv[0]);
		printf("Example: %s -curtime -quiet\n",argv[0]);
		printf("Example: %s -forecast:-10,40,1,2,3,4,5 -forecast:-11,41,6,7,8,9,10 -forecast:-12,42,11,12,13,14,15 -forecast:-13,43,16,17,18,19,20 -areaid:75 -quiet\n",argv[0]);
		printf("Example: starmeteo + rf-tools pocsag + hackrf :\n");
		printf("         ./starmeteo -curtime -quiet | ./pocsag -generate -stdin_message -stdout -ric:25176 -func:3 -alpha | hackrf_transfer  -f 466206250 -t -  -x 10 -a 0 -s 2000000\n");
		printf("Example: starmeteo + rpitx :\n");
		printf("         ./starmeteo -quiet -curtime -rpitx | sudo pocsag -f \"466205000\" -r 1200 -t 1\n");

		exit(0);
	}

	rpitx_outmode = 0;
	if(isOption(argc, argv,"rpitx",NULL, NULL) )
	{
		rpitx_outmode = 1;
	}

	param_start_index = 1;
	if(isOption(argc, argv,"decode",NULL, &param_start_index) )
	{
		genfrm = calloc(sizeof(frame)*argc,1);
		if(!genfrm)
			exit(-1);

		fidx = 0;
		i = param_start_index + 1;
		while( argv[i] )
		{
			printf("\nFile : %s\n",argv[i]);
			fidx = loadfrm(genfrm, fidx, argv[i]);
			i++;
		}

		printf("Frames:%d\n",fidx);

		for(int b=0;b<fidx;b++)
		{
			if(verbose)
			{
				i = 0;
				while(i<genfrm[b].quartets_cnt)
				{
					//printbin( genfrm[b].quartetfrm[i],4);
					//printf(" ");
					printf("%X",genfrm[b].quartetfrm[i]);
					i++;
				}

				printf("\n");
			}

			if( genfrm[b].quartets_cnt )
			{
				int hour, minutes, month, day, year;

				switch( genfrm[b].quartetfrm[0] )
				{
					case 0xF:
						// Trame horaire
						printf("Date frame : ");
						// Si champ heure < 10 : Codage BCD direct
						if( genfrm[b].quartetfrm[1] < 10 )
						{
							// Si les dizaine de minutes : +10 sur les heures
							if( genfrm[b].quartetfrm[2] >= 10 )
							{
								hour = genfrm[b].quartetfrm[1] + 10;
								minutes = ((genfrm[b].quartetfrm[2]-10)*10 + genfrm[b].quartetfrm[3]) ;
							}
							else
							{
								// Sinon codage BCD direct.
								hour = genfrm[b].quartetfrm[1];
								minutes = (genfrm[b].quartetfrm[2]*10 + genfrm[b].quartetfrm[3]) ;
							}
						}
						else
						{
							// Si heure >= 10
							hour = genfrm[b].quartetfrm[1] + 10;
							minutes = (genfrm[b].quartetfrm[2]*10 + genfrm[b].quartetfrm[3]) ;
						}

						month = genfrm[b].quartetfrm[4];
						day = ((genfrm[b].quartetfrm[5]>>2)&3)*10 + ( ((genfrm[b].quartetfrm[5]&3)<<2) + (genfrm[b].quartetfrm[6]>>2));
						year = 2000 + (((genfrm[b].quartetfrm[6]&3)<<4) + genfrm[b].quartetfrm[7] );

						printf("%.2d:%.2d  %d/%d/%d",hour, minutes,day,month,year);

						sum = 0x7;

						i = 0;
						while(i<8)
						{
							sum += genfrm[b].quartetfrm[i];
							i++;
						}

						if( (sum&0xF) == genfrm[b].quartetfrm[i] )
						{
							printf(" (Valid checksum)") ;
						}
						else
						{
							printf(" (Bad checksum)  ");
						}

						printf(" Area codes : ");

						i++;
						idx = (i + 3) * 4;

						int interval_minutes, areas_cnt, areas_id;

						interval_minutes = get_field((unsigned char*)&genfrm[b].quartetfrm, idx, 5, MAX_MSG_SIZE*3);
						idx += 5;

						areas_cnt = get_field((unsigned char*)&genfrm[b].quartetfrm, idx, 5, MAX_MSG_SIZE*3);
						idx += 5;

						for(int areaidx=0;areaidx<areas_cnt;areaidx++)
						{
							areas_id = get_field((unsigned char*)&genfrm[b].quartetfrm, idx, 7, MAX_MSG_SIZE*3);
							idx += 7;
							printf("%d ", areas_id);
						}

						if (idx & 3)
							idx = (idx & (~0x3)) + 0x4;

						idx += 4;

						printf("Interval (minutes) : %d ", interval_minutes);

						sum = 0x7;
						while(i<(idx>>2))
						{
							sum += genfrm[b].quartetfrm[i];
							i++;
						}

						if( (sum&0xFF) == ( (genfrm[b].quartetfrm[i]<<4) | genfrm[b].quartetfrm[i+1] ) )
						{
							printf(" (Valid checksum)") ;
						}
						else
						{
							printf(" (Bad checksum)  ");
						}

						printf("\n");

					break;
					default:
						// Trame prévision

						printf("Forecast frame - Header : ");

						for(i=0;i<6;i++)
							printf("%X",genfrm[b].quartetfrm[i]);

						printf(", Area Code : %.2d", (genfrm[b].quartetfrm[1]<<4) | genfrm[b].quartetfrm[2]);

						sum = 0x7;
						i = 0;
						while(i<5)
						{
							sum += genfrm[b].quartetfrm[i];
							i++;
						}

						printf(", ");

						if( (sum&0xF) == genfrm[b].quartetfrm[5] )
						{
							printf("Checksum : OK");
						}
						else
						{
							printf("Checksum : KO");
						}
						printf("\n");

						prev_cnt = ( genfrm[b].quartets_cnt - ((4*6)/4) ) / ((10*6)/4);

						if( prev_cnt > 6)
							prev_cnt = 6;

						idx = ((4*6)/4);
						while( prev_cnt > 0 )
						{
							int lowtemp,hightemp;

							printf("Frame: ");

							for(i=0;i<((10*6)/4);i++)
								printf("%X",genfrm[b].quartetfrm[idx+i]);

							printf(", ");

							// Checksum
							sum = 0x7;
							i = 0;
							while(i<((10*6)/4)-1)
							{
								sum += genfrm[b].quartetfrm[idx+i];
								i++;
							}

							if( (sum&0xF) == genfrm[b].quartetfrm[idx+i] )
							{
								printf("Checksum : OK, ");
							}
							else
							{
								printf("Checksum : KO, ");
							}

							// High temp : Quartet 0 :  High BCD, Quartet 1 : Low BCD
							hightemp = (((genfrm[b].quartetfrm[idx+2])*10) + genfrm[b].quartetfrm[idx+3] ) - 40;
							printf("High temp: %d°C, ", hightemp);

							// Loq temp : Quartet 2 : High BCD, Quartet 3 : Low BCD
							lowtemp = (((genfrm[b].quartetfrm[idx+0])*10) + genfrm[b].quartetfrm[idx+1]) - 40;
							printf("Low temp: %d°C, ", lowtemp);

							for(int t=0;t < 5 ;t++)
							{
								int picto = (((genfrm[b].quartetfrm[idx+4+ (t*2)])<<4) | genfrm[b].quartetfrm[idx+4+ (t*2) + 1]) & 0x3F;
								printf("Picto %d: %d (0x%.2X), ", t, picto, picto);
							}
							printf("\n");

							idx += ((10*6)/4);
							prev_cnt--;
						}

						if(idx < genfrm[b].quartets_cnt)
						{
							ck = 0;
							printf("Extra quartet(s) : ");
							while( idx < genfrm[b].quartets_cnt )
							{
								printf("%X",genfrm[b].quartetfrm[idx]);
								ck += genfrm[b].quartetfrm[idx];
								idx++;
							}
							printf("\n ck:%x\n",ck);
						}

					break;
				}
			}
		}
	}

	if(isOption(argc, argv,"encode",(char*)tmp_str, &param_start_index) )
	{
		unsigned char q;
		int size;

		genfrm = calloc(sizeof(frame)*1,1);
		if(!genfrm)
			exit(-1);

		i = 0;
		while( tmp_str[i] )
		{
			q = 0;

			if(tmp_str[i] >= 'A' && tmp_str[i] <= 'F')
				q = (tmp_str[i] - 'A') + 10;
			else if(tmp_str[i] >= 'a' && tmp_str[i] <= 'f')
				q = (tmp_str[i] - 'a') + 10;
			else if(tmp_str[i] >= '0' && tmp_str[i] <= '9')
				q = (tmp_str[i] - '0');
			else printf("Error : Invalid quartet:%c\n",tmp_str[i]);

			genfrm->quartetfrm[i] = q;
			set_quartet( (unsigned char*)(genfrm->dcodefrm), i, q);

			i++;
		}

		genfrm->quartets_cnt = i;

		if( isOption(argc, argv,"checksum",NULL, NULL) && genfrm->quartets_cnt )
		{
			int sum;

			sum = 0x7;

			for(i = 0;i < genfrm->quartets_cnt - 1; i++ )
			{
				sum = (sum + genfrm->quartetfrm[i]) & 0xF;
			}

			genfrm->quartetfrm[genfrm->quartets_cnt - 1] = sum;
			set_quartet( (unsigned char*)(genfrm->dcodefrm), genfrm->quartets_cnt - 1, sum);
		}

		size = (genfrm->quartets_cnt*4)/6;
		i = 0;
		while( i < size )
		{
			genfrm->frm[i] = raw2char(genfrm->dcodefrm[i]);
			printf("%c",genfrm->frm[i]);
			i++;
		}

		if(!quiet)
			printf("\n");

		free(genfrm);
	}

	if(isOption(argc, argv,"curtime",(char*)tmp_str, &param_start_index) )
	{
		genfrm = calloc(sizeof(frame)*1,1);
		if(!genfrm)
			exit(-1);

		genfrm->quartets_cnt = gen_current_time(genfrm->quartetfrm);
		genfrm->quartets_cnt += gen_area_ids(&genfrm->quartetfrm[genfrm->quartets_cnt]);


		i = 0;
		while( i < genfrm->quartets_cnt )
		{
			set_quartet( (unsigned char*)(genfrm->dcodefrm), i, genfrm->quartetfrm[i]);
			i++;
		}

		if( rpitx_outmode )
		{
			// RPITX string output : Put the RIC + function code before the message
			printf("25176D:");
		}

		int size = (genfrm->quartets_cnt*4)/6;
		i = 0;
		while( i < size )
		{
			genfrm->frm[i] = raw2char(genfrm->dcodefrm[i]);
			printf("%c",genfrm->frm[i]);
			i++;
		}

		if(!quiet)
			printf("\n");

		free(genfrm);
	}

	forecast_cnt = 0;
	param_start_index = 0;
	while( isOption(argc, argv,"forecast",(char*)tmp_str, &param_start_index) )
	{
		forecast_cnt++;
		param_start_index++;
	}

	if(forecast_cnt)
	{
		int departement;

		departement = 75;

		if(isOption(argc, argv,"areaid",(char*)tmp_str, NULL) )
		{
			departement = atoi(tmp_str);
		}

		genfrm = calloc(sizeof(frame)*1,1);
		if(!genfrm)
			exit(-1);

		genfrm->quartetfrm[0] = 0x4;
		genfrm->quartetfrm[1] = (departement>>4) & 0xF;
		genfrm->quartetfrm[2] = (departement   ) & 0xF;
		genfrm->quartetfrm[3] = 0x0;
		genfrm->quartetfrm[4] = 0x4;

		genfrm->quartetfrm[5] = 0x7;
		for(i=0;i<5;i++)
		{
			genfrm->quartetfrm[5] += genfrm->quartetfrm[i];
		}

		genfrm->quartets_cnt = 6;

		i = 0;
		while( i < genfrm->quartets_cnt )
		{
			set_quartet( (unsigned char*)(genfrm->dcodefrm), i, genfrm->quartetfrm[i]);
			i++;
		}

		if( rpitx_outmode )
		{
			// RPITX string output : Put the RIC + function code before the message
			printf("25176D:");
		}

		int size = (genfrm->quartets_cnt*4)/6;
		i = 0;
		while( i < size )
		{
			genfrm->frm[i] = raw2char(genfrm->dcodefrm[i]);
			printf("%c",genfrm->frm[i]);
			i++;
		}

		if(!quiet)
			printf("\n");

		free(genfrm);
	}

	param_start_index = 0;
	while( isOption(argc, argv,"forecast",(char*)tmp_str, &param_start_index) )
	{
		genfrm = calloc(sizeof(frame)*1,1);
		if(!genfrm)
			exit(-1);

		genfrm->quartets_cnt = gen_forecast(genfrm->quartetfrm,tmp_str);

		i = 0;
		while( i < genfrm->quartets_cnt )
		{
			set_quartet( (unsigned char*)(genfrm->dcodefrm), i, genfrm->quartetfrm[i]);
			i++;
		}

		int size = (genfrm->quartets_cnt*4)/6;
		i = 0;
		while( i < size )
		{
			genfrm->frm[i] = raw2char(genfrm->dcodefrm[i]);
			printf("%c",genfrm->frm[i]);
			i++;
		}

		if(!quiet)
			printf("\n");

		free(genfrm);

		param_start_index++;
	}
}
