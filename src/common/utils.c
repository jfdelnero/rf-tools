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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_filename(char * path,char * filename)
{
	int i,done;

	i=strlen(path);
	done=0;
	while(i && !done)
	{
		i--;

		if(path[i]=='/')
		{
			done=1;
			i++;
		}
	}

	sprintf(filename,"%s",&path[i]);

	i=0;
	while(filename[i])
	{
		if(filename[i]=='.')
		{
			filename[i]='_';
		}

		i++;
	}

	return;
}

int is_printable_char(unsigned char c)
{
	int i;
	unsigned char specialchar[]={"&#{}()|_@=$!?;+*-"};

	if( (c >= 'A' && c <= 'Z') ||
		(c >= 'a' && c <= 'z') ||
		(c >= '0' && c <= '9') )
	{
		return 1;
	}

	i = 0;
	while(specialchar[i])
	{
		if(specialchar[i] == c)
		{
			return 1;
		}

		i++;
	}

	return 0;
}

void printbuf(void * buf,int size)
{
	#define PRINTBUF_HEXPERLINE 16
	#define PRINTBUF_MAXLINE_SIZE ((3*PRINTBUF_HEXPERLINE)+1+PRINTBUF_HEXPERLINE+2)

	int i,j;
	unsigned char *ptr = buf;
	char tmp[8];
	char str[PRINTBUF_MAXLINE_SIZE];

	memset(str, ' ', PRINTBUF_MAXLINE_SIZE);
	str[PRINTBUF_MAXLINE_SIZE-1] = 0;

	j = 0;
	for(i=0;i<size;i++)
	{
		if(!(i&(PRINTBUF_HEXPERLINE-1)) && i)
		{
			printf("%s\n", str);
			memset(str, ' ', PRINTBUF_MAXLINE_SIZE);
			str[PRINTBUF_MAXLINE_SIZE-1] = 0;
			j = 0;
		}

		sprintf(tmp, "%02X", ptr[i]);
		memcpy(&str[j*3],tmp,2);

		if( is_printable_char(ptr[i]) )
			str[3*PRINTBUF_HEXPERLINE + 1 + j] = ptr[i];
		else
			str[3*PRINTBUF_HEXPERLINE + 1 + j] = '.';

		j++;
	}

	printf("%s\n", str);
}
