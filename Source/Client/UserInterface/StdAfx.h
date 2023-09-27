#pragma once

#pragma warning(disable:4702)
#pragma warning(disable:4100)
#pragma warning(disable:4201)
#pragma warning(disable:4511)
#pragma warning(disable:4663)
#pragma warning(disable:4018)
#pragma warning(disable:4245)
#pragma warning(disable:4828)

#include <Basic/PragmaWarnings.hpp>
#include "../../Libraries/eterLib/StdAfx.h"
#include "../../Libraries/eterPythonLib/StdAfx.h"
#include "../../Libraries/gameLib/StdAfx.h"
#include "../../Libraries/scriptLib/StdAfx.h"
#include "../../Libraries/milesLib/StdAfx.h"
#include "../../Libraries/EffectLib/StdAfx.h"
#include "../../Libraries/PRTerrainLib/StdAfx.h"
#include "../../Libraries/SpeedTreeLib/StdAfx.h"
#include <VFE/Include/VFE.hpp>
#include <Basic/Logging.hpp>

#define ENABLE_NEW_EQUIPMENT_SYSTEM

#ifndef __D3DRM_H__
#define __D3DRM_H__
#endif

#include <dshow.h>

#include "Locale.h"

#include "GameType.h"

extern uint32_t __DEFAULT_CODE_PAGE__;

enum
{
	PLAYER_NAME_MAX_LEN = 12,
};

extern void ApplicationSetErrorString(const char* szErrorString);

struct SCameraPos
{
	float m_fUpDir;
	float m_fViewDir;
	float m_fCrossDir;

	SCameraPos() : m_fUpDir(0.0f), m_fViewDir(0.0f), m_fCrossDir(0.0f) {}
};

struct SCameraSetting
{
	D3DXVECTOR3				v3CenterPosition;
	SCameraPos				kCmrPos;
	float					fRotation;
	float					fPitch;
	float					fZoom;

	SCameraSetting() : v3CenterPosition(0.0f, 0.0f, 0.0f),
		fRotation(0.0f),
		fPitch(0.0f),
		fZoom(0.0f) {}
};
