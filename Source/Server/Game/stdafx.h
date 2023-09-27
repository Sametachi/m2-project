#pragma warning(disable:4828)

#include <LibThecore/include/stdafx.h>

#include <Basic/Singleton.h>
#include <Common/utils.h>
#include <Common/service.h>

#include <VFE/Include/VFE.hpp>

#include <algorithm>
#include <cmath>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <string>
#include <vector>

#ifdef __GNUC__
#include <float.h>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#define TR1_NS std::tr1
#else
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#define TR1_NS boost
#define isdigit iswdigit
#define isspace iswspace
#endif

#include "typedef.h"
#include "locale.hpp"
#include "event.h"

#define LC_TEXT(str) (str)

#define PASSES_PER_SEC(sec) ((sec)* passes_per_sec)

#ifndef M_PI
#define M_PI    3.14159265358979323846 /* pi */
#endif
#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923 /* pi/2 */
#endif

#define IN
#define OUT

#define M2_NEW new
#define M2_DELETE(p) delete (p)
#define M2_DELETE_ARRAY(p) delete[] (p)
#define M2_PTR_REF(p) (p)
#define M2_PTR_DEREF(p) (*(p))
