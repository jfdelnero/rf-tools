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
	//	printf(">> %d\n",params_dec[j]);

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
	int param_start_index;
	frame * genfrm;
	int sum;
	char tmp_str[512];
	int forcast_cnt;

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
		printf("startmeteo v0.1 -help format command line syntax.\n");

	if(isOption(argc, argv,"help",NULL, NULL) )
	{
		printf("Syntax:\n");
		printf("%s -decode [files]\n",argv[0]);
		printf("%s -encode:[HEX Quartets]\n",argv[0]);
		printf("%s -checksum     (Update checksum with \"-encode\")\n",argv[0]);
		printf("%s -curtime       Generate current date/hour frame\n",argv[0]);
		printf("%s -forecast:[LowTemp],[HighTemp],[MainPicto],[Picto_2],[Picto_3],[Picto_4],[Picto_5]\n",argv[0]);
		printf("%s -areaid:[idcode]\n",argv[0]);

		printf("%s -quiet\n",argv[0]);
		printf("%s -verbose\n",argv[0]);
		printf("\n");
		printf("Example: %s -decode ../previsions_ok/*.txt\n",argv[0]);
		printf("Example: %s -curtime -quiet\n",argv[0]);
		printf("Example: %s -forecast:-10,40,1,2,3,4,5 -forecast:-11,41,6,7,8,9,10 -forecast:-12,42,11,12,13,14,15 -forecast:-13,43,16,17,18,19,20 -areaid:75 -quiet\n",argv[0]);
		exit(0);
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

						idx = ((4*6)/4);
						while( ( idx + ((10*6)/4) - ( (4*6)/4) ) <= genfrm[b].quartets_cnt )
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
	}

	forcast_cnt = 0;
	param_start_index = 0;
	while( isOption(argc, argv,"forecast",(char*)tmp_str, &param_start_index) )
	{
		forcast_cnt++;
		param_start_index++;
	}

	if(forcast_cnt)
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

