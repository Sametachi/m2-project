#include "stdafx.h"
#include "start_position.h"


char g_nation_name[4][32] =
{
	"",
	"Red",
	"Yellow",
	"Blue",
};


int32_t g_start_map[4] =
{
	0,	// reserved
	1,	// Red
	21,	// Yellow
	41	// Blue
};

uint32_t g_start_position[4][2] =
{
	{      0,      0 },	// reserved
	{ 469300, 964200 },	// Red
	{  55700, 157900 },	// Yellow
	{ 969600, 278400 }	// Blue
};


uint32_t arena_return_position[4][2] =
{
	{       0,  0       },
	{   347600, 882700  },
	{   138600, 236600  },
	{   857200, 251800  }
};


uint32_t g_create_position[4][2] = 
{
	{		0,		0 },
	{ 459800, 953900 },
	{ 52070, 166600 },
	{ 957300, 255200 },	
};

uint32_t g_create_position_canada[4][2] = 
{
	{		0,		0 },
	{ 457100, 946900 },
	{ 45700, 166500 },
	{ 966300, 288300 },	
};

