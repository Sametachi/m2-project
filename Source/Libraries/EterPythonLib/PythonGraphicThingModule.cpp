#include "StdAfx.h"
#include "../EterLib/ResourceManager.h"

static CGraphicThingInstance* grpThingGenerate(std::string szFileName)
{
	if (szFileName.empty())
		return 0;

	CResource* pResource = CResourceManager::GetInstance()->LoadResource<CResource>(szFileName.c_str());

	if (!pResource->IsType(CGraphicThing::Type()))
		throw std::runtime_error("Invalid created graphic thing");

	CGraphicThingInstance* pThingInstance = CGraphicThingInstance::New();
	pThingInstance->ReserveModelThing(1);
	pThingInstance->ReserveModelInstance(1);
	pThingInstance->RegisterModelThing(0, static_cast<CGraphicThing*>(pResource));
	pThingInstance->SetModelInstance(0, 0, 0);
	return pThingInstance;
}

static void grpThingDelete(CGraphicThingInstance* pThingInstance)
{
	CGraphicThingInstance::Delete(pThingInstance);
}

static void grpThingSetFileName(CGraphicThingInstance* pThingInstance, std::string szFileName)
{
	CResource* pResource = CResourceManager::GetInstance()->LoadResource<CResource>(szFileName.c_str());

	if (!pResource->IsType(CGraphicThing::Type()))
		throw std::runtime_error("Invalid graphic thing type");

	pThingInstance->Clear();
	pThingInstance->ReserveModelThing(1);
	pThingInstance->ReserveModelInstance(1);
	pThingInstance->RegisterModelThing(0, static_cast<CGraphicThing*>(pResource));
	pThingInstance->SetModelInstance(0, 0, 0);
}

static void grpThingRender(CGraphicThingInstance* pThingInstance)
{
	pThingInstance->Render();
}

static void grpThingUpdate(CGraphicThingInstance* pThingInstance)
{
	pThingInstance->Update();
	pThingInstance->Deform();
}

static void grpSetThingPosition(CGraphicThingInstance* pThingInstance, float x, float y, float z)
{
	pThingInstance->SetPosition(x, y, z);
}

static void grpSetThingRotation(CGraphicThingInstance* pThingInstance, float fDegree)
{
	pThingInstance->SetRotation(fDegree);
}

static void grpSetThingScale(CGraphicThingInstance* pThingInstance, float x, float y, float z)
{
	pThingInstance->SetScale(x, y, z);
}

PYBIND11_EMBEDDED_MODULE(grpThing, m)
{
	m.def("Update", grpThingUpdate);
	m.def("Render", grpThingRender);
	m.def("SetPosition", grpSetThingPosition);
	m.def("Generate", grpThingGenerate);
	m.def("Delete", grpThingDelete);
	m.def("SetFileName", grpThingSetFileName);
	m.def("SetRotation", grpSetThingRotation);
	m.def("SetScale", grpSetThingScale);
}
