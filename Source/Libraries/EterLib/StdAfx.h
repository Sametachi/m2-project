#pragma once

#define _WIN32_DCOM

#pragma warning(disable:4710)	// not inlined
#pragma warning(disable:4786)
#pragma warning(disable:4244)	// type conversion possible lose of data

#pragma warning(disable:4018)
#pragma warning(disable:4245)
#pragma warning(disable:4512)
#pragma warning(disable:4201)

#if _MSC_VER >= 1400
#pragma warning(disable:4201 4512 4238 4239)
#endif

#include <d3d9.h>
#include <d3dx9.h>

#include <dinput.h>

#pragma warning ( disable : 4201 )
#include <mmsystem.h>
#pragma warning ( default : 4201 )
#include <process.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <direct.h>
#include <malloc.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3d9.lib")
//#pragma comment(lib, "d3dx9.lib")

#include "../eterBase/StdAfx.h"
#include <Basic/Logging.hpp>
#include <VFE/Include/VFE.hpp>
#include <EterBase/Stl.h>
#include <Basic/Logging.hpp>
#ifndef VC_EXTRALEAN
#include <winsock.h>
#endif
