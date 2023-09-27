#include "StdAfx.h"
#include <eterLib/GrpExpandedImageInstance.h>
#include <eterLib/GrpTextInstance.h>
#include <eterLib/GrpMarkInstance.h>
#include <eterLib/GrpSubImage.h>
#include <eterLib/GrpText.h>
#include <eterLib/AttributeData.h>
#include <eterGrnLib/Thing.h>
#include <eterGrnLib/ThingInstance.h>
#include <effectLib/EffectMesh.h>
#include <effectLib/EffectInstance.h>
#include <gamelib/WeaponTrace.h>
#include <gamelib/MapType.h>
#include <gamelib/GameType.h>
#include <gamelib/RaceData.h>
#include <gamelib/RaceMotionData.h>
#include <gamelib/ActorInstance.h>
#include <gamelib/Area.h>
#include <gamelib/ItemData.h>
#include <gamelib/FlyingData.h>
#include <gamelib/FlyTrace.h>
#include <gamelib/FlyingInstance.h>
#include <gamelib/FlyingData.h>

#include "PythonResourceManager.h"

CResource* NewImage(const FilenameWrapper& c_szFileName)
{
	return new CGraphicImage(c_szFileName);
}

CResource* NewSubImage(const FilenameWrapper& c_szFileName)
{
	return new CGraphicSubImage(c_szFileName);
}

CResource* NewThing(const FilenameWrapper& c_szFileName)
{
	return new CGraphicThing(c_szFileName);
}

CResource* NewEffectMesh(const FilenameWrapper& c_szFileName)
{
	return new CEffectMesh(c_szFileName);
}

CResource* NewAttributeData(const FilenameWrapper& c_szFileName)
{
	return new CAttributeData(c_szFileName);
}

void CPythonResource::Destroy()
{
    CFlyingInstance::DestroySystem();
    CActorInstance::DestroySystem();
    CGraphicExpandedImageInstance::DestroySystem();
    CGraphicImageInstance::DestroySystem();
    CGraphicThingInstance::DestroySystem();
    CGrannyModelInstance::DestroySystem();
    CEffectInstance::DestroySystem();
    CWeaponTrace::DestroySystem();
    CFlyTrace::DestroySystem();

    m_resManager.DestroyDeletingList();

    CFlyingData::DestroySystem();
    CItemData::DestroySystem();
    CEffectData::DestroySystem();
    CEffectMesh::SEffectMeshData::DestroySystem();
    NRaceData::DestroySystem();

    m_resManager.Destroy();
}

CPythonResource::CPythonResource()
{
	m_resManager.RegisterAllowedExtension("sub", NewSubImage);
	m_resManager.RegisterAllowedExtension("dds", NewImage);
	m_resManager.RegisterAllowedExtension("jpg", NewImage);
	m_resManager.RegisterAllowedExtension("tga", NewImage);
	m_resManager.RegisterAllowedExtension("png", NewImage);
	m_resManager.RegisterAllowedExtension("bmp", NewImage);
	m_resManager.RegisterAllowedExtension("gr2", NewThing);
	m_resManager.RegisterAllowedExtension("msa", NewThing);
	m_resManager.RegisterAllowedExtension("msm", NewThing);
	m_resManager.RegisterAllowedExtension("mde", NewEffectMesh);
	m_resManager.RegisterAllowedExtension("mdatr", NewAttributeData);
}
