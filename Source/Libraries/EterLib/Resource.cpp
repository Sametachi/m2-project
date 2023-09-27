#include "StdAfx.h"
#include "../eterBase/Timer.h"
#include "../eterBase/Stl.h"
#include "Resource.h"
#include "ResourceManager.h"

bool CResource::ms_bDeleteImmediately = false;

CResource::CResource(const FilenameWrapper& filename) : me_state(STATE_EMPTY), m_stFileName(filename)
{
}

CResource::TType CResource::StringToType(const char* c_szType)
{
	return GetCRC32(c_szType, strlen(c_szType));
}

CResource::TType CResource::Type()
{
	static TType s_type = StringToType("CResource");
	return s_type;
}

bool CResource::OnIsType(TType type)
{
	if (CResource::Type() == type)
		return true;

	return false;
}

void CResource::Load()
{
	if (me_state != STATE_EMPTY)
		return;

	auto vfs = CallFS().Open(m_stFileName.GetPath());
	if (vfs)
	{
		const uint32_t size = vfs->GetSize();
		storm::View data(storm::GetDefaultAllocator());
		vfs->GetView(0, data, size);
		if (OnLoad(size, data.GetData()))
		{
			me_state = STATE_EXIST;
		}
		else
		{
			const auto& stRefResourceName = GetFileNameString();
			ConsoleLog("CResource::Load Error {0}\n", stRefResourceName.c_str());
			me_state = STATE_ERROR;
		}
	}
	else
	{
		if (OnLoad(0, nullptr))
			me_state = STATE_EXIST;
		else
		{
			const auto& stRefResourceName = GetFileNameString();
			ConsoleLog("CResource::Load file not exist {0}\n", stRefResourceName.c_str());
			me_state = STATE_ERROR;
		}
	}
}

void CResource::Reload()
{
	Clear();

	const auto& stRefResourceName = GetFileNameString();
#ifdef USE_RESOURCE_DEBUG
	ConsoleLog("CResource::Reload {0}\n", stRefResourceName.c_str());
#endif

	auto vfs = CallFS().Open(m_stFileName.GetPath());
	if (vfs)
	{
		const uint32_t size = vfs->GetSize();
		storm::View data(storm::GetDefaultAllocator());
		vfs->GetView(0, data, size);
		if (OnLoad(size, data.GetData()))
		{
			me_state = STATE_EXIST;
		}
		else
		{
			me_state = STATE_ERROR;
			return;
		}
	}
	else
	{
		if (OnLoad(0, nullptr))
			me_state = STATE_EXIST;
		else
			me_state = STATE_ERROR;
	}
}

void CResource::Clear()
{
	OnClear();
	me_state = STATE_EMPTY;
}

void CResource::SetDeleteImmediately(bool isSet)
{
	ms_bDeleteImmediately = isSet;
}

void CResource::OnConstruct()
{
	Load();
}

void CResource::OnSelfDestruct()
{
	if (ms_bDeleteImmediately)
		Clear();
	else
		CResourceManager::GetInstance()->ReserveDeletingResource(this);
}

bool CResource::CreateDeviceObjects()
{
	return true;
}

void CResource::DestroyDeviceObjects()
{
	//We never Destory this Type of resources -> CResource
}

void CResource::SetFileName(const FilenameWrapper& filename)
{
	m_stFileName = filename;
}

//Checking the resource by it existance/visibility/type
bool CResource::IsEmpty() const
{
	return OnIsEmpty();
}

bool CResource::IsData() const
{
	return me_state != STATE_EMPTY;
}

bool CResource::IsType(TType type)
{
	return OnIsType(type);
}

