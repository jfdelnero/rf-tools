/*
//
// Command line helper
//
// Copyright (C) Jean-François DEL NERO
//
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int isOption(int argc, char* argv[],char * paramtosearch,char * argtoparam, int * param_start_index)
{
	int param;
	int i,j;

	char option[512];

	memset(option,0,sizeof(option));

	param = 1;

	if( param_start_index )
	{
		param = *param_start_index;
	}

	if(param < 1)
	{
		param = 1;
	}

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

							if(param_start_index)
								*param_start_index = param;

							return 1;
						}
						else
						{
							return -1;
						}
					}
					else
					{
						if(param_start_index)
							*param_start_index = param;

						return 1;
					}
				}
			}
		}
		param++;
	}

	return 0;
}


