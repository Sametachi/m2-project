#pragma once

#include <LibThecore/include/stdafx.h>

#ifndef _WIN32
#include <semaphore.h>
#else
#define isdigit iswdigit
#define isspace iswspace
#endif

#include <Common/length.h>
#include <Common/tables.h>
#include <Basic/Singleton.h>
#include <Common/utils.h>
#include <Common/stl.h>
#include <Common/service.h>

#include <VFE/Include/VFE.hpp>
