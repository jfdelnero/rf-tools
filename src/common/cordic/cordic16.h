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
// File : cordic16.h
// Contains: 16 bits cordic
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

//#define CORDIC16_DEBUG 1

typedef  int16_t cordic16_wordsize;

#define CORDIC16_BITS_CNT 16

// 1 / K
#define CORDIC16_K1 0.6072529350088812561694
#define CORDIC16_PI 3.1415926536897932384626

#define CORDIC16_MUL ((float)(1 << (CORDIC16_BITS_CNT-2))) // -2 -> one bit for the sign, one bit to have a symetric min max value

#define CORDIC16_1K ((cordic16_wordsize)(CORDIC16_MUL*CORDIC16_K1))

#define CORDIC16_HALF_PI_INT (((cordic16_wordsize)(CORDIC16_MUL*(CORDIC16_PI/2))))
#define CORDIC16_PI_INT ((int)(CORDIC16_MUL*CORDIC16_PI))

#define CORDIC_0_2PI 1

#ifdef CORDIC_0_2PI
#define CORDIC16_MIN_ANGLE 0
#define CORDIC16_MAX_ANGLE ((int)(2*CORDIC16_MUL*CORDIC16_PI))
#else
#define CORDIC16_MIN_ANGLE ((int)(-CORDIC16_MUL*CORDIC16_PI))
#define CORDIC16_MAX_ANGLE ((int)(CORDIC16_MUL*CORDIC16_PI))
#endif

#define CORDIC16_FAST_CODE 1

///////////////////////////////////////////////////////////////////////////////////

/*
	table generator :

	printf("\n");
	for(i=0;i<64;i++)
	{
		printf("(cordic16_wordsize)(%.20f*CORDIC16_MUL),", atan(pow(2, -i)) );
	}
	printf("\n");
*/

static cordic16_wordsize cordic_atanpowtab [] = 
{
	(cordic16_wordsize)(0.78539816339744827900*CORDIC16_MUL),(cordic16_wordsize)(0.46364760900080609352*CORDIC16_MUL),(cordic16_wordsize)(0.24497866312686414347*CORDIC16_MUL),(cordic16_wordsize)(0.12435499454676143816*CORDIC16_MUL),
	(cordic16_wordsize)(0.06241880999595735002*CORDIC16_MUL),(cordic16_wordsize)(0.03123983343026827744*CORDIC16_MUL),(cordic16_wordsize)(0.01562372862047683129*CORDIC16_MUL),(cordic16_wordsize)(0.00781234106010111114*CORDIC16_MUL),
	(cordic16_wordsize)(0.00390623013196697176*CORDIC16_MUL),(cordic16_wordsize)(0.00195312251647881876*CORDIC16_MUL),(cordic16_wordsize)(0.00097656218955931946*CORDIC16_MUL),(cordic16_wordsize)(0.00048828121119489829*CORDIC16_MUL),
	(cordic16_wordsize)(0.00024414062014936177*CORDIC16_MUL),(cordic16_wordsize)(0.00012207031189367021*CORDIC16_MUL),(cordic16_wordsize)(0.00006103515617420877*CORDIC16_MUL),(cordic16_wordsize)(0.00003051757811552610*CORDIC16_MUL),
	(cordic16_wordsize)(0.00001525878906131576*CORDIC16_MUL),(cordic16_wordsize)(0.00000762939453110197*CORDIC16_MUL),(cordic16_wordsize)(0.00000381469726560650*CORDIC16_MUL),(cordic16_wordsize)(0.00000190734863281019*CORDIC16_MUL),
	(cordic16_wordsize)(0.00000095367431640596*CORDIC16_MUL),(cordic16_wordsize)(0.00000047683715820309*CORDIC16_MUL),(cordic16_wordsize)(0.00000023841857910156*CORDIC16_MUL),(cordic16_wordsize)(0.00000011920928955078*CORDIC16_MUL),
	(cordic16_wordsize)(0.00000005960464477539*CORDIC16_MUL),(cordic16_wordsize)(0.00000002980232238770*CORDIC16_MUL),(cordic16_wordsize)(0.00000001490116119385*CORDIC16_MUL),(cordic16_wordsize)(0.00000000745058059692*CORDIC16_MUL),
	(cordic16_wordsize)(0.00000000372529029846*CORDIC16_MUL),(cordic16_wordsize)(0.00000000186264514923*CORDIC16_MUL),(cordic16_wordsize)(0.00000000093132257462*CORDIC16_MUL),(cordic16_wordsize)(0.00000000046566128731*CORDIC16_MUL),
	(cordic16_wordsize)(0.00000000023283064365*CORDIC16_MUL),(cordic16_wordsize)(0.00000000011641532183*CORDIC16_MUL),(cordic16_wordsize)(0.00000000005820766091*CORDIC16_MUL),(cordic16_wordsize)(0.00000000002910383046*CORDIC16_MUL),
	(cordic16_wordsize)(0.00000000001455191523*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000727595761*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000363797881*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000181898940*CORDIC16_MUL),
	(cordic16_wordsize)(0.00000000000090949470*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000045474735*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000022737368*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000011368684*CORDIC16_MUL),
	(cordic16_wordsize)(0.00000000000005684342*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000002842171*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000001421085*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000710543*CORDIC16_MUL),
	(cordic16_wordsize)(0.00000000000000355271*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000177636*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000088818*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000044409*CORDIC16_MUL),
	(cordic16_wordsize)(0.00000000000000022204*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000011102*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000005551*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000002776*CORDIC16_MUL),
	(cordic16_wordsize)(0.00000000000000001388*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000000694*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000000347*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000000173*CORDIC16_MUL),
	(cordic16_wordsize)(0.00000000000000000087*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000000043*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000000022*CORDIC16_MUL),(cordic16_wordsize)(0.00000000000000000011*CORDIC16_MUL)
};

///////////////////////////////////////////////////////////////////////////////////

static void cordic16(int theta, cordic16_wordsize *s, cordic16_wordsize *c)
{
	int k;
	cordic16_wordsize tx, ty;
	cordic16_wordsize *ptr;
	cordic16_wordsize x = CORDIC16_1K;
	cordic16_wordsize y = 0;
#ifdef CORDIC16_FAST_CODE
	cordic16_wordsize sign;
#endif
	cordic16_wordsize sign_out;

	int z;

	ptr = (cordic16_wordsize*)&cordic_atanpowtab;

#ifdef CORDIC_0_2PI
	theta -= CORDIC16_PI_INT;
#endif

	z = theta;

	sign_out = 0;

	if( theta > CORDIC16_HALF_PI_INT )
	{   // > PI / 2 ?
		z = theta - CORDIC16_PI_INT; // angle -= PI
		sign_out = -1;
	}

	if( theta < -CORDIC16_HALF_PI_INT )
	{   // < -PI / 2 ?
		z =  theta + CORDIC16_PI_INT; // angle += PI
		sign_out = -1;
	}

//  z <<= 2;

	for (k=0; k<CORDIC16_BITS_CNT; k++)
	{
#ifdef CORDIC16_FAST_CODE
		// Shift the sign bit. result -1 (all bits to 1) or 0
		sign = z>>(CORDIC16_BITS_CNT-1);

		// if sign == -1, the y/x>>k sign is changed
		tx = x - (((y>>k) ^ sign) - sign);
		ty = y + (((x>>k) ^ sign) - sign);
		z -= (((*ptr++) ^ sign) - sign);
#else
		if(z < 0)
		{   // Clockwise
			tx = x + (y>>k);
			ty = y - (x>>k);
			z += (*ptr++);
		}
		else
		{   // Counter Clockwise
			tx = x - (y>>k);
			ty = y + (x>>k);
			z -= (*ptr++);
		}
#endif
		x = tx;
		y = ty;
	}

	*c = (x ^ sign_out) - sign_out;
	*s = (y ^ sign_out) - sign_out;
}

///////////////////////////////////////////////////////////////////////////////////

#ifdef CORDIC16_DEBUG

static void print_cordic_stat()
{
	int i;

	printf("CORDIC Stats :\n");
	printf("CORDIC BITS : %d (0x%X)\n",CORDIC16_BITS_CNT,CORDIC16_BITS_CNT);
	printf("CORDIC MUL : %f (0x%X)\n",CORDIC16_MUL,(int)CORDIC16_MUL);
	printf("CORDIC MIN ANGLE : %d (0x%X)\n",CORDIC16_MIN_ANGLE,CORDIC16_MIN_ANGLE);
	printf("CORDIC MAX ANGLE : %d (0x%X)\n",CORDIC16_MAX_ANGLE,CORDIC16_MAX_ANGLE);

	printf("CORDIC HALF PI INT : %d (0x%X)\n",CORDIC16_HALF_PI_INT,CORDIC16_HALF_PI_INT);
	printf("CORDIC PI INT : %d (0x%X)\n",CORDIC16_PI_INT,CORDIC16_PI_INT);

	printf("ATAN Table :\n");
	for(i=0;i<CORDIC16_BITS_CNT;i++)
	{
		printf("0x%.4X,",cordic_atanpowtab[i]);
	}
	printf("\n");

	printf("\n");
}

#endif
