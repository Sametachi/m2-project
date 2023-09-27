#include "StdAfx.h"
#include "PythonNonPlayer.h"

#include "InstanceBase.h"
#include "PythonCharacterManager.h"

static uint8_t nonplayerGetEventType(uint32_t iVirtualNumber)
{
	return CPythonNonPlayer::GetInstance()->GetEventType(iVirtualNumber);
}

static uint8_t nonplayerGetEventTypeByVID(uint32_t iVirtualID)
{
	return CPythonNonPlayer::GetInstance()->GetEventTypeByVID(iVirtualID);
}

static int32_t nonplayerGetLevelByVID(uint32_t iVirtualID)
{

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);

	if (!pInstance)
		return  -1;

	const TMobTable * pMobTable = CPythonNonPlayer::GetInstance()->GetTable(pInstance->GetVirtualNumber());

	if (!pMobTable)
		return  -1;

	float fAverageLevel = pMobTable->bLevel; //(float(pMobTable->abLevelRange[0]) + float(pMobTable->abLevelRange[1])) / 2.0f;
	fAverageLevel = floor(fAverageLevel + 0.5f);
	return  int32_t(fAverageLevel);
}

static uint8_t nonplayerGetGradeByVID(uint32_t iVirtualID)
{

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);

	if (!pInstance)
		return  -1;

	const TMobTable * pMobTable = CPythonNonPlayer::GetInstance()->GetTable(pInstance->GetVirtualNumber());

	if (!pMobTable)
		return  -1;

	return  pMobTable->bRank;
}

static std::string nonplayerGetMonsterName(uint32_t iVNum)
{

	auto rkNonPlayer=CPythonNonPlayer::GetInstance();
	return  rkNonPlayer->GetMonsterName(iVNum);
}

static void nonplayerLoadNonPlayerData(std::string szFileName)
{

	CPythonNonPlayer::GetInstance()->LoadMobProto(szFileName.c_str());

}



PYBIND11_EMBEDDED_MODULE(nonplayer, m)
{
	m.def("GetEventType",	nonplayerGetEventType);
	m.def("GetEventTypeByVID",	nonplayerGetEventTypeByVID);
	m.def("GetLevelByVID",	nonplayerGetLevelByVID);
	m.def("GetGradeByVID",	nonplayerGetGradeByVID);
	m.def("GetMonsterName",	nonplayerGetMonsterName);
	m.def("LoadNonPlayerData",	nonplayerLoadNonPlayerData);

	m.attr("ON_CLICK_EVENT_NONE") = int32_t(CPythonNonPlayer::ON_CLICK_EVENT_NONE);
	m.attr("ON_CLICK_EVENT_BATTLE") = int32_t(CPythonNonPlayer::ON_CLICK_EVENT_BATTLE);
	m.attr("ON_CLICK_EVENT_SHOP") = int32_t(CPythonNonPlayer::ON_CLICK_EVENT_SHOP);
	m.attr("ON_CLICK_EVENT_TALK") = int32_t(CPythonNonPlayer::ON_CLICK_EVENT_TALK);
	m.attr("ON_CLICK_EVENT_VEHICLE") = int32_t(CPythonNonPlayer::ON_CLICK_EVENT_VEHICLE);

	m.attr("PAWN") =	0;
	m.attr("S_PAWN") =	1;
	m.attr("KNIGHT") =	2;
	m.attr("S_KNIGHT") =	3;
	m.attr("BOSS") =	4;
	m.attr("KING") = 5;
}
