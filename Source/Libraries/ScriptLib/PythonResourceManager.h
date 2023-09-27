#pragma once
#include <EffectLib/StdAfx.h>
#include <eterLib/Resource.h>
#include <eterLib/ResourceManager.h>

enum EResourceTypes
{
	RES_TYPE_UNKNOWN
};

class CPythonResource : public Singleton<CPythonResource>
{
public:
	CPythonResource();
	virtual ~CPythonResource() = default;

	void Destroy();

protected:
	CResourceManager m_resManager;
};