#pragma once

#pragma warning(disable: 4100 4127 4189 4231 4505 4512 4706) // cryptopp
#pragma warning(disable:4995)	// pragma deprecated

#pragma warning(disable:4710)	// not inlined
#pragma warning(disable:4786)
#pragma warning(disable:4244)	// type conversion possible lose of data

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#pragma warning ( disable : 4201 )
#include <mmsystem.h>
#pragma warning ( default : 4201 )
#include <imagehlp.h>
#include <time.h>

#pragma warning ( push, 3 )

#include <algorithm>
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <map>

#pragma warning ( pop )

#if _MSC_VER >= 1400
#define stricmp _stricmp
#define strnicmp _strnicmp
#define strupt _strupr
#define strcmpi _strcmpi
#define fileno _fileno
//#define access _access_s
//#define _access _access_s
#define atoi _atoi64
#endif

#include "vk.h"
#include "ServiceDefs.h"
