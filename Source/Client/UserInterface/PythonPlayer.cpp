#include "StdAfx.h"
#include "PythonPlayerEventHandler.h"
#include "PythonApplication.h"
#include "PythonItem.h"
#include "../../Libraries/eterBase/Timer.h"
#include <Core/Race/RaceConstans.hpp>

static const uint32_t SKILL_POWER_NUM = 50U;

// 0 5 6 8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 50 52 54 56 58 60 63 66 69 72 82 85 88 91 94 98 102 106 110 115 125 125 125 125 125
static unsigned INTERNATIONAL_SKILL_POWERS[SKILL_POWER_NUM] =
{
	0,
		5,  6,  8, 10, 12,
		14, 16, 18, 20, 22,
		24, 26, 28, 30, 32,
		34, 36, 38, 40, 50, // master
		52, 54, 56, 58, 60,
		63, 66, 69, 72, 82, // grand_master
		85, 88, 91, 94, 98,
		102,106,110,115,125,// perfect_master
		125,
};

const uint32_t POINT_MAGIC_NUMBER = 0xe73ac1da;

void CPythonPlayer::SPlayerStatus::SetPoint(UINT ePoint, int32_t lPoint)
{
	m_alPoint[ePoint]=lPoint ^ POINT_MAGIC_NUMBER;
}

int32_t CPythonPlayer::SPlayerStatus::GetPoint(UINT ePoint)
{
	return m_alPoint[ePoint] ^ POINT_MAGIC_NUMBER;
}

bool CPythonPlayer::AffectIndexToSkillIndex(uint32_t dwAffectIndex, uint32_t * pdwSkillIndex)
{
	if (m_kMap_dwAffectIndexToSkillIndex.end() == m_kMap_dwAffectIndexToSkillIndex.find(dwAffectIndex))
		return false;

	*pdwSkillIndex = m_kMap_dwAffectIndexToSkillIndex[dwAffectIndex];
	return true;
}

bool CPythonPlayer::AffectIndexToSkillSlotIndex(UINT uAffect, uint32_t* pdwSkillSlotIndex)
{
	uint32_t dwSkillIndex=m_kMap_dwAffectIndexToSkillIndex[uAffect];

	return GetSkillSlotIndex(dwSkillIndex, pdwSkillSlotIndex);
}

bool CPythonPlayer::__GetPickedActorPtr(CInstanceBase** ppkInstPicked)
{
	auto rkChrMgr=CPythonCharacterManager::GetInstance();
	CInstanceBase* pkInstPicked=rkChrMgr->OLD_GetPickedInstancePtr();
	if (!pkInstPicked)
		return false;

	*ppkInstPicked=pkInstPicked;
	return true;
}

bool CPythonPlayer::__GetPickedActorID(uint32_t* pdwActorID)
{
	auto rkChrMgr=CPythonCharacterManager::GetInstance();
	return rkChrMgr->OLD_GetPickedInstanceVID(pdwActorID);
}

bool CPythonPlayer::__GetPickedItemID(uint32_t* pdwItemID)
{
	auto rkItemMgr=CPythonItem::GetInstance();
	return rkItemMgr->GetPickedItemID(pdwItemID);
}

bool CPythonPlayer::__GetPickedGroundPos(TPixelPosition* pkPPosPicked)
{
	auto rkBG=CPythonBackground::GetInstance();

	TPixelPosition kPPosPicked;
	if (rkBG->GetPickingPoint(pkPPosPicked))
	{
		pkPPosPicked->y=-pkPPosPicked->y;
		return true;
	}

	return false;
}

void CPythonPlayer::NEW_GetMainActorPosition(TPixelPosition* pkPPosActor)
{
	TPixelPosition kPPosMainActor;

	auto rkPlayer=CPythonPlayer::GetInstance();
	CInstanceBase * pInstance = rkPlayer->NEW_GetMainActorPtr();
	if (pInstance)
	{
		pInstance->NEW_GetPixelPosition(pkPPosActor);
	}
	else
	{
		CPythonApplication::GetInstance()->GetCenterPosition(pkPPosActor);
	}
}



bool CPythonPlayer::RegisterEffect(uint32_t dwEID, const char* c_szFileName, bool isCache)
{
	if (dwEID>=EFFECT_NUM)
		return false;

	auto rkEftMgr=CEffectManager::GetInstance();
	rkEftMgr->RegisterEffect2(c_szFileName, &m_adwEffect[dwEID], isCache);
	return true;
}

void CPythonPlayer::NEW_ShowEffect(int32_t dwEID, TPixelPosition kPPosDst)
{
	if (dwEID>=EFFECT_NUM)
		return;

	D3DXVECTOR3 kD3DVt3Pos(kPPosDst.x, -kPPosDst.y, kPPosDst.z);
	D3DXVECTOR3 kD3DVt3Dir(0.0f, 0.0f, 1.0f);

	auto rkEftMgr=CEffectManager::GetInstance();
	rkEftMgr->CreateEffect(m_adwEffect[dwEID], kD3DVt3Pos, kD3DVt3Dir);
}

CInstanceBase* CPythonPlayer::NEW_FindActorPtr(uint32_t dwVID)
{
	auto rkChrMgr = CPythonCharacterManager::GetInstance();
	return rkChrMgr->GetInstancePtr(dwVID);
}

CInstanceBase* CPythonPlayer::NEW_GetMainActorPtr()
{
	return NEW_FindActorPtr(m_dwMainCharacterIndex);
}

///////////////////////////////////////////////////////////////////////////////////////////

void CPythonPlayer::Update()
{
	NEW_RefreshMouseWalkingDirection();

	CPythonPlayerEventHandler& rkPlayerEventHandler=CPythonPlayerEventHandler::GetSingleton();
	rkPlayerEventHandler.FlushVictimList();

	if (m_isDestPosition)
	{
		CInstanceBase * pInstance = NEW_GetMainActorPtr();
		if (pInstance)
		{
			TPixelPosition PixelPosition;
			pInstance->NEW_GetPixelPosition(&PixelPosition);

			if (abs(int32_t(PixelPosition.x) - m_ixDestPos) + abs(int32_t(PixelPosition.y) - m_iyDestPos) < 10000)
			{
				m_isDestPosition = FALSE;
			}
			else
			{
				if (CTimer::GetInstance()->GetCurrentMillisecond() - m_iLastAlarmTime > 20000)
				{
					AlarmHaveToGo();
				}
			}
		}
	}

	if (m_isConsumingStamina)
	{
		float fElapsedTime = CTimer::GetInstance()->GetElapsedSecond();
		m_fCurrentStamina -= (fElapsedTime * m_fConsumeStaminaPerSec);

		SetStatus(POINT_STAMINA, uint32_t(m_fCurrentStamina));

		PyCallClassMemberFunc(m_ppyGameWindow, "RefreshStamina");
	}

	__Update_AutoAttack();
	__Update_NotifyGuildAreaEvent();
}

bool CPythonPlayer::__IsUsingChargeSkill()
{
	CInstanceBase * pkInstMain = NEW_GetMainActorPtr();
	if (!pkInstMain)
		return false;

	if (__CheckDashAffect(*pkInstMain))
		return true;

	if (MODE_USE_SKILL != m_eReservedMode)
		return false;

	if (m_dwSkillSlotIndexReserved >= SKILL_MAX_NUM)
		return false;

	TSkillInstance & rkSkillInst = m_playerStatus.aSkill[m_dwSkillSlotIndexReserved];

	CPythonSkill::TSkillData * pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(rkSkillInst.dwIndex, &pSkillData))
		return false;

	return pSkillData->IsChargeSkill() ? true : false;
}

void CPythonPlayer::__Update_AutoAttack()
{
	if (0 == m_dwAutoAttackTargetVID)
		return;

	CInstanceBase * pkInstMain = NEW_GetMainActorPtr();
	if (!pkInstMain)
		return;

	// 탄환격 쓰고 달려가는 도중에는 스킵
	if (__IsUsingChargeSkill())
		return;

	CInstanceBase* pkInstVictim=NEW_FindActorPtr(m_dwAutoAttackTargetVID);
	if (!pkInstVictim)
	{
		__ClearAutoAttackTargetActorID();
	}
	else
	{
		if (pkInstVictim->IsDead())
		{
			__ClearAutoAttackTargetActorID();
		}
		else if (pkInstMain->IsMountingHorse() && !pkInstMain->CanAttackHorseLevel())
		{
			__ClearAutoAttackTargetActorID();
		}
		else if (pkInstMain->IsAttackableInstance(*pkInstVictim))
		{
			if (pkInstMain->IsSleep())
			{
				//TraceError("SKIP_AUTO_ATTACK_IN_SLEEPING");
			}
			else
			{
				__ReserveClickActor(m_dwAutoAttackTargetVID);
			}
		}
	}
}

void CPythonPlayer::__Update_NotifyGuildAreaEvent()
{
	CInstanceBase * pkInstMain = NEW_GetMainActorPtr();
	if (pkInstMain)
	{
		TPixelPosition kPixelPosition;
		pkInstMain->NEW_GetPixelPosition(&kPixelPosition);

		uint32_t dwAreaID = CPythonMiniMap::GetInstance()->GetGuildAreaID(
			ULONG(kPixelPosition.x), ULONG(kPixelPosition.y));

		if (dwAreaID != m_inGuildAreaID)
		{
			if (0xffffffff != dwAreaID)
			{
				PyCallClassMemberFunc(m_ppyGameWindow, "BINARY_Guild_EnterGuildArea", dwAreaID);
			}
			else
			{
				PyCallClassMemberFunc(m_ppyGameWindow, "BINARY_Guild_ExitGuildArea", dwAreaID);
			}

			m_inGuildAreaID = dwAreaID;
		}
	}
}

void CPythonPlayer::SetMainCharacterIndex(int32_t iIndex)
{
	m_dwMainCharacterIndex = iIndex;

	CInstanceBase* pkInstMain=NEW_GetMainActorPtr();
	if (pkInstMain)
	{
		CPythonPlayerEventHandler& rkPlayerEventHandler=CPythonPlayerEventHandler::GetSingleton();
		pkInstMain->SetEventHandler(&rkPlayerEventHandler);
	}
}

uint32_t CPythonPlayer::GetMainCharacterIndex()
{
	return m_dwMainCharacterIndex;
}

bool CPythonPlayer::IsMainCharacterIndex(uint32_t dwIndex)
{
	return (m_dwMainCharacterIndex == dwIndex);
}

uint32_t CPythonPlayer::GetGuildID()
{
	CInstanceBase* pkInstMain=NEW_GetMainActorPtr();
	if (!pkInstMain)
		return 0xffffffff;

	return pkInstMain->GetGuildID();
}

void CPythonPlayer::SetWeaponPower(uint32_t dwMinPower, uint32_t dwMaxPower, uint32_t dwMinMagicPower, uint32_t dwMaxMagicPower, uint32_t dwAddPower)
{
	m_dwWeaponMinPower=dwMinPower;
	m_dwWeaponMaxPower=dwMaxPower;
	m_dwWeaponMinMagicPower=dwMinMagicPower;
	m_dwWeaponMaxMagicPower=dwMaxMagicPower;
	m_dwWeaponAddPower=dwAddPower;

	__UpdateBattleStatus();	
}

void CPythonPlayer::SetRace(uint32_t dwRace)
{
	m_dwRace=dwRace;
}

uint32_t CPythonPlayer::GetRace()
{
	return m_dwRace;
}

uint32_t CPythonPlayer::__GetRaceStat()
{
	switch (GetRace())
	{
		case MAIN_RACE_WARRIOR_M:
		case MAIN_RACE_WARRIOR_W:
			return GetStatus(POINT_ST);
			break;
		case MAIN_RACE_ASSASSIN_M:
		case MAIN_RACE_ASSASSIN_W:
			return GetStatus(POINT_DX);
			break;
		case MAIN_RACE_SURA_M:
		case MAIN_RACE_SURA_W:
			return GetStatus(POINT_ST);
			break;
		case MAIN_RACE_SHAMAN_M:
		case MAIN_RACE_SHAMAN_W:
			return GetStatus(POINT_IQ);
			break;
	}	
	return GetStatus(POINT_ST);
}

uint32_t CPythonPlayer::__GetLevelAtk()
{
	return 2*GetStatus(POINT_LEVEL);
}

uint32_t CPythonPlayer::__GetStatAtk()
{
	return (4*GetStatus(POINT_ST)+2*__GetRaceStat())/3;
}

uint32_t CPythonPlayer::__GetWeaponAtk(uint32_t dwWeaponPower)
{
	return 2*dwWeaponPower;
}

uint32_t CPythonPlayer::__GetTotalAtk(uint32_t dwWeaponPower, uint32_t dwRefineBonus)
{
	uint32_t dwLvAtk=__GetLevelAtk();
	uint32_t dwStAtk=__GetStatAtk();

	/////

	uint32_t dwWepAtk;
	uint32_t dwTotalAtk;	

	int32_t hr = __GetHitRate();
	dwWepAtk = __GetWeaponAtk(dwWeaponPower+dwRefineBonus);
	dwTotalAtk = dwLvAtk+(dwStAtk+dwWepAtk)*hr/100;	

	return dwTotalAtk;
}

uint32_t CPythonPlayer::__GetHitRate()
{
	int32_t src = (GetStatus(POINT_DX) * 4 + GetStatus(POINT_LEVEL) * 2)/6;

	return 100*(std::min(90, src)+210)/300;
}

uint32_t CPythonPlayer::__GetEvadeRate()
{
	return 30*(2*GetStatus(POINT_DX)+5)/(GetStatus(POINT_DX)+95);
} 

void CPythonPlayer::__UpdateBattleStatus()
{
	m_playerStatus.SetPoint(POINT_NONE, 0);
	m_playerStatus.SetPoint(POINT_ATT_GRADE, __GetEvadeRate());
	m_playerStatus.SetPoint(POINT_HIT_RATE, __GetHitRate());
	m_playerStatus.SetPoint(POINT_MIN_WEP, m_dwWeaponMinPower+m_dwWeaponAddPower);
	m_playerStatus.SetPoint(POINT_MAX_WEP, m_dwWeaponMaxPower+m_dwWeaponAddPower);
	m_playerStatus.SetPoint(POINT_MIN_MAGIC_WEP, m_dwWeaponMinMagicPower+m_dwWeaponAddPower);
	m_playerStatus.SetPoint(POINT_MAX_MAGIC_WEP, m_dwWeaponMaxMagicPower+m_dwWeaponAddPower);
	m_playerStatus.SetPoint(POINT_WEAPON_MIN, __GetTotalAtk(m_dwWeaponMinPower, m_dwWeaponAddPower));
	m_playerStatus.SetPoint(POINT_WEAPON_MAX, __GetTotalAtk(m_dwWeaponMaxPower, m_dwWeaponAddPower));
}

void CPythonPlayer::SetStatus(uint32_t dwType, int32_t lValue)
{
	if (dwType >= POINT_MAX_NUM)
	{
		assert(!" CPythonPlayer::SetStatus - Strange Status Type!");
		TraceLog("CPythonPlayer::SetStatus - Set Status Type Error\n");
		return;
	}

	if (dwType == POINT_LEVEL)
	{
		CInstanceBase* pkPlayer = NEW_GetMainActorPtr();

		if (pkPlayer)
			pkPlayer->UpdateTextTailLevel(lValue);
	}

	switch (dwType)
	{
		case POINT_MIN_WEP:
		case POINT_MAX_WEP:
		case POINT_WEAPON_MIN:
		case POINT_WEAPON_MAX:
		case POINT_HIT_RATE:
		case POINT_ATT_GRADE:
		case POINT_LEVEL:
		case POINT_ST:
		case POINT_DX:
		case POINT_IQ:
			m_playerStatus.SetPoint(dwType, lValue);
			__UpdateBattleStatus();
			break;
		default:
			m_playerStatus.SetPoint(dwType, lValue);
			break;
	}		
}

int32_t CPythonPlayer::GetStatus(uint32_t dwType)
{
	if (dwType >= POINT_MAX_NUM)
	{
		assert(!" CPythonPlayer::GetStatus - Strange Status Type!");
		TraceLog("CPythonPlayer::GetStatus - Get Status Type Error\n");
		return 0;
	}

	return m_playerStatus.GetPoint(dwType);
}

const char* CPythonPlayer::GetName()
{
	return m_stName.c_str();
}

void CPythonPlayer::SetName(const char *name)
{
	m_stName = name;
}

void CPythonPlayer::NotifyDeletingCharacterInstance(uint32_t dwVID)
{
	if (m_dwMainCharacterIndex == dwVID)
		m_dwMainCharacterIndex = 0;
}

void CPythonPlayer::NotifyCharacterDead(uint32_t dwVID)
{
	if (__IsSameTargetVID(dwVID))
	{
		SetTarget(0);
	}
}

void CPythonPlayer::NotifyCharacterUpdate(uint32_t dwVID)
{
	if (__IsSameTargetVID(dwVID))
	{
		CInstanceBase * pMainInstance = NEW_GetMainActorPtr();
		CInstanceBase * pTargetInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(dwVID);
		if (pMainInstance && pTargetInstance)
		{
			if (!pMainInstance->IsTargetableInstance(*pTargetInstance))
			{
				SetTarget(0);
				PyCallClassMemberFunc(m_ppyGameWindow, "CloseTargetBoard");
			}
			else
			{
				PyCallClassMemberFunc(m_ppyGameWindow, "RefreshTargetBoardByVID", dwVID);
			}
		}
	}
}

void CPythonPlayer::NotifyDeadMainCharacter()
{
	__ClearAutoAttackTargetActorID();
}

void CPythonPlayer::NotifyChangePKMode()
{
	PyCallClassMemberFunc(m_ppyGameWindow, "OnChangePKMode");
}


void CPythonPlayer::MoveItemData(TItemPos SrcCell, TItemPos DstCell)
{
	if (!SrcCell.IsValidItemPosition() || !DstCell.IsValidItemPosition())
		return;

	TItemData src_item(*GetItemData(SrcCell));
	TItemData dst_item(*GetItemData(DstCell));
	SetItemData(DstCell, src_item);
	SetItemData(SrcCell, dst_item);
}

const TItemData * CPythonPlayer::GetItemData(TItemPos Cell) const
{
	if (!Cell.IsValidItemPosition())
		return NULL;

	switch (Cell.window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		return &m_playerStatus.aItem[Cell.cell];
	case DRAGON_SOUL_INVENTORY:
		return &m_playerStatus.aDSItem[Cell.cell];
	default:
		return NULL;
	}
}

void CPythonPlayer::SetItemData(TItemPos Cell, const TItemData & c_rkItemInst)
{
	if (!Cell.IsValidItemPosition())
		return;

	if (c_rkItemInst.vnum != 0)
	{
		CItemData * pItemData;
		if (!CItemManager::GetInstance()->GetItemDataPointer(c_rkItemInst.vnum, &pItemData))
		{
			TraceLog("CPythonPlayer::SetItemData(window_type : {}, dwSlotIndex={}, itemIndex={}) - Failed to item data\n", Cell.window_type, Cell.cell, c_rkItemInst.vnum);
			return;
		}
	}

	switch (Cell.window_type)
	{
		case INVENTORY:
		case EQUIPMENT:
			m_playerStatus.aItem[Cell.cell] = c_rkItemInst;
			break;
		case DRAGON_SOUL_INVENTORY:
			m_playerStatus.aDSItem[Cell.cell] = c_rkItemInst;
			break;
	}
}

uint32_t CPythonPlayer::GetItemIndex(TItemPos Cell)
{
	if (!Cell.IsValidItemPosition())
		return 0;

	return GetItemData(Cell)->vnum;
}

uint32_t CPythonPlayer::GetItemFlags(TItemPos Cell)
{
	if (!Cell.IsValidItemPosition())
		return 0;
	const TItemData * pItem = GetItemData(Cell);
	assert (pItem != NULL);
	return pItem->flags;
}

uint32_t CPythonPlayer::GetItemCount(TItemPos Cell)
{
	if (!Cell.IsValidItemPosition())
		return 0;
	const TItemData * pItem = GetItemData(Cell);
	if (pItem == NULL)
		return 0;
	else
		return pItem->count;
}

uint32_t CPythonPlayer::GetItemCountByVnum(uint32_t dwVnum)
{
	uint32_t dwCount = 0;

	for (int32_t i = 0; i < c_Inventory_Count; ++i)
	{
		const TItemData & c_rItemData = m_playerStatus.aItem[i];
		if (c_rItemData.vnum == dwVnum)
		{
			dwCount += c_rItemData.count;
		}
	}

	return dwCount;
}

uint32_t CPythonPlayer::GetItemMetinSocket(TItemPos Cell, uint32_t dwMetinSocketIndex)
{
	if (!Cell.IsValidItemPosition())
		return 0;

	if (dwMetinSocketIndex >= ITEM::SOCKET_MAX_NUM)
		return 0;

	return GetItemData(Cell)->alSockets[dwMetinSocketIndex];
}

void CPythonPlayer::GetItemAttribute(TItemPos Cell, uint32_t dwAttrSlotIndex, uint8_t * pbyType, int16_t * psValue)
{
	*pbyType = 0;
	*psValue = 0;

	if (!Cell.IsValidItemPosition())
		return;

	if (dwAttrSlotIndex >= ITEM::ATTRIBUTE_MAX_NUM)
		return;

	*pbyType = GetItemData(Cell)->aAttr[dwAttrSlotIndex].bType;
	*psValue = GetItemData(Cell)->aAttr[dwAttrSlotIndex].sValue;
}

void CPythonPlayer::SetItemCount(TItemPos Cell, uint8_t byCount)
{
	if (!Cell.IsValidItemPosition())
		return;

	(const_cast <TItemData *>(GetItemData(Cell)))->count = byCount;
	PyCallClassMemberFunc(m_ppyGameWindow, "RefreshInventory");	
}

void CPythonPlayer::SetItemMetinSocket(TItemPos Cell, uint32_t dwMetinSocketIndex, uint32_t dwMetinNumber)
{
	if (!Cell.IsValidItemPosition())
		return;
	if (dwMetinSocketIndex >= ITEM::SOCKET_MAX_NUM)
		return;

	(const_cast <TItemData *>(GetItemData(Cell)))->alSockets[dwMetinSocketIndex] = dwMetinNumber;
}

void CPythonPlayer::SetItemAttribute(TItemPos Cell, uint32_t dwAttrIndex, uint8_t byType, int16_t sValue)
{
	if (!Cell.IsValidItemPosition())
		return;
	if (dwAttrIndex >= ITEM::ATTRIBUTE_MAX_NUM)
		return;

	(const_cast <TItemData *>(GetItemData(Cell)))->aAttr[dwAttrIndex].bType = byType;
	(const_cast <TItemData *>(GetItemData(Cell)))->aAttr[dwAttrIndex].sValue = sValue;
}

int32_t CPythonPlayer::GetQuickPage()
{
	return m_playerStatus.lQuickPageIndex;
}

void CPythonPlayer::SetQuickPage(int32_t nQuickPageIndex)
{
	if (nQuickPageIndex<0)
		m_playerStatus.lQuickPageIndex=QUICKSLOT_MAX_LINE+nQuickPageIndex;	
	else if (nQuickPageIndex>=QUICKSLOT_MAX_LINE)
		m_playerStatus.lQuickPageIndex=nQuickPageIndex%QUICKSLOT_MAX_LINE;	
	else
		m_playerStatus.lQuickPageIndex=nQuickPageIndex;	

	PyCallClassMemberFunc(m_ppyGameWindow, "RefreshInventory");
}

uint32_t	CPythonPlayer::LocalQuickSlotIndexToGlobalQuickSlotIndex(uint32_t dwLocalSlotIndex)
{
	return m_playerStatus.lQuickPageIndex*QUICKSLOT_MAX_COUNT_PER_LINE+dwLocalSlotIndex;	
}

void	CPythonPlayer::GetGlobalQuickSlotData(uint32_t dwGlobalSlotIndex, uint32_t* pdwWndType, uint32_t* pdwWndItemPos)
{
	TQuickSlot& rkQuickSlot=__RefGlobalQuickSlot(dwGlobalSlotIndex);
	*pdwWndType=rkQuickSlot.Type;
	*pdwWndItemPos=rkQuickSlot.Position;
}

void	CPythonPlayer::GetLocalQuickSlotData(uint32_t dwSlotPos, uint32_t* pdwWndType, uint32_t* pdwWndItemPos)
{
	TQuickSlot& rkQuickSlot=__RefLocalQuickSlot(dwSlotPos);
	*pdwWndType=rkQuickSlot.Type;
	*pdwWndItemPos=rkQuickSlot.Position;
}

TQuickSlot & CPythonPlayer::__RefLocalQuickSlot(int32_t SlotIndex)
{
	return __RefGlobalQuickSlot(LocalQuickSlotIndexToGlobalQuickSlotIndex(SlotIndex));
}

TQuickSlot & CPythonPlayer::__RefGlobalQuickSlot(int32_t SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= QUICKSLOT_MAX_NUM)
	{
		static TQuickSlot s_kQuickSlot;
		s_kQuickSlot.Type = 0;
		s_kQuickSlot.Position = 0;
		return s_kQuickSlot;
	}

	return m_playerStatus.aQuickSlot[SlotIndex];
}

void CPythonPlayer::RemoveQuickSlotByValue(int32_t iType, int32_t iPosition)
{
	for (uint8_t i = 0; i < QUICKSLOT_MAX_NUM; ++i)
	{
		if (iType == m_playerStatus.aQuickSlot[i].Type)
			if (iPosition == m_playerStatus.aQuickSlot[i].Position)
				CPythonNetworkStream::GetInstance()->SendQuickSlotDelPacket(i);
	}
}

char CPythonPlayer::IsItem(TItemPos Cell)
{
	if (!Cell.IsValidItemPosition())
		return 0;

	return 0 != GetItemData(Cell)->vnum;
}

void CPythonPlayer::RequestMoveGlobalQuickSlotToLocalQuickSlot(uint32_t dwGlobalSrcSlotIndex, uint32_t dwLocalDstSlotIndex)
{
	//uint32_t dwGlobalSrcSlotIndex=LocalQuickSlotIndexToGlobalQuickSlotIndex(dwLocalSrcSlotIndex);
	uint32_t dwGlobalDstSlotIndex=LocalQuickSlotIndexToGlobalQuickSlotIndex(dwLocalDstSlotIndex);

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendQuickSlotMovePacket((uint8_t) dwGlobalSrcSlotIndex, (uint8_t)dwGlobalDstSlotIndex);
}

void CPythonPlayer::RequestAddLocalQuickSlot(uint32_t dwLocalSlotIndex, uint32_t dwWndType, uint32_t dwWndItemPos)
{
	if (dwLocalSlotIndex>=QUICKSLOT_MAX_COUNT_PER_LINE)
		return;

	uint32_t dwGlobalSlotIndex=LocalQuickSlotIndexToGlobalQuickSlotIndex(dwLocalSlotIndex);

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendQuickSlotAddPacket((uint8_t)dwGlobalSlotIndex, (uint8_t)dwWndType, (uint8_t)dwWndItemPos);
}

void CPythonPlayer::RequestAddToEmptyLocalQuickSlot(uint32_t dwWndType, uint32_t dwWndItemPos)
{
    for (int32_t i = 0; i < QUICKSLOT_MAX_COUNT_PER_LINE; ++i)
    {
        TQuickSlot& rkQuickSlot=__RefLocalQuickSlot(i);

        if (0 == rkQuickSlot.Type)
        {
            uint32_t dwGlobalQuickSlotIndex=LocalQuickSlotIndexToGlobalQuickSlotIndex(i);
            auto rkNetStream=CPythonNetworkStream::GetInstance();
            rkNetStream->SendQuickSlotAddPacket((uint8_t)dwGlobalQuickSlotIndex, (uint8_t)dwWndType, (uint8_t)dwWndItemPos);
            return;
        }
    }

}

void CPythonPlayer::RequestDeleteGlobalQuickSlot(uint32_t dwGlobalSlotIndex)
{
	if (dwGlobalSlotIndex>=QUICKSLOT_MAX_COUNT)
		return;

	//if (dwLocalSlotIndex>=QUICKSLOT_MAX_SLOT_PER_LINE)
	//	return;

	//uint32_t dwGlobalSlotIndex=LocalQuickSlotIndexToGlobalQuickSlotIndex(dwLocalSlotIndex);

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendQuickSlotDelPacket((uint8_t)dwGlobalSlotIndex);
}

void CPythonPlayer::RequestUseLocalQuickSlot(uint32_t dwLocalSlotIndex)
{
	if (dwLocalSlotIndex>=QUICKSLOT_MAX_COUNT_PER_LINE)
		return;

	uint32_t dwRegisteredType;
	uint32_t dwRegisteredItemPos;
	GetLocalQuickSlotData(dwLocalSlotIndex, &dwRegisteredType, &dwRegisteredItemPos);

	switch (dwRegisteredType)
	{
		case SLOT_TYPE_INVENTORY:
		{
			auto rkNetStream=CPythonNetworkStream::GetInstance();
			rkNetStream->SendItemUsePacket(TItemPos(INVENTORY, (uint16_t)dwRegisteredItemPos));
			break;
		}
		case SLOT_TYPE_SKILL:
		{
			ClickSkillSlot(dwRegisteredItemPos);
			break;
		}
		case SLOT_TYPE_EMOTION:
		{
			PyCallClassMemberFunc(m_ppyGameWindow, "BINARY_ActEmotion", dwRegisteredItemPos);
			break;
		}
	}
}

void CPythonPlayer::AddQuickSlot(int32_t QuickSlotIndex, char IconType, char IconPosition)
{
	if (QuickSlotIndex < 0 || QuickSlotIndex >= QUICKSLOT_MAX_NUM)
		return;

	m_playerStatus.aQuickSlot[QuickSlotIndex].Type = IconType;
	m_playerStatus.aQuickSlot[QuickSlotIndex].Position = IconPosition;
}

void CPythonPlayer::DeleteQuickSlot(int32_t QuickSlotIndex)
{
	if (QuickSlotIndex < 0 || QuickSlotIndex >= QUICKSLOT_MAX_NUM)
		return;

	m_playerStatus.aQuickSlot[QuickSlotIndex].Type = 0;
	m_playerStatus.aQuickSlot[QuickSlotIndex].Position = 0;
}

void CPythonPlayer::MoveQuickSlot(int32_t Source, int32_t Target)
{
	if (Source < 0 || Source >= QUICKSLOT_MAX_NUM)
		return;

	if (Target < 0 || Target >= QUICKSLOT_MAX_NUM)
		return;

	TQuickSlot& rkSrcSlot=__RefGlobalQuickSlot(Source);
	TQuickSlot& rkDstSlot=__RefGlobalQuickSlot(Target);

	std::swap(rkSrcSlot, rkDstSlot);
}

#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
bool CPythonPlayer::IsBeltInventorySlot(TItemPos Cell)
{
	return Cell.IsBeltInventoryPosition();
}
#endif

bool CPythonPlayer::IsInventorySlot(TItemPos Cell)
{
	return !Cell.IsEquipPosition() && Cell.IsValidItemPosition();
}

bool CPythonPlayer::IsEquipmentSlot(TItemPos Cell)
{
	return Cell.IsEquipPosition();
}

bool CPythonPlayer::IsEquipItemInSlot(TItemPos Cell)
{
	if (!Cell.IsEquipPosition())
	{
		return false;
	}

	const TItemData * pData = GetItemData(Cell);
	
	if (NULL == pData)
	{
		return false;
	}

	uint32_t dwItemIndex = pData->vnum;

	CItemManager::GetInstance()->SelectItemData(dwItemIndex);
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
	{
		TraceLog("Failed to find ItemData - CPythonPlayer::IsEquipItem(window_type={}, iSlotindex={})\n", Cell.window_type, Cell.cell);
		return false;
	}

	return pItemData->IsEquipment() ? true : false;
}


void CPythonPlayer::SetSkill(uint32_t dwSlotIndex, uint32_t dwSkillIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return;

	m_playerStatus.aSkill[dwSlotIndex].dwIndex = dwSkillIndex;
	m_skillSlotDict[dwSkillIndex] = dwSlotIndex;
}

int32_t CPythonPlayer::GetSkillIndex(uint32_t dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_playerStatus.aSkill[dwSlotIndex].dwIndex;
}

bool CPythonPlayer::GetSkillSlotIndex(uint32_t dwSkillIndex, uint32_t* pdwSlotIndex)
{
	std::map<uint32_t, uint32_t>::iterator f=m_skillSlotDict.find(dwSkillIndex);
	if (m_skillSlotDict.end()==f)
	{
		return false;
	}

	*pdwSlotIndex=f->second;

	return true;
}

int32_t CPythonPlayer::GetSkillGrade(uint32_t dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_playerStatus.aSkill[dwSlotIndex].iGrade;
}

int32_t CPythonPlayer::GetSkillLevel(uint32_t dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_playerStatus.aSkill[dwSlotIndex].iLevel;
}

float CPythonPlayer::GetSkillCurrentEfficientPercentage(uint32_t dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_playerStatus.aSkill[dwSlotIndex].fcurEfficientPercentage;
}

float CPythonPlayer::GetSkillNextEfficientPercentage(uint32_t dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_playerStatus.aSkill[dwSlotIndex].fnextEfficientPercentage;
}

void CPythonPlayer::SetSkillLevel(uint32_t dwSlotIndex, uint32_t dwSkillLevel)
{
	assert(!"CPythonPlayer::SetSkillLevel");
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return;

	m_playerStatus.aSkill[dwSlotIndex].iGrade = -1;
	m_playerStatus.aSkill[dwSlotIndex].iLevel = dwSkillLevel;
}

void CPythonPlayer::SetSkillLevel_(uint32_t dwSkillIndex, uint32_t dwSkillGrade, uint32_t dwSkillLevel)
{
	uint32_t dwSlotIndex;
	if (!GetSkillSlotIndex(dwSkillIndex, &dwSlotIndex))
		return;

	if (dwSlotIndex >= SKILL_MAX_NUM)
		return;

	switch (dwSkillGrade)
	{
		case 0:
			m_playerStatus.aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_playerStatus.aSkill[dwSlotIndex].iLevel = dwSkillLevel;
			break;
		case 1:
			m_playerStatus.aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_playerStatus.aSkill[dwSlotIndex].iLevel = dwSkillLevel-20+1;
			break;
		case 2:
			m_playerStatus.aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_playerStatus.aSkill[dwSlotIndex].iLevel = dwSkillLevel-30+1;
			break;
		case 3:
			m_playerStatus.aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_playerStatus.aSkill[dwSlotIndex].iLevel = dwSkillLevel-40+1;
			break;
	}

	const uint32_t SKILL_MAX_LEVEL = 40;





	if (dwSkillLevel>SKILL_MAX_LEVEL)
	{
		m_playerStatus.aSkill[dwSlotIndex].fcurEfficientPercentage = 0.0f;
		m_playerStatus.aSkill[dwSlotIndex].fnextEfficientPercentage = 0.0f;

		TraceLog("CPythonPlayer::SetSkillLevel(SlotIndex={}, SkillLevel={})", dwSlotIndex, dwSkillLevel);
		return;
	}

	if (dwSkillLevel >= SKILL_POWER_NUM)
		m_playerStatus.aSkill[dwSlotIndex].fcurEfficientPercentage = 0;
	else
		m_playerStatus.aSkill[dwSlotIndex].fcurEfficientPercentage = INTERNATIONAL_SKILL_POWERS[dwSkillLevel] / 100.0f;

	if ((dwSkillLevel + 1) >= SKILL_POWER_NUM)
		m_playerStatus.aSkill[dwSlotIndex].fnextEfficientPercentage = 0;
	else
		m_playerStatus.aSkill[dwSlotIndex].fnextEfficientPercentage = INTERNATIONAL_SKILL_POWERS[dwSkillLevel + 1] / 100.0f;
}

void CPythonPlayer::SetSkillCoolTime(uint32_t dwSkillIndex)
{
	uint32_t dwSlotIndex;
	if (!GetSkillSlotIndex(dwSkillIndex, &dwSlotIndex))
	{
		TraceLog("CPythonPlayer::SetSkillCoolTime(dwSkillIndex={}) - FIND SLOT ERROR", dwSkillIndex);
		return;
	}

	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		TraceLog("CPythonPlayer::SetSkillCoolTime(dwSkillIndex={}) - dwSlotIndex=%d/%d OUT OF RANGE", dwSkillIndex, dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_playerStatus.aSkill[dwSlotIndex].isCoolTime=true;
}

void CPythonPlayer::EndSkillCoolTime(uint32_t dwSkillIndex)
{
	uint32_t dwSlotIndex;
	if (!GetSkillSlotIndex(dwSkillIndex, &dwSlotIndex))
	{
		TraceLog("CPythonPlayer::EndSkillCoolTime(dwSkillIndex={}) - FIND SLOT ERROR", dwSkillIndex);
		return;
	}

	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		TraceLog("CPythonPlayer::EndSkillCoolTime(dwSkillIndex={}) - dwSlotIndex={}/{} OUT OF RANGE", dwSkillIndex, dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_playerStatus.aSkill[dwSlotIndex].isCoolTime=false;
}

float CPythonPlayer::GetSkillCoolTime(uint32_t dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0.0f;

	return m_playerStatus.aSkill[dwSlotIndex].fCoolTime;
}

float CPythonPlayer::GetSkillElapsedCoolTime(uint32_t dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0.0f;

	return CTimer::GetInstance()->GetCurrentSecond() - m_playerStatus.aSkill[dwSlotIndex].fLastUsedTime;
}

void CPythonPlayer::__ActivateSkillSlot(uint32_t dwSlotIndex)
{
	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		TraceLog("CPythonPlayer::ActivavteSkill(dwSlotIndex={}/{}) - OUT OF RANGE", dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_playerStatus.aSkill[dwSlotIndex].bActive = TRUE;
	PyCallClassMemberFunc(m_ppyGameWindow, "ActivateSkillSlot", dwSlotIndex);
}

void CPythonPlayer::__DeactivateSkillSlot(uint32_t dwSlotIndex)
{
	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		TraceLog("CPythonPlayer::DeactivavteSkill(dwSlotIndex={}/{}) - OUT OF RANGE", dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_playerStatus.aSkill[dwSlotIndex].bActive = FALSE;
	PyCallClassMemberFunc(m_ppyGameWindow, "DeactivateSkillSlot", dwSlotIndex);
}

BOOL CPythonPlayer::IsSkillCoolTime(uint32_t dwSlotIndex)
{
	if (!__CheckRestSkillCoolTime(dwSlotIndex))
		return FALSE;

	return TRUE;
}

BOOL CPythonPlayer::IsSkillActive(uint32_t dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return FALSE;

	return m_playerStatus.aSkill[dwSlotIndex].bActive;
}

BOOL CPythonPlayer::IsToggleSkill(uint32_t dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return FALSE;

	uint32_t dwSkillIndex = m_playerStatus.aSkill[dwSlotIndex].dwIndex;

	CPythonSkill::TSkillData * pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(dwSkillIndex, &pSkillData))
		return FALSE;

	return pSkillData->IsToggleSkill();
}

void CPythonPlayer::SetPlayTime(uint32_t dwPlayTime)
{
	m_dwPlayTime = dwPlayTime;
}

uint32_t CPythonPlayer::GetPlayTime()
{
	return m_dwPlayTime;
}

void CPythonPlayer::SendClickItemPacket(uint32_t dwIID)
{
	if (IsObserverMode())
		return;

	static uint32_t s_dwNextTCPTime = 0;

	uint32_t dwCurTime=ELTimer_GetMSec();

	if (dwCurTime >= s_dwNextTCPTime)
	{
		s_dwNextTCPTime=dwCurTime + 500;

		const char * c_szOwnerName;
		if (!CPythonItem::GetInstance()->GetOwnership(dwIID, &c_szOwnerName))
			return;

		if (strlen(c_szOwnerName) > 0)
		if (0 != strcmp(c_szOwnerName, GetName()))
		{
			CItemData * pItemData;
			if (!CItemManager::GetInstance()->GetItemDataPointer(CPythonItem::GetInstance()->GetVirtualNumberOfGroundItem(dwIID), &pItemData))
			{
				TraceLog("CPythonPlayer::SendClickItemPacket(dwIID={}) : Non-exist item.", dwIID);
				return;
			}
			if (!IsPartyMemberByName(c_szOwnerName) || pItemData->IsAntiFlag(ITEM::ANTIFLAG_DROP | ITEM::ANTIFLAG_GIVE))
			{
				PyCallClassMemberFunc(m_ppyGameWindow, "OnCannotPickItem");
				return;
			}
		}

		auto rkNetStream=CPythonNetworkStream::GetInstance();
		rkNetStream->SendItemPickUpPacket(dwIID);
	}
}

void CPythonPlayer::__SendClickActorPacket(CInstanceBase& rkInstVictim)
{
	// 말을 타고 광산을 캐는 것에 대한 예외 처리
	CInstanceBase* pkInstMain=NEW_GetMainActorPtr();
	if (pkInstMain)
	if (pkInstMain->IsHoldingPickAxe())
	if (pkInstMain->IsMountingHorse())
	if (rkInstVictim.IsResource())
	{
		PyCallClassMemberFunc(m_ppyGameWindow, "OnCannotMining");
		return;
	}

	static uint32_t s_dwNextTCPTime = 0;

	uint32_t dwCurTime=ELTimer_GetMSec();

	if (dwCurTime >= s_dwNextTCPTime)
	{
		s_dwNextTCPTime=dwCurTime+1000;

		auto rkNetStream=CPythonNetworkStream::GetInstance();

		uint32_t dwVictimVID=rkInstVictim.GetVirtualID();
		rkNetStream->SendOnClickPacket(dwVictimVID);
	}
}

void CPythonPlayer::ActEmotion(uint32_t dwEmotionID)
{
	CInstanceBase * pkInstTarget = __GetAliveTargetInstancePtr();
	if (!pkInstTarget)
	{
		PyCallClassMemberFunc(m_ppyGameWindow, "OnCannotShotError", GetMainCharacterIndex(), "NEED_TARGET");
		return;
	}

	CPythonNetworkStream::GetInstance()->SendChatPacket(_getf("/kiss %s", pkInstTarget->GetNameString()));
}

void CPythonPlayer::StartEmotionProcess()
{
	__ClearReservedAction();
	__ClearAutoAttackTargetActorID();

	m_bisProcessingEmotion = TRUE;
}

void CPythonPlayer::EndEmotionProcess()
{
	m_bisProcessingEmotion = FALSE;
}

BOOL CPythonPlayer::__IsProcessingEmotion()
{
	return m_bisProcessingEmotion;
}

// Dungeon
void CPythonPlayer::SetDungeonDestinationPosition(int32_t ix, int32_t iy)
{
	m_isDestPosition = TRUE;
	m_ixDestPos = ix;
	m_iyDestPos = iy;

	AlarmHaveToGo();
}

void CPythonPlayer::AlarmHaveToGo()
{
	m_iLastAlarmTime = CTimer::GetInstance()->GetCurrentMillisecond();

	/////

	CInstanceBase * pInstance = NEW_GetMainActorPtr();
	if (!pInstance)
		return;

	TPixelPosition PixelPosition;
	pInstance->NEW_GetPixelPosition(&PixelPosition);

	float fAngle = GetDegreeFromPosition2(PixelPosition.x, PixelPosition.y, float(m_ixDestPos), float(m_iyDestPos));
	fAngle = fmod(540.0f - fAngle, 360.0f);
	D3DXVECTOR3 v3Rotation(0.0f, 0.0f, fAngle);

	PixelPosition.y *= -1.0f;

	CEffectManager::GetInstance()->RegisterEffect("d:/ymir work/effect/etc/compass/appear_middle.mse");
	CEffectManager::GetInstance()->CreateEffect("d:/ymir work/effect/etc/compass/appear_middle.mse", PixelPosition, v3Rotation);
}

// Party
void CPythonPlayer::ExitParty()
{
	m_PartyMemberMap.clear();

	CPythonCharacterManager::GetInstance()->RefreshAllPCTextTail();
}

void CPythonPlayer::AppendPartyMember(uint32_t dwPID, const char * c_szName)
{
	m_PartyMemberMap.insert(std::make_pair(dwPID, TPartyMemberInfo(dwPID, c_szName)));
}

void CPythonPlayer::LinkPartyMember(uint32_t dwPID, uint32_t dwVID)
{
	TPartyMemberInfo * pPartyMemberInfo;
	if (!GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
	{
		TraceLog(" CPythonPlayer::LinkPartyMember(dwPID={}, dwVID={}) - Failed to find party member", dwPID, dwVID);
		return;
	}

	pPartyMemberInfo->dwVID = dwVID;

	CInstanceBase * pInstance = NEW_FindActorPtr(dwVID);
	if (pInstance)
		pInstance->RefreshTextTail();
}

void CPythonPlayer::UnlinkPartyMember(uint32_t dwPID)
{
	TPartyMemberInfo * pPartyMemberInfo;
	if (!GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
	{
		TraceLog(" CPythonPlayer::UnlinkPartyMember(dwPID={}) - Failed to find party member", dwPID);
		return;
	}

	pPartyMemberInfo->dwVID = 0;
}

void CPythonPlayer::UpdatePartyMemberInfo(uint32_t dwPID, uint8_t byState, uint8_t byHPPercentage)
{
	TPartyMemberInfo * pPartyMemberInfo;
	if (!GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
	{
		TraceLog(" CPythonPlayer::UpdatePartyMemberInfo(dwPID={}, byState={}, byHPPercentage={}) - Failed to find character", dwPID, byState, byHPPercentage);
		return;
	}

	pPartyMemberInfo->byState = byState;
	pPartyMemberInfo->byHPPercentage = byHPPercentage;
}

void CPythonPlayer::UpdatePartyMemberAffect(uint32_t dwPID, uint8_t byAffectSlotIndex, int16_t sAffectNumber)
{
	if (byAffectSlotIndex >= PARTY_AFFECT_SLOT_MAX_NUM)
	{
		TraceLog(" CPythonPlayer::UpdatePartyMemberAffect(dwPID={}, byAffectSlotIndex={}, sAffectNumber={}) - Strange affect slot index", dwPID, byAffectSlotIndex, sAffectNumber);
		return;
	}

	TPartyMemberInfo * pPartyMemberInfo;
	if (!GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
	{
		TraceLog(" CPythonPlayer::UpdatePartyMemberAffect(dwPID={}, byAffectSlotIndex={}, sAffectNumber={}) - Failed to find character", dwPID, byAffectSlotIndex, sAffectNumber);
		return;
	}

	pPartyMemberInfo->sAffects[byAffectSlotIndex] = sAffectNumber;
}

void CPythonPlayer::RemovePartyMember(uint32_t dwPID)
{
	uint32_t dwVID = 0;
	TPartyMemberInfo * pPartyMemberInfo;
	if (GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
	{
		dwVID = pPartyMemberInfo->dwVID;
	}

	m_PartyMemberMap.erase(dwPID);

	if (dwVID > 0)
	{
		CInstanceBase * pInstance = NEW_FindActorPtr(dwVID);
		if (pInstance)
			pInstance->RefreshTextTail();
	}
}

bool CPythonPlayer::IsPartyMemberByVID(uint32_t dwVID)
{
	std::map<uint32_t, TPartyMemberInfo>::iterator itor = m_PartyMemberMap.begin();
	for (; itor != m_PartyMemberMap.end(); ++itor)
	{
		TPartyMemberInfo & rPartyMemberInfo = itor->second;
		if (dwVID == rPartyMemberInfo.dwVID)
			return true;
	}

	return false;
}

bool CPythonPlayer::IsPartyMemberByName(const char * c_szName)
{
	std::map<uint32_t, TPartyMemberInfo>::iterator itor = m_PartyMemberMap.begin();
	for (; itor != m_PartyMemberMap.end(); ++itor)
	{
		TPartyMemberInfo & rPartyMemberInfo = itor->second;
		if (0 == rPartyMemberInfo.strName.compare(c_szName))
			return true;
	}

	return false;
}

bool CPythonPlayer::GetPartyMemberPtr(uint32_t dwPID, TPartyMemberInfo ** ppPartyMemberInfo)
{
	std::map<uint32_t, TPartyMemberInfo>::iterator itor = m_PartyMemberMap.find(dwPID);

	if (m_PartyMemberMap.end() == itor)
		return false;

	*ppPartyMemberInfo = &(itor->second);

	return true;
}

bool CPythonPlayer::PartyMemberPIDToVID(uint32_t dwPID, uint32_t * pdwVID)
{
	std::map<uint32_t, TPartyMemberInfo>::iterator itor = m_PartyMemberMap.find(dwPID);

	if (m_PartyMemberMap.end() == itor)
		return false;

	const TPartyMemberInfo & c_rPartyMemberInfo = itor->second;
	*pdwVID = c_rPartyMemberInfo.dwVID;

	return true;
}

bool CPythonPlayer::PartyMemberVIDToPID(uint32_t dwVID, uint32_t * pdwPID)
{
	std::map<uint32_t, TPartyMemberInfo>::iterator itor = m_PartyMemberMap.begin();
	for (; itor != m_PartyMemberMap.end(); ++itor)
	{
		TPartyMemberInfo & rPartyMemberInfo = itor->second;
		if (dwVID == rPartyMemberInfo.dwVID)
		{
			*pdwPID = rPartyMemberInfo.dwPID;
			return true;
		}
	}

	return false;
}

bool CPythonPlayer::IsSamePartyMember(uint32_t dwVID1, uint32_t dwVID2)
{
	return (IsPartyMemberByVID(dwVID1) && IsPartyMemberByVID(dwVID2));
}

// PVP
void CPythonPlayer::RememberChallengeInstance(uint32_t dwVID)
{
	m_RevengeInstanceSet.erase(dwVID);
	m_ChallengeInstanceSet.insert(dwVID);
}
void CPythonPlayer::RememberRevengeInstance(uint32_t dwVID)
{
	m_ChallengeInstanceSet.erase(dwVID);
	m_RevengeInstanceSet.insert(dwVID);
}
void CPythonPlayer::RememberCantFightInstance(uint32_t dwVID)
{
	m_CantFightInstanceSet.insert(dwVID);
}
void CPythonPlayer::ForgetInstance(uint32_t dwVID)
{
	m_ChallengeInstanceSet.erase(dwVID);
	m_RevengeInstanceSet.erase(dwVID);
	m_CantFightInstanceSet.erase(dwVID);
}

bool CPythonPlayer::IsChallengeInstance(uint32_t dwVID)
{
	return m_ChallengeInstanceSet.end() != m_ChallengeInstanceSet.find(dwVID);
}
bool CPythonPlayer::IsRevengeInstance(uint32_t dwVID)
{
	return m_RevengeInstanceSet.end() != m_RevengeInstanceSet.find(dwVID);
}
bool CPythonPlayer::IsCantFightInstance(uint32_t dwVID)
{
	return m_CantFightInstanceSet.end() != m_CantFightInstanceSet.find(dwVID);
}

void CPythonPlayer::OpenPrivateShop()
{
	m_isOpenPrivateShop = TRUE;
}
void CPythonPlayer::ClosePrivateShop()
{
	m_isOpenPrivateShop = FALSE;
}

bool CPythonPlayer::IsOpenPrivateShop()
{
	return m_isOpenPrivateShop;
}

void CPythonPlayer::SetObserverMode(bool isEnable)
{
	m_isObserverMode=isEnable;
}

bool CPythonPlayer::IsObserverMode()
{
	return m_isObserverMode;
}


BOOL CPythonPlayer::__ToggleCoolTime()
{
	m_sysIsCoolTime = 1 - m_sysIsCoolTime;
	return m_sysIsCoolTime;
}

BOOL CPythonPlayer::__ToggleLevelLimit()
{
	m_sysIsLevelLimit = 1 - m_sysIsLevelLimit;
	return m_sysIsLevelLimit;
}

void CPythonPlayer::StartStaminaConsume(uint32_t dwConsumePerSec, uint32_t dwCurrentStamina)
{
	m_isConsumingStamina = TRUE;
	m_fConsumeStaminaPerSec = float(dwConsumePerSec);
	m_fCurrentStamina = float(dwCurrentStamina);

	SetStatus(POINT_STAMINA, dwCurrentStamina);
}

void CPythonPlayer::StopStaminaConsume(uint32_t dwCurrentStamina)
{
	m_isConsumingStamina = FALSE;
	m_fConsumeStaminaPerSec = 0.0f;
	m_fCurrentStamina = float(dwCurrentStamina);

	SetStatus(POINT_STAMINA, dwCurrentStamina);
}

uint32_t CPythonPlayer::GetPKMode()
{
	CInstanceBase * pInstance = NEW_GetMainActorPtr();
	if (!pInstance)
		return 0;

	return pInstance->GetPKMode();
}

void CPythonPlayer::SetGameWindow(pybind11::handle ppyObject)
{
	m_ppyGameWindow = ppyObject;
}

void CPythonPlayer::NEW_ClearSkillData(bool bAll)
{
	std::map<uint32_t, uint32_t>::iterator it;

	for (it = m_skillSlotDict.begin(); it != m_skillSlotDict.end();)
	{
		if (bAll || __GetSkillType(it->first) == CPythonSkill::SKILL_TYPE_ACTIVE)
			it = m_skillSlotDict.erase(it);
		else
			++it;
	}

	for (int32_t i = 0; i < SKILL_MAX_NUM; ++i)
	{
		ZeroMemory(&m_playerStatus.aSkill[i], sizeof(TSkillInstance));
	}

	for (int32_t j = 0; j < SKILL_MAX_NUM; ++j)
	{
		// 2004.09.30.myevan.스킬갱신시 스킬 포인트업[+] 버튼이 안나와 처리
		m_playerStatus.aSkill[j].iGrade = 0;
		m_playerStatus.aSkill[j].fcurEfficientPercentage=0.0f;
		m_playerStatus.aSkill[j].fnextEfficientPercentage=0.05f;
	}

	if (m_ppyGameWindow)
		PyCallClassMemberFunc(m_ppyGameWindow, "BINARY_CheckGameButton");
}

void CPythonPlayer::ClearSkillDict()
{
	// ClearSkillDict
	m_skillSlotDict.clear();

	// Game End - Player Data Reset
	m_isOpenPrivateShop = false;
	m_isObserverMode = false;

	m_isConsumingStamina = FALSE;
	m_fConsumeStaminaPerSec = 0.0f;
	m_fCurrentStamina = 0.0f;

	__ClearAutoAttackTargetActorID();
}

void CPythonPlayer::Clear()
{
	memset(&m_playerStatus, 0, sizeof(m_playerStatus));
	NEW_ClearSkillData(true);

	m_bisProcessingEmotion = FALSE;

	m_dwSendingTargetVID = 0;
	m_fTargetUpdateTime = 0.0f;

	// Test Code for Status Interface
	m_stName = "";
	m_dwMainCharacterIndex = 0;
	m_dwRace = 0;
	m_dwWeaponMinPower = 0;
	m_dwWeaponMaxPower = 0;
	m_dwWeaponMinMagicPower = 0;
	m_dwWeaponMaxMagicPower = 0;
	m_dwWeaponAddPower = 0;

	/////
	m_MovingCursorPosition = TPixelPosition(0, 0, 0);
	m_fMovingCursorSettingTime = 0.0f;

	m_eReservedMode = MODE_NONE;
	m_fReservedDelayTime = 0.0f;
	m_kPPosReserved = TPixelPosition(0, 0, 0);
	m_dwVIDReserved = 0;
	m_dwIIDReserved = 0;
	m_dwSkillSlotIndexReserved = 0;
	m_dwSkillRangeReserved = 0;

	m_isUp = false;
	m_isDown = false;
	m_isLeft = false;
	m_isRight = false;
	m_isSmtMov = false;
	m_isDirMov = false;
	m_isDirKey = false;
	m_isAtkKey = false;

	m_isCmrRot = true;
	m_fCmrRotSpd = 20.0f;

	m_iComboOld = 0;

	m_dwVIDPicked=0;
	m_dwIIDPicked=0;

	m_dwcurSkillSlotIndex = uint32_t(-1);

	m_dwTargetVID = 0;
	m_dwTargetEndTime = 0;

	m_PartyMemberMap.clear();

	m_ChallengeInstanceSet.clear();
	m_RevengeInstanceSet.clear();

	m_isOpenPrivateShop = false;
	m_isObserverMode = false;

	m_isConsumingStamina = FALSE;
	m_fConsumeStaminaPerSec = 0.0f;
	m_fCurrentStamina = 0.0f;

	m_inGuildAreaID = 0xffffffff;

	__ClearAutoAttackTargetActorID();
}

CPythonPlayer::CPythonPlayer(void)
{
	SetMovableGroundDistance(40.0f);

	// AffectIndex To SkillIndex
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_JEONGWI), 3));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_GEOMGYEONG), 4));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_CHEONGEUN), 19));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_GYEONGGONG), 49));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_EUNHYEONG), 34));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_GONGPO), 64));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_JUMAGAP), 65));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_HOSIN), 94));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_BOHO), 95));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_KWAESOK), 110));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_GICHEON), 96));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_JEUNGRYEOK), 111));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_PABEOP), 66));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_FALLEN_CHEONGEUN), 19));
	/////
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_GWIGEOM), 63));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_MUYEONG), 78));
	m_kMap_dwAffectIndexToSkillIndex.insert(std::make_pair(int32_t(CInstanceBase::AFFECT_HEUKSIN), 79));

	m_ppyGameWindow = pybind11::handle();

	m_sysIsCoolTime = TRUE;
	m_sysIsLevelLimit = TRUE;
	m_dwPlayTime = 0;

	m_aeMBFButton[MBT_LEFT]=CPythonPlayer::MBF_SMART;
	m_aeMBFButton[MBT_RIGHT]=CPythonPlayer::MBF_CAMERA;
	m_aeMBFButton[MBT_MIDDLE]=CPythonPlayer::MBF_CAMERA;

	memset(m_adwEffect, 0, sizeof(m_adwEffect));

	m_isDestPosition = FALSE;
	m_ixDestPos = 0;
	m_iyDestPos = 0;
	m_iLastAlarmTime = 0;

	Clear();
}

CPythonPlayer::~CPythonPlayer(void)
{
}
