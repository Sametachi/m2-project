#include "StdAfx.h"
#include "PythonCharacterManager.h"
#include "PythonBackground.h"
#include "InstanceBase.h"
#include "../../Libraries/gameLib/RaceManager.h"

static void chrmgrSetEmpireNameMode(bool iEnable)
{
	CInstanceBase::SetEmpireNameMode(iEnable);
	CPythonCharacterManager::GetInstance()->RefreshAllPCTextTail();
}

static std::string chrmgrGetVIDInfo(uint32_t nVID)
{
	auto rkChrMgr=CPythonCharacterManager::GetInstance();

	char szDetail[256]="";
	CInstanceBase* pkInstBase=rkChrMgr->GetInstancePtr(nVID);
	if (pkInstBase)
	{
		TPixelPosition kPPosInst;
		pkInstBase->NEW_GetPixelPosition(&kPPosInst);

		int32_t xInst=kPPosInst.x;
		int32_t yInst=kPPosInst.y;

		auto rkBG=CPythonBackground::GetInstance();
		rkBG->LocalPositionToGlobalPosition(xInst, yInst);
		sprintf(szDetail, "pos=(%d, %d)", xInst, yInst);
	}	
	

	char szInfo[1024];	
	sprintf(szInfo, "VID %d (isRegistered=%d, isAlive=%d, isDead=%d) %s", 
		nVID,
		rkChrMgr->IsRegisteredVID(nVID),
		rkChrMgr->IsAliveVID(nVID),
		rkChrMgr->IsDeadVID(nVID),
		szDetail
	);
	
	return  szInfo;
}

static uint32_t chrmgrGetPickedVID()
{
	auto rkChrMgr=CPythonCharacterManager::GetInstance();

	uint32_t dwPickedActorID;
	if (rkChrMgr->OLD_GetPickedInstanceVID(&dwPickedActorID))
		return  dwPickedActorID;

	return  -1;
}

static void chrmgrSetShapeModel(uint32_t eShape, std::string szFileName)
{
	CRaceData * pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();
	if (!pRaceData)
		throw std::runtime_error("RaceData has not selected!");

	pRaceData->SetShapeModel(eShape, szFileName.c_str());
}

static void chrmgrAppendShapeSkin(uint32_t eShape, uint32_t ePart, std::string szSrcFileName, std::string szDstFileName)
{
	CRaceData * pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();
	if (!pRaceData)
		throw std::runtime_error("RaceData has not selected!");

	pRaceData->AppendShapeSkin(eShape, ePart, szSrcFileName.c_str(), szDstFileName.c_str());
}

static void chrmgrSetPathName(std::string szPathName)
{
	CRaceManager::GetInstance()->SetPathName(szPathName.c_str());	
}

static int chrmgrLoadRaceData(std::string szFileName)
{
	CRaceData * pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();
	if (!pRaceData)
		throw std::runtime_error("RaceData has not selected!");

	const char * c_szFullFileName = CRaceManager::GetInstance()->GetFullPathFileName(szFileName.c_str());
	if (!pRaceData->LoadRaceData(c_szFullFileName))
	{
		TraceLog("Failed to load race data : {}\n", c_szFullFileName);
	}
}

static int chrmgrLoadLocalRaceData(std::string szFileName)
{
	CRaceData * pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();
	if (!pRaceData)
		throw std::runtime_error("RaceData has not selected!");

	if (!pRaceData->LoadRaceData(szFileName.c_str()))
	{
		TraceLog("Failed to load race data : {}\n", szFileName);
	}
}

static void chrmgrCreateRace(uint32_t iRace)
{
	CRaceManager::GetInstance()->CreateRace(iRace);
}

static void chrmgrSelectRace(uint32_t iRace)
{
	CRaceManager::GetInstance()->SelectRace(iRace);
}

static void chrmgrSetAffect(uint32_t nVID, uint32_t nEft, bool nVisible)
{
	auto rkChrMgr = CPythonCharacterManager::GetInstance();
	rkChrMgr->SCRIPT_SetAffect(nVID >= 0 ? nVID : 0xffffffff, nEft, nVisible);
}

static void chrmgrSetEmoticon(uint32_t nVID, uint32_t nEft)
{
	auto rkChrMgr = CPythonCharacterManager::GetInstance();
	rkChrMgr->SetEmoticon(nVID >= 0 ? nVID : 0xffffffff, nEft);
}

static uint32_t chrmgrIsPossibleEmoticon(uint32_t nVID)
{
	auto rkChrMgr = CPythonCharacterManager::GetInstance();
	return rkChrMgr->IsPossibleEmoticon(nVID >= 0 ? nVID : 0xffffffff);
}

static void chrmgrRegisterEffect(uint32_t eEftType, std::string szBoneName, std::string szPathName)
{
	CInstanceBase::RegisterEffect(eEftType, szBoneName.c_str(), szPathName.c_str(), false);
}

static void chrmgrRegisterCacheEffect(uint32_t eEftType, std::string szBoneName, std::string szPathName)
{
	CInstanceBase::RegisterEffect(eEftType, szBoneName.c_str(), szPathName.c_str(), true);
}

static void chrmgrRegisterPointEffect(uint32_t iEft, std::string szFileName)
{
	auto rkChrMgr=CPythonCharacterManager::GetInstance();
	rkChrMgr->RegisterPointEffect(iEft, szFileName.c_str());
}

static void chrmgrShowPointEffect(uint32_t nVID, uint32_t nEft)
{
	auto rkChrMgr = CPythonCharacterManager::GetInstance();
	rkChrMgr->ShowPointEffect(nEft, nVID >= 0 ? nVID : 0xffffffff);
}

static void chrmgrToggleDirectionLine()
{
	static bool s_isVisible=true;
	CActorInstance::ShowDirectionLine(s_isVisible);

	s_isVisible=!s_isVisible;
}

static void chrmgrSetMovingSpeed(uint32_t nMovSpd)
{
#ifndef _DISTRIBUTE
	if (nMovSpd<0)
		throw std::runtime_error("MovingSpeed < 0");
	
	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pkInst)
		throw std::runtime_error("MainCharacter has not selected!");

	pkInst->SetMoveSpeed(nMovSpd);
#endif
}

static void chrmgrSetDustGap(float nGap)
{
	CInstanceBase::SetDustGap(nGap);
}

static void chrmgrSetHorseDustGap(float nGap)
{
	CInstanceBase::SetHorseDustGap(nGap);
}

static void chrmgrRegisterTitleName(uint32_t iIndex, std::string szTitleName)
{
	CInstanceBase::RegisterTitleName(iIndex, szTitleName.c_str());
}

static void chrmgrRegisterNameColor(uint32_t index, uint32_t ir, uint32_t ig, uint32_t ib)
{
	CInstanceBase::RegisterNameColor(index, ir, ig, ib);
}

static void chrmgrRegisterTitleColor(uint32_t iIndex, uint32_t ir, uint32_t ig, uint32_t ib)
{
	CInstanceBase::RegisterTitleColor(iIndex, ir, ig, ib);
}

PYBIND11_EMBEDDED_MODULE(chrmgr, m)
{
	m.def("SetEmpireNameMode",	chrmgrSetEmpireNameMode);
	m.def("GetVIDInfo",	chrmgrGetVIDInfo);
	m.def("GetPickedVID",	chrmgrGetPickedVID);
	m.def("SetShapeModel",	chrmgrSetShapeModel);
	m.def("AppendShapeSkin",	chrmgrAppendShapeSkin);
	m.def("SetPathName",	chrmgrSetPathName);
	m.def("LoadRaceData",	chrmgrLoadRaceData);
	m.def("LoadLocalRaceData",	chrmgrLoadLocalRaceData);
	m.def("CreateRace",	chrmgrCreateRace);
	m.def("SelectRace",	chrmgrSelectRace);
	m.def("SetAffect",	chrmgrSetAffect);
	m.def("SetEmoticon",	chrmgrSetEmoticon);
	m.def("IsPossibleEmoticon",	chrmgrIsPossibleEmoticon);
	m.def("RegisterEffect",	chrmgrRegisterEffect);
	m.def("RegisterCacheEffect",	chrmgrRegisterCacheEffect);
	m.def("RegisterPointEffect",	chrmgrRegisterPointEffect);
	m.def("ShowPointEffect",	chrmgrShowPointEffect);
	m.def("ToggleDirectionLine",	chrmgrToggleDirectionLine);
	m.def("SetMovingSpeed",	chrmgrSetMovingSpeed);
	m.def("SetDustGap",	chrmgrSetDustGap);
	m.def("SetHorseDustGap",	chrmgrSetHorseDustGap);
	m.def("RegisterTitleName",	chrmgrRegisterTitleName);
	m.def("RegisterNameColor",	chrmgrRegisterNameColor);
	m.def("RegisterTitleColor",	chrmgrRegisterTitleColor);

	m.attr("NAMECOLOR_MOB") = int32_t(CInstanceBase::NAMECOLOR_NORMAL_MOB);
	m.attr("NAMECOLOR_NPC") = int32_t(CInstanceBase::NAMECOLOR_NORMAL_NPC);
	m.attr("NAMECOLOR_PC") = int32_t(CInstanceBase::NAMECOLOR_NORMAL_PC);
	m.attr("NAMECOLOR_EMPIRE_MOB") = int32_t(CInstanceBase::NAMECOLOR_EMPIRE_MOB);
	m.attr("NAMECOLOR_EMPIRE_NPC") = int32_t(CInstanceBase::NAMECOLOR_EMPIRE_NPC);
	m.attr("NAMECOLOR_EMPIRE_PC") = int32_t(CInstanceBase::NAMECOLOR_EMPIRE_PC);
	m.attr("NAMECOLOR_FUNC") = int32_t(CInstanceBase::NAMECOLOR_FUNC);
	m.attr("NAMECOLOR_PK") = int32_t(CInstanceBase::NAMECOLOR_PK);
	m.attr("NAMECOLOR_PVP") = int32_t(CInstanceBase::NAMECOLOR_PVP);
	m.attr("NAMECOLOR_PARTY") = int32_t(CInstanceBase::NAMECOLOR_PARTY);
	m.attr("NAMECOLOR_WARP") = int32_t(CInstanceBase::NAMECOLOR_WARP);
	m.attr("NAMECOLOR_WAYPOINT") = int32_t(CInstanceBase::NAMECOLOR_WAYPOINT);
	m.attr("NAMECOLOR_EXTRA") = int32_t(CInstanceBase::NAMECOLOR_EXTRA);
	m.attr("EFFECT_SPAWN_DISAPPEAR") = int32_t(CInstanceBase::EFFECT_SPAWN_DISAPPEAR);
	m.attr("EFFECT_SPAWN_APPEAR") = int32_t(CInstanceBase::EFFECT_SPAWN_APPEAR);
	m.attr("EFFECT_DUST") = int32_t(CInstanceBase::EFFECT_DUST);
	m.attr("EFFECT_HORSE_DUST") = int32_t(CInstanceBase::EFFECT_HORSE_DUST);
	m.attr("EFFECT_STUN") = int32_t(CInstanceBase::EFFECT_STUN);
	m.attr("EFFECT_HIT") = int32_t(CInstanceBase::EFFECT_HIT);
	m.attr("EFFECT_FLAME_ATTACK") = int32_t(CInstanceBase::EFFECT_FLAME_ATTACK);
	m.attr("EFFECT_FLAME_HIT") = int32_t(CInstanceBase::EFFECT_FLAME_HIT);
	m.attr("EFFECT_FLAME_ATTACH") = int32_t(CInstanceBase::EFFECT_FLAME_ATTACH);
	m.attr("EFFECT_ELECTRIC_ATTACK") = int32_t(CInstanceBase::EFFECT_ELECTRIC_ATTACK);
	m.attr("EFFECT_ELECTRIC_HIT") = int32_t(CInstanceBase::EFFECT_ELECTRIC_HIT);
	m.attr("EFFECT_ELECTRIC_ATTACH") = int32_t(CInstanceBase::EFFECT_ELECTRIC_ATTACH);
	m.attr("EFFECT_SELECT") = int32_t(CInstanceBase::EFFECT_SELECT);
	m.attr("EFFECT_TARGET") = int32_t(CInstanceBase::EFFECT_TARGET);
	m.attr("EFFECT_CRITICAL") = int32_t(CInstanceBase::EFFECT_CRITICAL);
	m.attr("EFFECT_DAMAGE_TARGET") = int32_t(CInstanceBase::EFFECT_DAMAGE_TARGET);
	m.attr("EFFECT_DAMAGE_NOT_TARGET") = int32_t(CInstanceBase::EFFECT_DAMAGE_NOT_TARGET);
	m.attr("EFFECT_DAMAGE_SELFDAMAGE") = int32_t(CInstanceBase::EFFECT_DAMAGE_SELFDAMAGE);
	m.attr("EFFECT_DAMAGE_SELFDAMAGE2") = int32_t(CInstanceBase::EFFECT_DAMAGE_SELFDAMAGE2);
	m.attr("EFFECT_DAMAGE_POISON") = int32_t(CInstanceBase::EFFECT_DAMAGE_POISON);
	m.attr("EFFECT_DAMAGE_MISS") = int32_t(CInstanceBase::EFFECT_DAMAGE_MISS);
	m.attr("EFFECT_DAMAGE_TARGETMISS") = int32_t(CInstanceBase::EFFECT_DAMAGE_TARGETMISS);
	m.attr("EFFECT_DAMAGE_CRITICAL") = int32_t(CInstanceBase::EFFECT_DAMAGE_CRITICAL);
	m.attr("EFFECT_LEVELUP") = int32_t(CInstanceBase::EFFECT_LEVELUP);
	m.attr("EFFECT_SKILLUP") = int32_t(CInstanceBase::EFFECT_SKILLUP);
	m.attr("EFFECT_HPUP_RED") = int32_t(CInstanceBase::EFFECT_HPUP_RED);
	m.attr("EFFECT_SPUP_BLUE") = int32_t(CInstanceBase::EFFECT_SPUP_BLUE);
	m.attr("EFFECT_SPEEDUP_GREEN") = int32_t(CInstanceBase::EFFECT_SPEEDUP_GREEN);
	m.attr("EFFECT_DXUP_PURPLE") = int32_t(CInstanceBase::EFFECT_DXUP_PURPLE);
	m.attr("EFFECT_PENETRATE") = int32_t(CInstanceBase::EFFECT_PENETRATE);
	m.attr("EFFECT_BLOCK") = int32_t(CInstanceBase::EFFECT_BLOCK);
	m.attr("EFFECT_DODGE") = int32_t(CInstanceBase::EFFECT_DODGE);
	m.attr("EFFECT_FIRECRACKER") = int32_t(CInstanceBase::EFFECT_FIRECRACKER);
	m.attr("EFFECT_SPIN_TOP") = int32_t(CInstanceBase::EFFECT_SPIN_TOP);
	m.attr("EFFECT_WEAPON") = int32_t(CInstanceBase::EFFECT_WEAPON);
	m.attr("EFFECT_AFFECT") = int32_t(CInstanceBase::EFFECT_AFFECT);
	m.attr("EFFECT_EMOTICON") = int32_t(CInstanceBase::EFFECT_EMOTICON);
	m.attr("EFFECT_EMPIRE") = int32_t(CInstanceBase::EFFECT_EMPIRE);
	m.attr("EFFECT_REFINED") = int32_t(CInstanceBase::EFFECT_REFINED);
	m.attr("EFFECT_LEVELUP_ON_14_FOR_GERMANY") = int32_t(CInstanceBase::EFFECT_LEVELUP_ON_14_FOR_GERMANY);
	m.attr("EFFECT_LEVELUP_UNDER_15_FOR_GERMANY") = int32_t(CInstanceBase::EFFECT_LEVELUP_UNDER_15_FOR_GERMANY);
	m.attr("EFFECT_PERCENT_DAMAGE1") = int32_t(CInstanceBase::EFFECT_PERCENT_DAMAGE1);
	m.attr("EFFECT_PERCENT_DAMAGE2") = int32_t(CInstanceBase::EFFECT_PERCENT_DAMAGE2);
	m.attr("EFFECT_PERCENT_DAMAGE3") = int32_t(CInstanceBase::EFFECT_PERCENT_DAMAGE3);
	m.attr("EFFECT_AUTO_HPUP") = int32_t(CInstanceBase::EFFECT_AUTO_HPUP);
	m.attr("EFFECT_AUTO_SPUP") = int32_t(CInstanceBase::EFFECT_AUTO_SPUP);
	m.attr("EFFECT_RAMADAN_RING_EQUIP") = int32_t(CInstanceBase::EFFECT_RAMADAN_RING_EQUIP);
	m.attr("EFFECT_HALLOWEEN_CANDY_EQUIP") = int32_t(CInstanceBase::EFFECT_HALLOWEEN_CANDY_EQUIP);
	m.attr("EFFECT_HAPPINESS_RING_EQUIP") = int32_t(CInstanceBase::EFFECT_HAPPINESS_RING_EQUIP);
	m.attr("EFFECT_LOVE_PENDANT_EQUIP") = int32_t(CInstanceBase::EFFECT_LOVE_PENDANT_EQUIP);
}
