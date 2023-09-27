#include "StdAfx.h"
#include "PythonPlayer.h"
#include "PythonApplication.h"
#include "AbstractResources.h"
#include <Core/Race/RaceConstans.hpp>
#include <Core/Constants/Group.hpp>

static int32_t GetItemGrade(const char* c_szItemName)
{
	std::string strName = c_szItemName;
	if (strName.empty())
		return 0;

	char chGrade = strName[strName.length() - 1];
	if (chGrade < '0' || chGrade > '9')
		chGrade = '0';

	int32_t iGrade = chGrade - '0';
	return iGrade;
}

enum
{
	REFINE_SCROLL_TYPE_MAKE_SOCKET = 1,
	REFINE_SCROLL_TYPE_UP_GRADE = 2,
};

enum
{
	REFINE_CANT,
	REFINE_OK,
	REFINE_ALREADY_MAX_SOCKET_COUNT,
	REFINE_NEED_MORE_GOOD_SCROLL,
	REFINE_CANT_MAKE_SOCKET_ITEM,
	REFINE_NOT_NEXT_GRADE_ITEM,
	REFINE_CANT_REFINE_METIN_TO_EQUIPMENT,
	REFINE_CANT_REFINE_ROD,
};

enum
{
	ATTACH_METIN_CANT,
	ATTACH_METIN_OK,
	ATTACH_METIN_NOT_MATCHABLE_ITEM,
	ATTACH_METIN_NO_MATCHABLE_SOCKET,
	ATTACH_METIN_NOT_EXIST_GOLD_SOCKET,
	ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT,
};

enum
{
	DETACH_METIN_CANT,
	DETACH_METIN_OK,
};

#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
class CBeltInventoryHelper
{
public:
	typedef uint8_t	TGradeUnit;

	static TGradeUnit GetBeltGradeByRefineLevel(int32_t refineLevel)
	{
		static TGradeUnit beltGradeByLevelTable[] =
		{
			0,			// ��Ʈ+0
			1,			// +1
			1,			// +2
			2,			// +3
			2,			// +4,
			3,			// +5
			4,			// +6,
			5,			// +7,
			6,			// +8,
			7,			// +9
		};

		return beltGradeByLevelTable[refineLevel];
	}

	// ���� ��Ʈ ������ ��������, � ������ �̿��� �� �ִ��� ����
	static const TGradeUnit* GetAvailableRuleTableByGrade()
	{
		/**
			��Ʈ�� �� +0 ~ +9 ������ ���� �� ������, ������ ���� 7�ܰ� ������� ���еǾ� �κ��丮�� Ȱ�� ȭ �ȴ�.
			��Ʈ ������ ���� ��� ������ ���� �Ʒ� �׸��� ����. ���� ��� >= Ȱ������ ����̸� ��� ����.
			(��, ���� ������ 0�̸� ������ ��� �Ұ�, ��ȣ ���� ���ڴ� ���)

				2(1)  4(2)  6(4)  8(6)
				5(3)  5(3)  6(4)  8(6)
				7(5)  7(5)  7(5)  8(6)
				9(7)  9(7)  9(7)  9(7)

			��Ʈ �κ��丮�� ũ��� 4x4 (16ĭ)
		*/

		static TGradeUnit availableRuleByGrade[c_Belt_Inventory_Slot_Count] = {
			1, 2, 4, 6,
			3, 3, 4, 6,
			5, 5, 5, 6,
			7, 7, 7, 7
		};

		return availableRuleByGrade;
	}

	static bool IsAvailableCell(uint16_t cell, int32_t beltGrade /*int32_t beltLevel*/)
	{
		// ��ȹ �� �ٲ�.. �Ƴ�...
		//const TGradeUnit beltGrade = GetBeltGradeByRefineLevel(beltLevel);		
		const TGradeUnit* ruleTable = GetAvailableRuleTableByGrade();

		return ruleTable[cell] <= beltGrade;
	}

};
#endif

static std::tuple<bool, int32_t, int32_t, int32_t> playerGetAutoPotionInfo(int potionType)
{
	auto player = CPythonPlayer::GetInstance();

	CPythonPlayer::SAutoPotionInfo& potionInfo = player->GetAutoPotionInfo(potionType);
	
	return std::make_tuple( potionInfo.bActivated, potionInfo.currentAmount, potionInfo.totalAmount, potionInfo.inventorySlotIndex);
}

static void playerSetAutoPotionInfo(int32_t potionType, bool bActivated, int32_t currentAmount, int32_t totalAmount, int32_t inventorySlotIndex)
{
	auto player = CPythonPlayer::GetInstance();
	CPythonPlayer::SAutoPotionInfo& potionInfo = player->GetAutoPotionInfo(potionType);
	potionInfo.bActivated = bActivated;
	potionInfo.currentAmount = currentAmount;
	potionInfo.totalAmount = totalAmount;
	potionInfo.inventorySlotIndex = inventorySlotIndex;
}

static void playerPickCloseItem()
{
	CPythonPlayer::GetInstance()->PickCloseItem();

}

static void playerSetGameWindow(pybind11::handle pyHandle)
{

	auto rkPlayer = CPythonPlayer::GetInstance();
	rkPlayer->SetGameWindow(pyHandle);

}

static void playerRegisterEffect(uint32_t iEft, std::string szFileName)
{

	auto rkPlayer = CPythonPlayer::GetInstance();
	if (!rkPlayer->RegisterEffect(iEft, szFileName.c_str(), false))
		throw std::runtime_error("CPythonPlayer::RegisterEffect(eEft=" + std::to_string(iEft) + std::string(", szFileName=") + szFileName);
}

static void playerRegisterCacheEffect(uint32_t iEft, std::string szFileName)
{

	auto rkPlayer = CPythonPlayer::GetInstance();
	if (!rkPlayer->RegisterEffect(iEft, szFileName.c_str(), true))
		throw std::runtime_error("CPythonPlayer::RegisterEffect(eEft=" + std::to_string(iEft) + ", szFileName " + szFileName);


}

static void playerSetMouseState(int32_t eMBT, int32_t eMBS)
{

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->NEW_SetMouseState(eMBT, eMBS);


}

static void playerSetMouseFunc(int32_t eMBT, int32_t eMBS)
{

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->NEW_SetMouseFunc(eMBT, eMBS);


}

static int32_t playerGetMouseFunc(int32_t eMBT)
{

	auto rkPlayer=CPythonPlayer::GetInstance();
	return  rkPlayer->NEW_GetMouseFunc(eMBT);
}

static void playerSetMouseMiddleButtonState(int32_t eMBS)
{

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->NEW_SetMouseMiddleButtonState(eMBS);


}

static void playerSetMainCharacterIndex(uint32_t iVID)
{

	CPythonPlayer::GetInstance()->SetMainCharacterIndex(iVID);
	CPythonCharacterManager::GetInstance()->SetMainInstance(iVID);


}

static uint32_t playerGetMainCharacterIndex()
{
	return  CPythonPlayer::GetInstance()->GetMainCharacterIndex();
}

static std::string playerGetMainCharacterName()
{
	return  CPythonPlayer::GetInstance()->GetName();
}

static std::tuple<float,float,float> playerGetMainCharacterPosition()
{
	TPixelPosition kPPosMainActor;
	auto rkPlayer = CPythonPlayer::GetInstance();
	rkPlayer->NEW_GetMainActorPosition(&kPPosMainActor);
	return std::make_tuple( kPPosMainActor.x, kPPosMainActor.y, kPPosMainActor.z);
}

static int playerIsMainCharacterIndex(uint32_t iVID)
{

	return  CPythonPlayer::GetInstance()->IsMainCharacterIndex(iVID);
}

static bool playerCanAttackInstance(uint32_t iVID)
{

	CInstanceBase * pMainInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	CInstanceBase * pTargetInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVID);
	if (!pMainInstance)
		return false;

	if (!pTargetInstance)
		return false;
	
	return  pMainInstance->IsAttackableInstance(*pTargetInstance);
}

static BOOL playerIsActingEmotion()
{
	CInstanceBase * pMainInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	if (!pMainInstance)
		return FALSE;

	return  pMainInstance->IsActingEmotion();
}

static bool playerIsPVPInstance(uint32_t iVID)
{

	CInstanceBase * pMainInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	CInstanceBase * pTargetInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVID);
	if (!pMainInstance)
		return  0;	
	
	if (!pTargetInstance)
		return  0;

	return  pMainInstance->IsPVPInstance(*pTargetInstance);
}

static BOOL playerIsSameEmpire(uint32_t iVID)
{

	CInstanceBase * pMainInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	CInstanceBase * pTargetInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVID);
	if (!pMainInstance)
		return  FALSE;	
	
	if (!pTargetInstance)
		return  FALSE;

	return  pMainInstance->IsSameEmpire(*pTargetInstance);
}

static bool playerIsChallengeInstance(uint32_t iVID)
{

	return  CPythonPlayer::GetInstance()->IsChallengeInstance(iVID);
}

static bool playerIsRevengeInstance(uint32_t iVID)
{

	return  CPythonPlayer::GetInstance()->IsRevengeInstance(iVID);
}

static bool playerIsCantFightInstance(uint32_t iVID)
{

	return  CPythonPlayer::GetInstance()->IsCantFightInstance(iVID);
}

static float playerGetCharacterDistance(uint32_t iVID)
{

	CInstanceBase * pMainInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	CInstanceBase * pTargetInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVID);
	if (!pMainInstance)
		return  -1.0f;	
	
	if (!pTargetInstance)
		return  -1.0f;

	return  pMainInstance->GetDistance(pTargetInstance);
}

static BOOL playerIsInSafeArea()
{
	CInstanceBase * pMainInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	if (!pMainInstance)
		return  FALSE;

	return  pMainInstance->IsInSafe();
}

static BOOL playerIsMountingHorse()
{
	CInstanceBase * pMainInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	if (!pMainInstance)
		return  FALSE;

	return  pMainInstance->IsMountingHorse();
}

static bool playerIsObserverMode()
{
	auto rkPlayer=CPythonPlayer::GetInstance();
	return  rkPlayer->IsObserverMode();
}

static void playerActEmotion(uint32_t iVID)
{

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->ActEmotion(iVID);

}

static void playerShowPlayer()
{
	CInstanceBase * pMainInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	if (pMainInstance)
		pMainInstance->GetGraphicThingInstanceRef().Show();

}

static void playerHidePlayer()
{
	CInstanceBase * pMainInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	if (pMainInstance)
		pMainInstance->GetGraphicThingInstanceRef().Hide();

}

static void playerComboAttack()
{
	CPythonPlayer::GetInstance()->NEW_Attack();

}

static void playerSetAutoCameraRotationSpeed(float fCmrRotSpd)
{

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->NEW_SetAutoCameraRotationSpeed(fCmrRotSpd);

}

static void playerSetAttackKeyState(bool isPressed)
{

	auto rkPlayer = CPythonPlayer::GetInstance();
	rkPlayer->SetAttackKeyState(isPressed);

}

static void playerSetSingleDIKKeyState(int32_t eDIK, bool isPressed)
{

	auto rkPlayer = CPythonPlayer::GetInstance();
	rkPlayer->NEW_SetSingleDIKKeyState(eDIK, isPressed);

}

static void playerEndKeyWalkingImmediately()
{
	CPythonPlayer::GetInstance()->NEW_Stop();

}

static void playerStartMouseWalking()
{

}

static void playerEndMouseWalking()
{

}

static void playerResetCameraRotation()
{
	CPythonPlayer::GetInstance()->NEW_ResetCameraRotation();

}

static void playerSetQuickCameraMode(int nIsEnable)
{

	auto rkPlayer = CPythonPlayer::GetInstance();
	rkPlayer->SetQuickCameraMode(nIsEnable ? true : false);	


}

static void playerSetSkill(uint32_t iSlotIndex, uint32_t iSkillIndex)
{

	CPythonPlayer::GetInstance()->SetSkill(iSlotIndex, iSkillIndex);

}

static uint32_t playerGetSkillIndex(uint32_t iSlotIndex)
{

	return  CPythonPlayer::GetInstance()->GetSkillIndex(iSlotIndex);
}

static uint32_t playerGetSkillSlotIndex(uint32_t iSkillIndex)
{

	uint32_t dwSlotIndex;
	if (!CPythonPlayer::GetInstance()->GetSkillSlotIndex(iSkillIndex, &dwSlotIndex))
		throw std::runtime_error("No skill slot selected");

	return  dwSlotIndex;
}

static int playerGetSkillGrade(int iSlotIndex)
{

	return  CPythonPlayer::GetInstance()->GetSkillGrade(iSlotIndex);
}

static int32_t playerGetSkillLevel(uint32_t iSlotIndex)
{

	return  CPythonPlayer::GetInstance()->GetSkillLevel(iSlotIndex);
}

static float playerGetSkillCurrentEfficientPercentage(uint32_t iSlotIndex)
{

	return  CPythonPlayer::GetInstance()->GetSkillCurrentEfficientPercentage(iSlotIndex);
}

static float playerGetSkillNextEfficientPercentage(uint32_t iSlotIndex)
{

	return  CPythonPlayer::GetInstance()->GetSkillNextEfficientPercentage(iSlotIndex);
}

static void playerClickSkillSlot(uint32_t iSkillSlot)
{

	CPythonPlayer::GetInstance()->ClickSkillSlot(iSkillSlot);


}

static void playerChangeCurrentSkillNumberOnly(uint32_t iSlotIndex)
{

	CPythonPlayer::GetInstance()->ChangeCurrentSkillNumberOnly(iSlotIndex);


}

static void playerClearSkillDict()
{
	CPythonPlayer::GetInstance()->ClearSkillDict();

}

static uint32_t playerGetItemIndex(uint16_t iSlotIndex)
{
	return  CPythonPlayer::GetInstance()->GetItemIndex(TItemPos(INVENTORY, iSlotIndex));
}

static uint32_t playerGetItemIndex2(uint8_t window, uint16_t iSlotIndex)
{
	return  CPythonPlayer::GetInstance()->GetItemIndex(TItemPos(window, iSlotIndex));
}

static uint32_t playerGetItemFlags(uint16_t iSlotIndex)
{
	return  CPythonPlayer::GetInstance()->GetItemFlags(TItemPos(INVENTORY, iSlotIndex));
}

static uint32_t playerGetItemFlags2(uint8_t window, uint16_t iSlotIndex)
{
	return  CPythonPlayer::GetInstance()->GetItemFlags(TItemPos(window, iSlotIndex));
}

static uint32_t playerGetItemCount(uint16_t iSlotIndex)
{
	return  CPythonPlayer::GetInstance()->GetItemCount(TItemPos(INVENTORY, iSlotIndex));
}

static uint32_t playerGetItemCount2(uint8_t window, uint16_t iSlotIndex)
{
	return  CPythonPlayer::GetInstance()->GetItemCount(TItemPos(window, iSlotIndex));
}

static uint32_t playerGetItemCountByVnum(uint32_t ivnum)
{
	return CPythonPlayer::GetInstance()->GetItemCountByVnum(ivnum);
}

static uint32_t playerGetItemMetinSocket2(uint8_t window, uint16_t iSlotIndex, uint32_t iMetinSocketIndex)
{
	return  CPythonPlayer::GetInstance()->GetItemMetinSocket(TItemPos(window, iSlotIndex), iMetinSocketIndex);
}

static uint32_t playerGetItemMetinSocket(uint16_t iSlotIndex, uint32_t iMetinSocketIndex)
{
	return CPythonPlayer::GetInstance()->GetItemMetinSocket(TItemPos(INVENTORY, iSlotIndex), iMetinSocketIndex);
}

static std::tuple<uint8_t, int16_t> playerGetItemAttribute2(uint8_t window, uint16_t iSlotIndex, uint32_t iMetinSocketIndex)
{
	uint8_t byType;
	int16_t sValue;
	CPythonPlayer::GetInstance()->GetItemAttribute(TItemPos(window, iSlotIndex), iMetinSocketIndex, &byType, &sValue);
	return std::make_tuple(byType, sValue);
}

static std::tuple<uint8_t, int16_t> playerGetItemAttribute(uint16_t iSlotIndex, uint32_t iMetinSocketIndex)
{
	uint8_t byType;
	int16_t sValue;
	CPythonPlayer::GetInstance()->GetItemAttribute(TItemPos(INVENTORY, iSlotIndex), iMetinSocketIndex, &byType, &sValue);
	return std::make_tuple(byType, sValue);
}

static int32_t playerGetISellItemPrice2(uint8_t window, uint16_t iSlotIndex)
{
	TItemPos Cell(window, iSlotIndex);
	CItemData* pItemData;

	if (!CItemManager::GetInstance()->GetItemDataPointer(CPythonPlayer::GetInstance()->GetItemIndex(Cell), &pItemData))
		return 0;

	int32_t iPrice;

	if (pItemData->IsFlag(ITEM::FLAG_COUNT_PER_1GOLD))
		iPrice = CPythonPlayer::GetInstance()->GetItemCount(Cell) / pItemData->GetISellItemPrice();
	else
		iPrice = pItemData->GetISellItemPrice() * CPythonPlayer::GetInstance()->GetItemCount(Cell);

	iPrice /= 5;
	return iPrice;
}


static uint32_t playerGetISellItemPrice(uint16_t iSlotIndex)
{
	return playerGetISellItemPrice2(INVENTORY, iSlotIndex);
}

static void playerMoveItem(uint8_t srcw, uint16_t srcc, uint8_t dstw, uint16_t dstc)
{
	CPythonPlayer::GetInstance()->MoveItemData(TItemPos(srcw, srcc), TItemPos(dstw, dstc));
}

static void playerMoveItem2(uint16_t srcc, uint16_t dstc)
{
	CPythonPlayer::GetInstance()->MoveItemData(TItemPos(INVENTORY, srcc), TItemPos(INVENTORY, dstc));
}

static void playerSendClickItemPacket(uint32_t ivid)
{

	CPythonPlayer::GetInstance()->SendClickItemPacket(ivid);

}

static std::string playerGetName()
{
	return  CPythonPlayer::GetInstance()->GetName();
}

static int32_t playerGetJob()
{
	uint32_t race = CPythonPlayer::GetInstance()->GetRace();
	int32_t job = RaceToJob(race);
	return  job;
}

static uint32_t playerGetRace()
{
	return  CPythonPlayer::GetInstance()->GetRace();
}

static uint32_t playerGetPlayTime()
{
	return  CPythonPlayer::GetInstance()->GetPlayTime();
}

static void playerSetPlayTime(uint32_t iTime)
{

	CPythonPlayer::GetInstance()->SetPlayTime(iTime);

}

static BOOL playerIsSkillCoolTime(uint32_t iSlotIndex)
{

	return  CPythonPlayer::GetInstance()->IsSkillCoolTime(iSlotIndex);
}

static std::tuple<float,float> playerGetSkillCoolTime(uint32_t iSlotIndex)
{

	float fCoolTime = CPythonPlayer::GetInstance()->GetSkillCoolTime(iSlotIndex);
	float fElapsedCoolTime = CPythonPlayer::GetInstance()->GetSkillElapsedCoolTime(iSlotIndex);
	return std::make_tuple( fCoolTime, fElapsedCoolTime);
}

static BOOL playerIsSkillActive(uint32_t iSlotIndex)
{

	return  CPythonPlayer::GetInstance()->IsSkillActive(iSlotIndex);
}

static void playerUseGuildSkill(uint32_t iSkillSlotIndex)
{

	CPythonPlayer::GetInstance()->UseGuildSkill(iSkillSlotIndex);

}

static uint32_t playerAffectIndexToSkillIndex(uint32_t iAffectIndex)
{

	uint32_t dwSkillIndex;
	if (!CPythonPlayer::GetInstance()->AffectIndexToSkillIndex(iAffectIndex, &dwSkillIndex))
		return  0;

	return  dwSkillIndex;
}

static uint32_t playerGetEXP()
{
	return uint32_t(CPythonPlayer::GetInstance()->GetStatus(POINT_EXP));
}

static int32_t playerGetStatus(uint32_t iType)
{

	int32_t iValue = CPythonPlayer::GetInstance()->GetStatus(iType);

	if (POINT_ATT_SPEED == iType)
	{
		CInstanceBase * pInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
		if (pInstance && (ITEM::WEAPON_TWO_HANDED == pInstance->GetWeaponType()))
		{
			iValue -= 10;
		}
	}

	return  iValue;
}

static void playerSetStatus(uint32_t iType, int32_t iValue)
{

	CPythonPlayer::GetInstance()->SetStatus(iType, iValue);

}

static uint32_t playerGetElk()
{
	return uint32_t(CPythonPlayer::GetInstance()->GetStatus(POINT_GOLD));
}

static uint32_t playerGetGuildID()
{
	CInstanceBase * pInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	if (!pInstance)
		return  0;

	return  pInstance->GetGuildID();
}

static std::string playerGetGuildName()
{
	CInstanceBase * pInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	if (pInstance)
	{
		uint32_t dwID = pInstance->GetGuildID();
		std::string strName;
		if (CPythonGuild::GetInstance()->GetGuildName(dwID, &strName))
			return  strName;	
	}

	return  "";
}

static std::tuple<int32_t, uint32_t> playerGetAlignmentData()
{
	CInstanceBase * pInstance = CPythonPlayer::GetInstance()->NEW_GetMainActorPtr();
	int32_t iAlignmentPoint = 0;
	uint32_t iAlignmentGrade = 4;
	if (pInstance)
	{
		iAlignmentPoint = pInstance->GetAlignment();
		iAlignmentGrade = pInstance->GetAlignmentGrade();
	}
	return std::make_tuple( iAlignmentPoint, iAlignmentGrade);
}

static void playerRequestAddLocalQuickSlot(uint32_t nSlotIndex, uint32_t nWndType, uint32_t nWndItemPos)
{
	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->RequestAddLocalQuickSlot(nSlotIndex, nWndType, nWndItemPos);


}

static void playerRequestAddToEmptyLocalQuickSlot(uint32_t nWndType, uint32_t nWndItemPos)
{

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->RequestAddToEmptyLocalQuickSlot(nWndType, nWndItemPos);


}

static void playerRequestDeleteGlobalQuickSlot(uint32_t nGlobalSlotIndex)
{

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->RequestDeleteGlobalQuickSlot(nGlobalSlotIndex);

}

static void playerRequestMoveGlobalQuickSlotToLocalQuickSlot(uint32_t nGlobalSrcSlotIndex, uint32_t nLocalDstSlotIndex)
{
	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->RequestMoveGlobalQuickSlotToLocalQuickSlot(nGlobalSrcSlotIndex, nLocalDstSlotIndex);

}

static void playerRequestUseLocalQuickSlot(uint32_t iLocalPosition)
{

	CPythonPlayer::GetInstance()->RequestUseLocalQuickSlot(iLocalPosition);


}

static uint32_t playerLocalQuickSlotIndexToGlobalQuickSlotIndex(uint32_t iLocalSlotIndex)
{

	auto rkPlayer=CPythonPlayer::GetInstance();
	return  rkPlayer->LocalQuickSlotIndexToGlobalQuickSlotIndex(iLocalSlotIndex);
}

static int32_t playerGetQuickPage()
{
	return  CPythonPlayer::GetInstance()->GetQuickPage();
}

static void playerSetQuickPage(int32_t iPageIndex)
{

	CPythonPlayer::GetInstance()->SetQuickPage(iPageIndex);

}

static std::tuple<uint32_t, uint32_t> playerGetLocalQuickSlot(uint32_t iSlotIndex)
{

	uint32_t dwWndType;
	uint32_t dwWndItemPos;

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->GetLocalQuickSlotData(iSlotIndex, &dwWndType, &dwWndItemPos);

	return std::make_tuple( dwWndType, dwWndItemPos);
}

static std::tuple<uint32_t, uint32_t> playerGetGlobalQuickSlot(uint32_t iSlotIndex)
{

	uint32_t dwWndType;
	uint32_t dwWndItemPos;

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->GetGlobalQuickSlotData(iSlotIndex, &dwWndType, &dwWndItemPos);

	return std::make_tuple( dwWndType, dwWndItemPos);
}

static void playerRemoveQuickSlotByValue(uint32_t iType, uint32_t iPosition)
{
	
	CPythonPlayer::GetInstance()->RemoveQuickSlotByValue(iType, iPosition);


}

static char playerisItem(uint16_t iSlotIndex)
{
	return CPythonPlayer::GetInstance()->IsItem(TItemPos(INVENTORY, iSlotIndex));
}

static bool playerIsEquipmentSlot(uint16_t iSlotIndex)
{
	return iSlotIndex >= c_Equipment_Start && iSlotIndex <= c_DragonSoul_Equip_End;
}

static bool playerIsDSEquipmentSlot(uint8_t bWindowType, uint16_t iSlotIndex)
{
	return INVENTORY == bWindowType && iSlotIndex >= c_DragonSoul_Equip_Start && iSlotIndex <= c_DragonSoul_Equip_End;
}

static bool playerIsCostumeSlot(uint16_t iSlotIndex)
{
	return iSlotIndex >= c_Costume_Slot_Start && iSlotIndex <= c_Costume_Slot_End;
}

static bool playerIsValuableItem(uint8_t window, uint16_t cell)
{
	TItemPos SlotIndex(window, cell);
	uint32_t dwItemIndex = CPythonPlayer::GetInstance()->GetItemIndex(SlotIndex);
	CItemManager::GetInstance()->SelectItemData(dwItemIndex);
	CItemData* pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Can't find item data");

	bool hasMetinSocket = false;
	bool isHighPrice = false;

	for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
		if (CPythonPlayer::METIN_SOCKET_TYPE_NONE != CPythonPlayer::GetInstance()->GetItemMetinSocket(SlotIndex, i))
			hasMetinSocket = true;

	uint32_t dwValue = pItemData->GetISellItemPrice();
	if (dwValue > 5000)
		isHighPrice = true;

	return hasMetinSocket || isHighPrice;
}

static bool playerIsValuableItem2(uint16_t cell)
{
	return playerIsValuableItem(INVENTORY, cell);
}

static bool playerIsOpenPrivateShop()
{
	return  CPythonPlayer::GetInstance()->IsOpenPrivateShop();
}

#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
static char playerIsBeltInventorySlot(uint16_t iSlotIndex)
{
	return CPythonPlayer::GetInstance()->IsBeltInventorySlot(TItemPos(INVENTORY, iSlotIndex));
}

static bool playerIsEquippingBelt()
{
	const auto player = CPythonPlayer::GetInstance();
	bool bEquipping = false;

	const TItemData* data = player->GetItemData(TItemPos(EQUIPMENT, c_Equipment_Belt));

	if (NULL != data)
		bEquipping = 0 < data->count;

	return  bEquipping;	

}

static bool playerIsAvailableBeltInventoryCell(uint16_t pos)
{
	const auto player = CPythonPlayer::GetInstance();
	const TItemData* pData = player->GetItemData(TItemPos(EQUIPMENT, c_Equipment_Belt));

	if (NULL == pData || 0 == pData->count)
		return  false;

	CItemManager::GetInstance()->SelectItemData(pData->vnum);
	CItemData * pItem = CItemManager::GetInstance()->GetSelectedItemDataPointer();

	int32_t beltGrade = pItem->GetValue(0);

	return CBeltInventoryHelper::IsAvailableCell(pos - c_Belt_Inventory_Slot_Start, beltGrade);
}
#endif

static int32_t playerGetItemGrade(uint8_t window, uint16_t cell)
{
	int32_t iItemIndex = CPythonPlayer::GetInstance()->GetItemIndex(TItemPos(window, cell));
	CItemManager::GetInstance()->SelectItemData(iItemIndex);
	CItemData* pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Can't find item data");

	return GetItemGrade(pItemData->GetName());
}

static int32_t playerGetItemGrade2(uint16_t cell)
{
	return playerGetItemGrade(INVENTORY, cell);
}

static int32_t playerCanRefine(int32_t iScrollItemIndex, uint8_t window, uint16_t cell)
{
	TItemPos TargetSlotIndex(window, cell);
	
	if (CPythonPlayer::GetInstance()->IsEquipmentSlot(TargetSlotIndex))
	{
		return int32_t(REFINE_CANT_REFINE_METIN_TO_EQUIPMENT);
	}

	// Scroll
	CItemManager::GetInstance()->SelectItemData(iScrollItemIndex);
	CItemData* pScrollItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pScrollItemData)
		return int32_t(REFINE_CANT);
	int32_t iScrollType = pScrollItemData->GetType();
	int32_t iScrollSubType = pScrollItemData->GetSubType();
	if (iScrollType != ITEM::TYPE_USE)
		return int32_t(REFINE_CANT);
	if (iScrollSubType != ITEM::USE_TUNING)
		return int32_t(REFINE_CANT);

	// Target Item
	int32_t iTargetItemIndex = CPythonPlayer::GetInstance()->GetItemIndex(TargetSlotIndex);
	CItemManager::GetInstance()->SelectItemData(iTargetItemIndex);
	CItemData* pTargetItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pTargetItemData)
		return int32_t(REFINE_CANT);
	int32_t iTargetType = pTargetItemData->GetType();
	//int32_t iTargetSubType = pTargetItemData->GetSubType();
	if (ITEM::TYPE_ROD == iTargetType)
		return int32_t(REFINE_CANT_REFINE_ROD);

	if (pTargetItemData->HasNextGrade())
	{
		return int32_t(REFINE_OK);
	}
	else
	{
		return int32_t(REFINE_NOT_NEXT_GRADE_ITEM);
	}

	return int32_t(REFINE_CANT);
}

static int32_t playerCanRefine2(int32_t iScrollItemIndex,uint16_t cell)
{
	return playerCanRefine(iScrollItemIndex, INVENTORY, cell);
}

static int32_t playerCanDetach(int32_t iScrollitemIndex, uint8_t window, uint16_t cell)
{
	int32_t iScrollItemIndex = 0;
	TItemPos TargetSlotIndex(window, cell);


	// Scroll
	CItemManager::GetInstance()->SelectItemData(iScrollItemIndex);
	CItemData* pScrollItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pScrollItemData)
		throw std::runtime_error("Can't find item data");
	int32_t iScrollType = pScrollItemData->GetType();
	int32_t iScrollSubType = pScrollItemData->GetSubType();
	if (iScrollType != ITEM::TYPE_USE)
		return int32_t(DETACH_METIN_CANT);
	if (iScrollSubType != ITEM::USE_DETACHMENT)
		return int32_t(DETACH_METIN_CANT);

	// Target Item
	int32_t iTargetItemIndex = CPythonPlayer::GetInstance()->GetItemIndex(TargetSlotIndex);
	CItemManager::GetInstance()->SelectItemData(iTargetItemIndex);
	CItemData* pTargetItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pTargetItemData)
		throw std::runtime_error("Can't find item data");
	//int32_t iTargetType = pTargetItemData->GetType();
	//int32_t iTargetSubType = pTargetItemData->GetSubType();

	if (pTargetItemData->IsFlag(ITEM::FLAG_REFINEABLE))
	{
		for (int32_t iSlotCount = 0; iSlotCount < ITEM::SOCKET_MAX_NUM; ++iSlotCount)
			if (CPythonPlayer::GetInstance()->GetItemMetinSocket(TargetSlotIndex, iSlotCount) > 2)
			{
				return int32_t(DETACH_METIN_OK);
			}
	}

	return int32_t(DETACH_METIN_CANT);
}

static int32_t playerCanDetach2(int32_t iScrollitemIndex, uint16_t cell)
{
	return playerCanDetach(iScrollitemIndex, INVENTORY, cell);
}

static bool playerCanUnlock(int32_t iKeyItemIndex, uint8_t window, uint16_t cell)
{
	TItemPos TargetSlotIndex;

	// Key
	CItemManager::GetInstance()->SelectItemData(iKeyItemIndex);
	CItemData* pKeyItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pKeyItemData)
		throw std::runtime_error("Can't find item data");
	int32_t iKeyType = pKeyItemData->GetType();
	if (iKeyType != ITEM::TYPE_TREASURE_KEY)
		return false;

	// Target Item
	int32_t iTargetItemIndex = CPythonPlayer::GetInstance()->GetItemIndex(TargetSlotIndex);
	CItemManager::GetInstance()->SelectItemData(iTargetItemIndex);
	CItemData* pTargetItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pTargetItemData)
		throw std::runtime_error("Can't find item data");
	int32_t iTargetType = pTargetItemData->GetType();
	if (iTargetType != ITEM::TYPE_TREASURE_BOX)
		return false;

	return true;
}

static bool playerCanUnlock2(int32_t iKeyItemIndex, uint16_t cell)
{
	return playerCanUnlock(iKeyItemIndex, INVENTORY, cell);
}

static int32_t playerCanAttachMetin(int32_t iMetinItemID, uint8_t window, uint16_t cell)
{
	TItemPos TargetSlotIndex;

	if (CPythonPlayer::GetInstance()->IsEquipmentSlot(TargetSlotIndex))
	{
		return int32_t(ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT);
	}

	CItemData* pMetinItemData;
	if (!CItemManager::GetInstance()->GetItemDataPointer(iMetinItemID, &pMetinItemData))
		throw std::runtime_error("can't find item data");

	uint32_t dwTargetItemIndex = CPythonPlayer::GetInstance()->GetItemIndex(TargetSlotIndex);
	CItemData* pTargetItemData;
	if (!CItemManager::GetInstance()->GetItemDataPointer(dwTargetItemIndex, &pTargetItemData))
		throw std::runtime_error("can't find item data");

	uint32_t dwMetinWearFlags = pMetinItemData->GetWearFlags();
	uint32_t dwTargetWearFlags = pTargetItemData->GetWearFlags();
	if (0 == (dwMetinWearFlags & dwTargetWearFlags))
		return int32_t(ATTACH_METIN_NOT_MATCHABLE_ITEM);
	if (ITEM::TYPE_ROD == pTargetItemData->GetType())
		return int32_t(ATTACH_METIN_CANT);

	bool bNotExistGoldSocket = false;

	int32_t iSubType = pMetinItemData->GetSubType();
	for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
	{
		uint32_t dwSocketType = CPythonPlayer::GetInstance()->GetItemMetinSocket(TargetSlotIndex, i);
		if (ITEM::METIN_NORMAL == iSubType)
		{
			if (CPythonPlayer::METIN_SOCKET_TYPE_SILVER == dwSocketType ||
				CPythonPlayer::METIN_SOCKET_TYPE_GOLD == dwSocketType)
			{
				return int32_t(ATTACH_METIN_OK);
			}
		}
		else if (ITEM::METIN_GOLD == iSubType)
		{
			if (CPythonPlayer::METIN_SOCKET_TYPE_GOLD == dwSocketType)
			{
				return int32_t(ATTACH_METIN_OK);
			}
			else if (CPythonPlayer::METIN_SOCKET_TYPE_SILVER == dwSocketType)
			{
				bNotExistGoldSocket = true;
			}
		}
	}

	if (bNotExistGoldSocket)
	{
		return int32_t(ATTACH_METIN_NOT_EXIST_GOLD_SOCKET);
	}

	return int32_t(ATTACH_METIN_NO_MATCHABLE_SOCKET);
}

static int32_t playerCanAttachMetin2(int32_t iMetinItemID, uint16_t cell)
{
	return playerCanAttachMetin(iMetinItemID, INVENTORY, cell);
}

static bool playerIsRefineGradeScroll(uint8_t window, uint16_t cell)
{
	int32_t iScrollItemIndex = CPythonPlayer::GetInstance()->GetItemIndex(TItemPos(window, cell));
	CItemManager::GetInstance()->SelectItemData(iScrollItemIndex);
	CItemData* pScrollItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pScrollItemData)
		throw std::runtime_error("Can't find item data");

	return REFINE_SCROLL_TYPE_UP_GRADE == pScrollItemData->GetValue(0);
}

static bool playerIsRefineGradeScroll2(uint16_t cell)
{
	return playerIsRefineGradeScroll(INVENTORY, cell);
}

static void playerClearTarget()
{
	CPythonPlayer::GetInstance()->SetTarget(0);

}

static void playerSetTarget(uint32_t iVID)
{

	CPythonPlayer::GetInstance()->SetTarget(iVID);

}

static void playerOpenCharacterMenu(uint32_t iVID)
{

	CPythonPlayer::GetInstance()->OpenCharacterMenu(iVID);

}

static void playerUpdate()
{
	CPythonPlayer::GetInstance()->Update();

}

static void playerRender()
{

}

static void playerClear()
{
	CPythonPlayer::GetInstance()->ClearSkillDict();

}

static bool playerIsPartyMember(uint32_t iVID)
{

	return  CPythonPlayer::GetInstance()->IsPartyMemberByVID(iVID);
}

static bool playerIsPartyLeader(uint32_t iVID)
{
	uint32_t dwPID;
	if (!CPythonPlayer::GetInstance()->PartyMemberVIDToPID(iVID, &dwPID))
		return false;
	CPythonPlayer::TPartyMemberInfo * pPartyMemberInfo;
	if (!CPythonPlayer::GetInstance()->GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
		return false;

	return  CPythonPlayer::PARTY_ROLE_LEADER == pPartyMemberInfo->byState;
}

static bool playerIsPartyLeaderByPID(uint32_t iPID)
{

	CPythonPlayer::TPartyMemberInfo * pPartyMemberInfo;
	if (!CPythonPlayer::GetInstance()->GetPartyMemberPtr(iPID, &pPartyMemberInfo))
		return  false;

	return  CPythonPlayer::PARTY_ROLE_LEADER == pPartyMemberInfo->byState;
}

static uint8_t playerGetPartyMemberHPPercentage(uint32_t iPID)
{

	CPythonPlayer::TPartyMemberInfo * pPartyMemberInfo;
	if (!CPythonPlayer::GetInstance()->GetPartyMemberPtr(iPID, &pPartyMemberInfo))
		return  0;
	return  pPartyMemberInfo->byHPPercentage;
}

static uint8_t playerGetPartyMemberState(uint32_t iPID)
{

	CPythonPlayer::TPartyMemberInfo * pPartyMemberInfo;
	if (!CPythonPlayer::GetInstance()->GetPartyMemberPtr(iPID, &pPartyMemberInfo))
		return  0;

	return  pPartyMemberInfo->byState;
}

static std::tuple<int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, int16_t> playerGetPartyMemberAffects(uint32_t iPID)
{

	CPythonPlayer::TPartyMemberInfo * pPartyMemberInfo;
	if (!CPythonPlayer::GetInstance()->GetPartyMemberPtr(iPID, &pPartyMemberInfo))
		return std::make_tuple(0, 0, 0, 0, 0, 0, 0);

	return std::make_tuple(pPartyMemberInfo->sAffects[0],
		pPartyMemberInfo->sAffects[1],
		pPartyMemberInfo->sAffects[2],
		pPartyMemberInfo->sAffects[3],
		pPartyMemberInfo->sAffects[4],
		pPartyMemberInfo->sAffects[5],
		pPartyMemberInfo->sAffects[6]);
}

static void playerRemovePartyMember(uint32_t iPID)
{

	CPythonPlayer::GetInstance()->RemovePartyMember(iPID);

}

static void playerExitParty()
{
	CPythonPlayer::GetInstance()->ExitParty();

}

static uint32_t playerGetPKMode()
{
	return  CPythonPlayer::GetInstance()->GetPKMode();
}

static void playerSetWeaponAttackBonusFlag(uint32_t iFlag)
{


}

static BOOL playerToggleCoolTime()
{
	return  CPythonPlayer::GetInstance()->__ToggleCoolTime();
}

static BOOL playerToggleLevelLimit()
{
	return  CPythonPlayer::GetInstance()->__ToggleLevelLimit();
}

static uint32_t playerGetTargetVID()
{
	return  CPythonPlayer::GetInstance()->GetTargetVID();
}

static void playerSetItemData(uint16_t iSlotIndex, uint32_t iVirtualID, uint8_t iNum)
{

	TItemData kItemInst;
	ZeroMemory(&kItemInst, sizeof(kItemInst));
	kItemInst.vnum=iVirtualID;
	kItemInst.count=iNum;
	CPythonPlayer::GetInstance()->SetItemData(TItemPos(INVENTORY, iSlotIndex), kItemInst);

}

static void playerSetItemMetinSocket(uint8_t window, uint16_t cell, int32_t iMetinSocketNumber, int32_t iNum)
{
	CPythonPlayer::GetInstance()->SetItemMetinSocket(TItemPos(window, cell), iMetinSocketNumber, iNum);
}

static void playerSetItemMetinSocket2(uint16_t cell, int32_t iMetinSocketNumber, int32_t iNum)
{
	CPythonPlayer::GetInstance()->SetItemMetinSocket(TItemPos(INVENTORY, cell), iMetinSocketNumber, iNum);
}

static void playerSetItemAttribute(uint8_t window, uint16_t cell, int32_t iAttributeSlotIndex, int32_t iAttributeType, int32_t iAttributeValue)
{
	CPythonPlayer::GetInstance()->SetItemAttribute(TItemPos(window, cell), iAttributeSlotIndex, iAttributeType, iAttributeValue);
}

static void playerSetItemAttribute2(uint16_t cell, int32_t iAttributeSlotIndex, int32_t iAttributeType, int32_t iAttributeValue)
{
	CPythonPlayer::GetInstance()->SetItemAttribute(TItemPos(INVENTORY, cell), iAttributeSlotIndex, iAttributeType, iAttributeValue);
}

static void playerSetItemCount(uint8_t window, uint16_t cell, uint8_t bCount)
{
	CPythonPlayer::GetInstance()->SetItemCount(TItemPos(window, cell), bCount);
}

static void playerSetItemCount2(uint8_t window, uint16_t cell, uint8_t bCount)
{
	CPythonPlayer::GetInstance()->SetItemCount(TItemPos(INVENTORY, cell), bCount);
}

static std::string playerGetItemLink(uint8_t window, uint16_t cell)
{
	TItemPos Cell(window, cell);
	const TItemData* pPlayerItem = CPythonPlayer::GetInstance()->GetItemData(Cell);
	CItemData* pItemData = NULL;
	char buf[1024];

	if (pPlayerItem && CItemManager::GetInstance()->GetItemDataPointer(pPlayerItem->vnum, &pItemData))
	{
		char itemlink[256];
		int32_t len;
		bool isAttr = false;

		len = snprintf(itemlink, sizeof(itemlink), "item:%x:%x:%x:%x:%x",
			pPlayerItem->vnum, pPlayerItem->flags,
			pPlayerItem->alSockets[0], pPlayerItem->alSockets[1], pPlayerItem->alSockets[2]);

		for (int32_t i = 0; i < ITEM::ATTRIBUTE_MAX_NUM; ++i)
			if (pPlayerItem->aAttr[i].bType != 0)
			{
				len += snprintf(itemlink + len, sizeof(itemlink) - len, ":%x:%d",
					pPlayerItem->aAttr[i].bType, pPlayerItem->aAttr[i].sValue);
				isAttr = true;
			}


		//if (GetDefaultCodePage() == CP_ARABIC) {
		//	if (isAttr)
		//		snprintf(buf, sizeof(buf), "|cffffc700|H%s|h |h|r", itemlink);
		//	else
		//		snprintf(buf, sizeof(buf), "|cfff1e6c0|H%s|h |h|r", itemlink);
		//}
		//else 
		{
			if (isAttr)
				snprintf(buf, sizeof(buf), "|cffffc700|H%s|h |h|r", itemlink);
			else
				snprintf(buf, sizeof(buf), "|cfff1e6c0|H%s|h |h|r", itemlink);
		}
	}
	else
		buf[0] = '\0';

	return buf;
}

static std::string playerGetItemLink2(uint16_t cell)
{
	return playerGetItemLink(INVENTORY, cell);
}

static uint8_t playerSlotTypeToInvenType(uint8_t slotType)
{
	return  SlotTypeToInvenType(slotType);
}

static void playerSendDragonSoulRefine(uint8_t bSubHeader, std::unordered_map<int32_t, std::tuple<uint8_t, uint16_t>> pDic)
{
	TItemPos RefineItemPoses[DS_REFINE_WINDOW_MAX_NUM];

	switch (bSubHeader)
	{
		case DS_SUB_HEADER_CLOSE:
			break;
		case DS_SUB_HEADER_DO_REFINE_GRADE:
		case DS_SUB_HEADER_DO_REFINE_STEP:
		case DS_SUB_HEADER_DO_REFINE_STRENGTH:
		{
			int32_t pos = 0;

			for (auto& [key,value] : pDic)
			{
				if (key > DS_REFINE_WINDOW_MAX_NUM)
					return throw std::runtime_error("Invalid dss dict");

				RefineItemPoses[key] = TItemPos(std::get<0>(value), std::get<1>(value));
			}
		}
		break;
	}

	auto rns = CPythonNetworkStream::GetInstance();
	rns->SendDragonSoulRefinePacket(bSubHeader, RefineItemPoses);
}

static void playerSendDragonSoulRefine2(uint8_t bSubHeader)
{
	std::unordered_map<int32_t, std::tuple<uint8_t, uint16_t>> emptyMap;
	playerSendDragonSoulRefine(bSubHeader, emptyMap);
}


PYBIND11_EMBEDDED_MODULE(player, m)
{
	m.def("GetAutoPotionInfo",	playerGetAutoPotionInfo);
	m.def("SetAutoPotionInfo",	playerSetAutoPotionInfo);
	m.def("PickCloseItem",	playerPickCloseItem);
	m.def("SetGameWindow",	playerSetGameWindow);
	m.def("RegisterEffect",	playerRegisterEffect);
	m.def("RegisterCacheEffect",	playerRegisterCacheEffect);
	m.def("SetMouseState",	playerSetMouseState);
	m.def("SetMouseFunc",	playerSetMouseFunc);
	m.def("GetMouseFunc",	playerGetMouseFunc);
	m.def("SetMouseMiddleButtonState",	playerSetMouseMiddleButtonState);
	m.def("SetMainCharacterIndex",	playerSetMainCharacterIndex);
	m.def("GetMainCharacterIndex",	playerGetMainCharacterIndex);
	m.def("GetMainCharacterName",	playerGetMainCharacterName);
	m.def("GetMainCharacterPosition",	playerGetMainCharacterPosition);
	m.def("IsMainCharacterIndex",	playerIsMainCharacterIndex);
	m.def("CanAttackInstance",	playerCanAttackInstance);
	m.def("IsActingEmotion",	playerIsActingEmotion);
	m.def("IsPVPInstance",	playerIsPVPInstance);
	m.def("IsSameEmpire",	playerIsSameEmpire);
	m.def("IsChallengeInstance",	playerIsChallengeInstance);
	m.def("IsRevengeInstance",	playerIsRevengeInstance);
	m.def("IsCantFightInstance",	playerIsCantFightInstance);
	m.def("GetCharacterDistance",	playerGetCharacterDistance);
	m.def("IsInSafeArea",	playerIsInSafeArea);
	m.def("IsMountingHorse",	playerIsMountingHorse);
	m.def("IsObserverMode",	playerIsObserverMode);
	m.def("ActEmotion",	playerActEmotion);
	m.def("ShowPlayer",	playerShowPlayer);
	m.def("HidePlayer",	playerHidePlayer);
	m.def("ComboAttack",	playerComboAttack);
	m.def("SetAutoCameraRotationSpeed",	playerSetAutoCameraRotationSpeed);
	m.def("SetAttackKeyState",	playerSetAttackKeyState);
	m.def("SetSingleDIKKeyState",	playerSetSingleDIKKeyState);
	m.def("EndKeyWalkingImmediately",	playerEndKeyWalkingImmediately);
	m.def("StartMouseWalking",	playerStartMouseWalking);
	m.def("EndMouseWalking",	playerEndMouseWalking);
	m.def("ResetCameraRotation",	playerResetCameraRotation);
	m.def("SetQuickCameraMode",	playerSetQuickCameraMode);
	m.def("SetSkill",	playerSetSkill);
	m.def("GetSkillIndex",	playerGetSkillIndex);
	m.def("GetSkillSlotIndex",	playerGetSkillSlotIndex);
	m.def("GetSkillGrade",	playerGetSkillGrade);
	m.def("GetSkillLevel",	playerGetSkillLevel);
	m.def("GetSkillCurrentEfficientPercentage",	playerGetSkillCurrentEfficientPercentage);
	m.def("GetSkillNextEfficientPercentage",	playerGetSkillNextEfficientPercentage);
	m.def("ClickSkillSlot",	playerClickSkillSlot);
	m.def("ChangeCurrentSkillNumberOnly",	playerChangeCurrentSkillNumberOnly);
	m.def("ClearSkillDict",	playerClearSkillDict);
	m.def("GetItemIndex",	playerGetItemIndex);
	m.def("GetItemIndex", playerGetItemIndex2);
	m.def("GetItemFlags",	playerGetItemFlags);
	m.def("GetItemFlags", playerGetItemFlags2);
	m.def("GetItemCount",	playerGetItemCount);
	m.def("GetItemCount", playerGetItemCount2);
	m.def("GetItemCountByVnum",	playerGetItemCountByVnum);
	m.def("GetItemMetinSocket",	playerGetItemMetinSocket);
	m.def("GetItemMetinSocket", playerGetItemMetinSocket2);
	m.def("GetItemAttribute",	playerGetItemAttribute);
	m.def("GetItemAttribute", playerGetItemAttribute2);
	m.def("GetISellItemPrice",	playerGetISellItemPrice);
	m.def("GetISellItemPrice", playerGetISellItemPrice2);
	m.def("MoveItem",	playerMoveItem);
	m.def("SendClickItemPacket",	playerSendClickItemPacket);
	m.def("GetName",	playerGetName);
	m.def("GetJob",	playerGetJob);
	m.def("GetRace",	playerGetRace);
	m.def("GetPlayTime",	playerGetPlayTime);
	m.def("SetPlayTime",	playerSetPlayTime);
	m.def("IsSkillCoolTime",	playerIsSkillCoolTime);
	m.def("GetSkillCoolTime",	playerGetSkillCoolTime);
	m.def("IsSkillActive",	playerIsSkillActive);
	m.def("UseGuildSkill",	playerUseGuildSkill);
	m.def("AffectIndexToSkillIndex",	playerAffectIndexToSkillIndex);
	m.def("GetEXP",	playerGetEXP);
	m.def("GetStatus",	playerGetStatus);
	m.def("SetStatus",	playerSetStatus);
	m.def("GetElk",	playerGetElk);
	m.def("GetMoney", playerGetElk);
	m.def("GetGuildID",	playerGetGuildID);
	m.def("GetGuildName",	playerGetGuildName);
	m.def("GetAlignmentData",	playerGetAlignmentData);
	m.def("RequestAddLocalQuickSlot",	playerRequestAddLocalQuickSlot);
	m.def("RequestAddToEmptyLocalQuickSlot",	playerRequestAddToEmptyLocalQuickSlot);
	m.def("RequestDeleteGlobalQuickSlot",	playerRequestDeleteGlobalQuickSlot);
	m.def("RequestMoveGlobalQuickSlotToLocalQuickSlot",	playerRequestMoveGlobalQuickSlotToLocalQuickSlot);
	m.def("RequestUseLocalQuickSlot",	playerRequestUseLocalQuickSlot);
	m.def("LocalQuickSlotIndexToGlobalQuickSlotIndex",	playerLocalQuickSlotIndexToGlobalQuickSlotIndex);
	m.def("GetQuickPage",	playerGetQuickPage);
	m.def("SetQuickPage",	playerSetQuickPage);
	m.def("GetLocalQuickSlot",	playerGetLocalQuickSlot);
	m.def("GetGlobalQuickSlot",	playerGetGlobalQuickSlot);
	m.def("RemoveQuickSlotByValue",	playerRemoveQuickSlotByValue);
	m.def("isItem",	playerisItem);
	m.def("IsEquipmentSlot",	playerIsEquipmentSlot);
	m.def("IsDSEquipmentSlot",	playerIsDSEquipmentSlot);
	m.def("IsCostumeSlot",	playerIsCostumeSlot);
	m.def("IsValuableItem",	playerIsValuableItem);
	m.def("IsValuableItem", playerIsValuableItem2);
	m.def("IsOpenPrivateShop",	playerIsOpenPrivateShop);
#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
	m.def("IsBeltInventorySlot",	playerIsBeltInventorySlot);
	m.def("IsEquippingBelt",	playerIsEquippingBelt);
	m.def("IsAvailableBeltInventoryCell",	playerIsAvailableBeltInventoryCell);
#endif
	m.def("GetItemGrade",	playerGetItemGrade);
	m.def("GetItemGrade", playerGetItemGrade2);
	m.def("CanRefine",	playerCanRefine);
	m.def("CanRefine", playerCanRefine2);
	m.def("CanDetach",	playerCanDetach);
	m.def("CanDetach", playerCanDetach2);
	m.def("CanUnlock",	playerCanUnlock);
	m.def("CanUnlock", playerCanUnlock2);
	m.def("CanAttachMetin",	playerCanAttachMetin);
	m.def("CanAttachMetin", playerCanAttachMetin2);
	m.def("IsRefineGradeScroll",	playerIsRefineGradeScroll);
	m.def("IsRefineGradeScroll", playerIsRefineGradeScroll2);
	m.def("ClearTarget",	playerClearTarget);
	m.def("SetTarget",	playerSetTarget);
	m.def("OpenCharacterMenu",	playerOpenCharacterMenu);
	m.def("Update",	playerUpdate);
	m.def("Render",	playerRender);
	m.def("Clear",	playerClear);
	m.def("IsPartyMember",	playerIsPartyMember);
	m.def("IsPartyLeader",	playerIsPartyLeader);
	m.def("IsPartyLeaderByPID",	playerIsPartyLeaderByPID);
	m.def("GetPartyMemberHPPercentage",	playerGetPartyMemberHPPercentage);
	m.def("GetPartyMemberState",	playerGetPartyMemberState);
	m.def("GetPartyMemberAffects",	playerGetPartyMemberAffects);
	m.def("RemovePartyMember",	playerRemovePartyMember);
	m.def("ExitParty",	playerExitParty);
	m.def("GetPKMode",	playerGetPKMode);
	m.def("SetWeaponAttackBonusFlag",	playerSetWeaponAttackBonusFlag);
	m.def("ToggleCoolTime",	playerToggleCoolTime);
	m.def("ToggleLevelLimit",	playerToggleLevelLimit);
	m.def("GetTargetVID",	playerGetTargetVID);
	m.def("SetItemData",	playerSetItemData);
	m.def("SetItemMetinSocket",	playerSetItemMetinSocket);
	m.def("SetItemMetinSocket", playerSetItemMetinSocket2);
	m.def("SetItemAttribute",	playerSetItemAttribute);
	m.def("SetItemAttribute", playerSetItemAttribute2);
	m.def("SetItemCount",	playerSetItemCount);
	m.def("SetItemCount", playerSetItemCount2);
	m.def("GetItemLink",	playerGetItemLink);
	m.def("GetItemLink", playerGetItemLink2);
	m.def("SlotTypeToInvenType",	playerSlotTypeToInvenType);
	m.def("SendDragonSoulRefine",	playerSendDragonSoulRefine);
	m.def("SendDragonSoulRefine", playerSendDragonSoulRefine2);

	m.attr("LEVEL") = int32_t(POINT_LEVEL);
	m.attr("VOICE") = int32_t(POINT_VOICE);
	m.attr("EXP") = int32_t(POINT_EXP);
	m.attr("NEXT_EXP") = int32_t(POINT_NEXT_EXP);
	m.attr("HP") = int32_t(POINT_HP);
	m.attr("MAX_HP") = int32_t(POINT_MAX_HP);
	m.attr("SP") = int32_t(POINT_SP);
	m.attr("MAX_SP") = int32_t(POINT_MAX_SP);
	m.attr("STAMINA") = int32_t(POINT_STAMINA);
	m.attr("MAX_STAMINA") = int32_t(POINT_MAX_STAMINA);
	m.attr("ELK") = int32_t(POINT_GOLD);
	m.attr("ST") = int32_t(POINT_ST);
	m.attr("HT") = int32_t(POINT_HT);
	m.attr("DX") = int32_t(POINT_DX);
	m.attr("IQ") = int32_t(POINT_IQ);
	m.attr("ATT_POWER") = int32_t(POINT_DEF_GRADE);
	m.attr("ATT_MIN" ) = int32_t(POINT_WEAPON_MIN);
	m.attr("ATT_MAX") = int32_t(POINT_WEAPON_MAX);
	m.attr("MIN_MAGIC_WEP") = int32_t(POINT_MIN_MAGIC_WEP);
	m.attr("MAX_MAGIC_WEP") = int32_t(POINT_MAX_MAGIC_WEP);
	m.attr("ATT_SPEED") = int32_t(POINT_ATT_SPEED);
	m.attr("ATT_BONUS") = int32_t(POINT_ATT_GRADE_BONUS);
	m.attr("EVADE_RATE") = int32_t(POINT_ATT_GRADE);
	m.attr("MOVING_SPEED") = int32_t(POINT_MOV_SPEED);
	m.attr("DEF_GRADE") = int32_t(POINT_CLIENT_DEF_GRADE);
	m.attr("DEF_BONUS") = int32_t(POINT_DEF_GRADE_BONUS);
	m.attr("CASTING_SPEED") = int32_t(POINT_CASTING_SPEED);
	m.attr("MAG_ATT") = int32_t(POINT_MAGIC_ATT_GRADE);
	m.attr("MAG_DEF") = int32_t(POINT_MAGIC_DEF_GRADE);
	m.attr("EMPIRE_POINT") = int32_t(POINT_EMPIRE_POINT);
	m.attr("STAT") = int32_t(POINT_STAT);
	m.attr("SKILL_PASSIVE") = int32_t(POINT_SUB_SKILL);
	m.attr("SKILL_SUPPORT") = int32_t(POINT_SUB_SKILL);
	m.attr("SKILL_ACTIVE") = int32_t(POINT_SKILL);
	m.attr("SKILL_HORSE") = int32_t(POINT_HORSE_SKILL);
	m.attr("PLAYTIME") = int32_t(POINT_PLAYTIME);
	m.attr("BOW_DISTANCE") = int32_t(POINT_BOW_DISTANCE);
	m.attr("HP_RECOVERY") = int32_t(POINT_HP_RECOVERY);
	m.attr("SP_RECOVERY") = int32_t(POINT_SP_RECOVERY);
	m.attr("ATTACKER_BONUS") = int32_t(POINT_PARTY_ATTACKER_BONUS);
	m.attr("MAX_NUM") = int32_t(POINT_MAX_NUM);
	m.attr("POINT_CRITICAL_PCT") = int32_t(POINT_CRITICAL_PCT);
	m.attr("POINT_PENETRATE_PCT") = int32_t(POINT_PENETRATE_PCT);
	m.attr("POINT_MALL_ATTBONUS") = int32_t(POINT_MALL_ATTBONUS);
	m.attr("POINT_MALL_DEFBONUS") = int32_t(POINT_MALL_DEFBONUS);
	m.attr("POINT_MALL_EXPBONUS") = int32_t(POINT_MALL_EXPBONUS);
	m.attr("POINT_MALL_ITEMBONUS") = int32_t(POINT_MALL_ITEMBONUS);
	m.attr("POINT_MALL_GOLDBONUS") = int32_t(POINT_MALL_GOLDBONUS);
	m.attr("POINT_MAX_HP_PCT") = int32_t(POINT_MAX_HP_PCT);
	m.attr("POINT_MAX_SP_PCT") = int32_t(POINT_MAX_SP_PCT);
	m.attr("POINT_SKILL_DAMAGE_BONUS") = int32_t(POINT_SKILL_DAMAGE_BONUS);
	m.attr("POINT_NORMAL_HIT_DAMAGE_BONUS") = int32_t(POINT_NORMAL_HIT_DAMAGE_BONUS);
	m.attr("POINT_SKILL_DEFEND_BONUS") = int32_t(POINT_SKILL_DEFEND_BONUS);
	m.attr("POINT_NORMAL_HIT_DEFEND_BONUS") = int32_t(POINT_NORMAL_HIT_DEFEND_BONUS);
	m.attr("SKILL_GRADE_NORMAL") = int32_t(CPythonPlayer::SKILL_NORMAL);
	m.attr("SKILL_GRADE_MASTER") = int32_t(CPythonPlayer::SKILL_MASTER);
	m.attr("SKILL_GRADE_GRAND_MASTER") = int32_t(CPythonPlayer::SKILL_GRAND_MASTER);
	m.attr("SKILL_GRADE_PERFECT_MASTER") = int32_t(CPythonPlayer::SKILL_PERFECT_MASTER);
	m.attr("CATEGORY_ACTIVE") = int32_t(CPythonPlayer::CATEGORY_ACTIVE);
	m.attr("CATEGORY_PASSIVE") = int32_t(CPythonPlayer::CATEGORY_PASSIVE);
	m.attr("INVENTORY_PAGE_SIZE")=	c_Inventory_Page_Size;
	m.attr("INVENTORY_PAGE_COUNT")=	c_Inventory_Page_Count;
	m.attr("INVENTORY_SLOT_COUNT")=	c_Inventory_Count;
	m.attr("EQUIPMENT_SLOT_START")=	c_Equipment_Start;
	m.attr("EQUIPMENT_PAGE_COUNT")=	c_Equipment_Count;
#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
	m.attr("NEW_EQUIPMENT_SLOT_START")=	c_New_Equipment_Start;
	m.attr("NEW_EQUIPMENT_SLOT_COUNT") =	c_New_Equipment_Count;
#endif
	m.attr("MBF_SKILL") = int32_t(CPythonPlayer::MBF_SKILL);
	m.attr("MBF_ATTACK") = int32_t(CPythonPlayer::MBF_ATTACK);
	m.attr("MBF_CAMERA") = int32_t(CPythonPlayer::MBF_CAMERA);
	m.attr("MBF_SMART") = int32_t(CPythonPlayer::MBF_SMART);
	m.attr("MBF_MOVE") = int32_t(CPythonPlayer::MBF_MOVE);
	m.attr("MBF_AUTO") = int32_t(CPythonPlayer::MBF_AUTO);
	m.attr("MBS_PRESS") = int32_t(CPythonPlayer::MBS_PRESS);
	m.attr("MBS_CLICK") = int32_t(CPythonPlayer::MBS_CLICK);
	m.attr("MBT_RIGHT") = int32_t(CPythonPlayer::MBT_RIGHT);
	m.attr("MBT_LEFT") = int32_t(CPythonPlayer::MBT_LEFT);
	m.attr("SLOT_TYPE_NONE") = int32_t(SLOT_TYPE_NONE);
	m.attr("SLOT_TYPE_INVENTORY") = int32_t(SLOT_TYPE_INVENTORY);
	m.attr("SLOT_TYPE_SKILL") = int32_t(SLOT_TYPE_SKILL);
	m.attr("SLOT_TYPE_SHOP") = int32_t(SLOT_TYPE_SHOP);
	m.attr("SLOT_TYPE_EXCHANGE_OWNER") = int32_t(SLOT_TYPE_EXCHANGE_OWNER);
	m.attr("SLOT_TYPE_EXCHANGE_TARGET") = int32_t(SLOT_TYPE_EXCHANGE_TARGET);
	m.attr("SLOT_TYPE_QUICK_SLOT") = int32_t(SLOT_TYPE_QUICK_SLOT);
	m.attr("SLOT_TYPE_SAFEBOX") = int32_t(SLOT_TYPE_SAFEBOX);
	m.attr("SLOT_TYPE_PRIVATE_SHOP") = int32_t(SLOT_TYPE_PRIVATE_SHOP);
	m.attr("SLOT_TYPE_MALL") = int32_t(SLOT_TYPE_MALL);
	m.attr("SLOT_TYPE_EMOTION") = int32_t(SLOT_TYPE_EMOTION);
	m.attr("SLOT_TYPE_DRAGON_SOUL_INVENTORY") = int32_t(SLOT_TYPE_DRAGON_SOUL_INVENTORY);
	m.attr("RESERVED_WINDOW") = int32_t(RESERVED_WINDOW);
	m.attr("INVENTORY") = int32_t(INVENTORY);
	m.attr("EQUIPMENT") = int32_t(EQUIPMENT);
	m.attr("SAFEBOX") = int32_t(SAFEBOX);
	m.attr("MALL") = int32_t(MALL);
	m.attr("DRAGON_SOUL_INVENTORY") = int32_t(DRAGON_SOUL_INVENTORY);
	m.attr("GROUND") = int32_t(GROUND);
	m.attr("ITEM_MONEY")=	-1;
	m.attr("SKILL_SLOT_COUNT")=	uint16_t(SKILL_MAX_NUM);
	m.attr("EFFECT_PICK") = int32_t(CPythonPlayer::EFFECT_PICK);
	m.attr("METIN_SOCKET_TYPE_NONE") = int32_t(CPythonPlayer::METIN_SOCKET_TYPE_NONE);
	m.attr("METIN_SOCKET_TYPE_SILVER") = int32_t(CPythonPlayer::METIN_SOCKET_TYPE_SILVER);
	m.attr("METIN_SOCKET_TYPE_GOLD") = int32_t(CPythonPlayer::METIN_SOCKET_TYPE_GOLD);
	m.attr("METIN_SOCKET_MAX_NUM") = int32_t(ITEM::SOCKET_MAX_NUM);
	m.attr("ATTRIBUTE_SLOT_MAX_NUM") = int32_t(ITEM::ATTRIBUTE_MAX_NUM);
	m.attr("REFINE_CANT") = int32_t(REFINE_CANT);
	m.attr("REFINE_OK") = int32_t(REFINE_OK);
	m.attr("REFINE_ALREADY_MAX_SOCKET_COUNT") = int32_t(REFINE_ALREADY_MAX_SOCKET_COUNT);
	m.attr("REFINE_NEED_MORE_GOOD_SCROLL") = int32_t(REFINE_NEED_MORE_GOOD_SCROLL);
	m.attr("REFINE_CANT_MAKE_SOCKET_ITEM") = int32_t(REFINE_CANT_MAKE_SOCKET_ITEM);
	m.attr("REFINE_NOT_NEXT_GRADE_ITEM") = int32_t(REFINE_NOT_NEXT_GRADE_ITEM);
	m.attr("REFINE_CANT_REFINE_METIN_TO_EQUIPMENT") = int32_t(REFINE_CANT_REFINE_METIN_TO_EQUIPMENT);
	m.attr("REFINE_CANT_REFINE_ROD") = int32_t(REFINE_CANT_REFINE_ROD);
	m.attr("ATTACH_METIN_CANT") = int32_t(ATTACH_METIN_CANT);
	m.attr("ATTACH_METIN_OK") = int32_t(ATTACH_METIN_OK);
	m.attr("ATTACH_METIN_NOT_MATCHABLE_ITEM") = int32_t(ATTACH_METIN_NOT_MATCHABLE_ITEM);
	m.attr("ATTACH_METIN_NO_MATCHABLE_SOCKET") = int32_t(ATTACH_METIN_NO_MATCHABLE_SOCKET);
	m.attr("ATTACH_METIN_NOT_EXIST_GOLD_SOCKET") = int32_t(ATTACH_METIN_NOT_EXIST_GOLD_SOCKET);
	m.attr("ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT") = int32_t(ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT);
	m.attr("DETACH_METIN_CANT") = int32_t(DETACH_METIN_CANT);
	m.attr("DETACH_METIN_OK") = int32_t(DETACH_METIN_OK);
	m.attr("PARTY_STATE_NORMAL") = int32_t(CPythonPlayer::PARTY_ROLE_NORMAL);
	m.attr("PARTY_STATE_LEADER") = int32_t(CPythonPlayer::PARTY_ROLE_LEADER);
	m.attr("PARTY_STATE_ATTACKER") = int32_t(CPythonPlayer::PARTY_ROLE_ATTACKER);
	m.attr("PARTY_STATE_TANKER") = int32_t(CPythonPlayer::PARTY_ROLE_TANKER);
	m.attr("PARTY_STATE_BUFFER") = int32_t(CPythonPlayer::PARTY_ROLE_BUFFER);
	m.attr("PARTY_STATE_SKILL_MASTER") = int32_t(CPythonPlayer::PARTY_ROLE_SKILL_MASTER);
	m.attr("PARTY_STATE_BERSERKER") = int32_t(CPythonPlayer::PARTY_ROLE_BERSERKER);
	m.attr("PARTY_STATE_DEFENDER") = int32_t(CPythonPlayer::PARTY_ROLE_DEFENDER);
	m.attr("PARTY_STATE_MAX_NUM") = int32_t(CPythonPlayer::PARTY_ROLE_MAX_NUM);
	m.attr("SKILL_INDEX_TONGSOL")=	c_iSkillIndex_Tongsol;
	m.attr("SKILL_INDEX_FISHING")=	c_iSkillIndex_Fishing;
	m.attr("SKILL_INDEX_MINING")=	c_iSkillIndex_Mining;
	m.attr("SKILL_INDEX_MAKING")=	c_iSkillIndex_Making;
	m.attr("SKILL_INDEX_COMBO")=	c_iSkillIndex_Combo;
	m.attr("SKILL_INDEX_LANGUAGE1")=	c_iSkillIndex_Language1;
	m.attr("SKILL_INDEX_LANGUAGE2")=	c_iSkillIndex_Language2;
	m.attr("SKILL_INDEX_LANGUAGE3")=	c_iSkillIndex_Language3;
	m.attr("SKILL_INDEX_POLYMORPH")=	c_iSkillIndex_Polymorph;
	m.attr("SKILL_INDEX_RIDING")=	c_iSkillIndex_Riding;
	m.attr("SKILL_INDEX_SUMMON")=	c_iSkillIndex_Summon;
	m.attr("PK_MODE_PEACE") = int32_t(PK_MODE_PEACE);
	m.attr("PK_MODE_REVENGE") = int32_t(PK_MODE_REVENGE);
	m.attr("PK_MODE_FREE") = int32_t(PK_MODE_FREE);
	m.attr("PK_MODE_PROTECT") = int32_t(PK_MODE_PROTECT);
	m.attr("PK_MODE_GUILD") = int32_t(PK_MODE_GUILD);
	m.attr("PK_MODE_MAX_NUM") = int32_t(PK_MODE_MAX_NUM);
	m.attr("BLOCK_EXCHANGE") = int32_t(BLOCK_EXCHANGE);
	m.attr("BLOCK_PARTY") = int32_t(BLOCK_PARTY_INVITE);
	m.attr("BLOCK_GUILD") = int32_t(BLOCK_GUILD_INVITE);
	m.attr("BLOCK_WHISPER") = int32_t(BLOCK_WHISPER);
	m.attr("BLOCK_FRIEND") = int32_t(BLOCK_MESSENGER_INVITE);
	m.attr("BLOCK_PARTY_REQUEST") = int32_t(BLOCK_PARTY_REQUEST);
	m.attr("PARTY_EXP_NON_DISTRIBUTION") = int32_t(PARTY_EXP_DISTRIBUTION_NON_PARITY);
	m.attr("PARTY_EXP_DISTRIBUTION_PARITY") = int32_t(PARTY_EXP_DISTRIBUTION_PARITY);
	m.attr("EMOTION_CLAP") = int32_t(EMOTION_CLAP);
	m.attr("EMOTION_CHEERS_1") = int32_t(EMOTION_CHEERS_1);
	m.attr("EMOTION_CHEERS_2") = int32_t(EMOTION_CHEERS_2);
	m.attr("EMOTION_DANCE_1") = int32_t(EMOTION_DANCE_1);
	m.attr("EMOTION_DANCE_2") = int32_t(EMOTION_DANCE_2);
	m.attr("EMOTION_DANCE_3") = int32_t(EMOTION_DANCE_3);
	m.attr("EMOTION_DANCE_4") = int32_t(EMOTION_DANCE_4);
	m.attr("EMOTION_DANCE_5") = int32_t(EMOTION_DANCE_5);
	m.attr("EMOTION_DANCE_6") = int32_t(EMOTION_DANCE_6);
	m.attr("EMOTION_CONGRATULATION") = int32_t(EMOTION_CONGRATULATION);
	m.attr("EMOTION_FORGIVE") = int32_t(EMOTION_FORGIVE);
	m.attr("EMOTION_ANGRY") = int32_t(EMOTION_ANGRY);
	m.attr("EMOTION_ATTRACTIVE") = int32_t(EMOTION_ATTRACTIVE);
	m.attr("EMOTION_SAD") = int32_t(EMOTION_SAD);
	m.attr("EMOTION_SHY") = int32_t(EMOTION_SHY);
	m.attr("EMOTION_CHEERUP") = int32_t(EMOTION_CHEERUP);
	m.attr("EMOTION_BANTER") = int32_t(EMOTION_BANTER);
	m.attr("EMOTION_JOY") = int32_t(EMOTION_JOY);
	m.attr("EMOTION_KISS") = int32_t(EMOTION_KISS);
	m.attr("EMOTION_FRENCH_KISS") = int32_t(EMOTION_FRENCH_KISS);
	m.attr("EMOTION_SLAP") = int32_t(EMOTION_SLAP);
	m.attr("AUTO_POTION_TYPE_HP") = int32_t(CPythonPlayer::AUTO_POTION_TYPE_HP);
	m.attr("AUTO_POTION_TYPE_SP") = int32_t(CPythonPlayer::AUTO_POTION_TYPE_SP);
	m.attr("DRAGON_SOUL_PAGE_SIZE")=	c_DragonSoul_Inventory_Box_Size;
	m.attr("DRAGON_SOUL_PAGE_COUNT") = int32_t(DRAGON_SOUL_GRADE_MAX);
	m.attr("DRAGON_SOUL_SLOT_COUNT")=	c_DragonSoul_Inventory_Count;
	m.attr("DRAGON_SOUL_EQUIPMENT_SLOT_START")=	c_DragonSoul_Equip_Start;
	m.attr("DRAGON_SOUL_EQUIPMENT_PAGE_COUNT") = int32_t(DRAGON_SOUL_DECK_MAX_NUM);
	m.attr("DRAGON_SOUL_EQUIPMENT_FIRST_SIZE")=	c_DragonSoul_Equip_Slot_Max;
	m.attr("DRAGON_SOUL_REFINE_CLOSE") = int32_t(DS_SUB_HEADER_CLOSE);
	m.attr("DS_SUB_HEADER_DO_UPGRADE") = int32_t(DS_SUB_HEADER_DO_REFINE_GRADE);
	m.attr("DS_SUB_HEADER_DO_IMPROVEMENT") = int32_t(DS_SUB_HEADER_DO_REFINE_STEP);
	m.attr("DS_SUB_HEADER_DO_REFINE") = int32_t(DS_SUB_HEADER_DO_REFINE_STRENGTH);

	m.attr("DIR_UP") = int32_t(CPythonPlayer::DIR_UP);
	m.attr("DIR_DOWN") = int32_t(CPythonPlayer::DIR_DOWN);
	m.attr("DIR_LEFT") = int32_t(CPythonPlayer::DIR_LEFT);
	m.attr("DIR_RIGHT") = int32_t(CPythonPlayer::DIR_RIGHT);
}
