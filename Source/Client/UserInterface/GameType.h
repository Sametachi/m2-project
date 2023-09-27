#pragma once
#include "../../Libraries/gameLib/ItemData.h"
#include <Core/Constants/Item.hpp>
#include <Common/tables.h>

struct SAffects
{
	enum
	{
		AFFECT_MAX_NUM = 32,
	};

	SAffects() : dwAffects(0) {}
	SAffects(const uint32_t & c_rAffects)
	{
		__SetAffects(c_rAffects);
	}
	int32_t operator = (const uint32_t & c_rAffects)
	{
		__SetAffects(c_rAffects);
	}

	BOOL IsAffect(uint8_t byIndex)
	{
		return dwAffects & (1 << byIndex);
	}

	void __SetAffects(const uint32_t & c_rAffects)
	{
		dwAffects = c_rAffects;
	}

	uint32_t dwAffects;
};

extern std::string g_strGuildSymbolPathName;

const uint32_t c_Name_Max_Length = 64;
const uint32_t c_FileName_Max_Length = 128;
const uint32_t c_Short_Name_Max_Length = 32;

const uint32_t c_Inventory_Page_Size = 5*9; // x*y
const uint32_t c_Inventory_Page_Count = 2;
const uint32_t c_ItemSlot_Count = c_Inventory_Page_Size * c_Inventory_Page_Count;
const uint32_t c_Equipment_Count = 12;

const uint32_t c_Equipment_Start = c_ItemSlot_Count;

const uint32_t c_Equipment_Body	= c_Equipment_Start + 0;
const uint32_t c_Equipment_Head	= c_Equipment_Start + 1;
const uint32_t c_Equipment_Shoes	= c_Equipment_Start + 2;
const uint32_t c_Equipment_Wrist	= c_Equipment_Start + 3;
const uint32_t c_Equipment_Weapon	= c_Equipment_Start + 4;
const uint32_t c_Equipment_Neck	= c_Equipment_Start + 5;
const uint32_t c_Equipment_Ear		= c_Equipment_Start + 6;
const uint32_t c_Equipment_Unique1	= c_Equipment_Start + 7;
const uint32_t c_Equipment_Unique2	= c_Equipment_Start + 8;
const uint32_t c_Equipment_Arrow	= c_Equipment_Start + 9;
const uint32_t c_Equipment_Shield	= c_Equipment_Start + 10;

// ���� �߰��� �ű� ���� & ��Ʈ
// ������ �����ۿ� �Ҵ��� �� �ִ� ��ġ�� ���� ���, ä��� ����Ʈ ����, �ڽ�Ƭ �ý��� ������ ���ؼ� ������ �߷��ִ�.
// �̰� �� ä��� ���� ������ ����������ó�� ������ ���� ������ ��������... ����
// 
// �����ϸ�, ���� ���â���� ����DB�� ������ �������� 90 ~ 102 �̰�,
// 2013�� �ʿ� ���� �߰��Ǵ� ���Ե��� 111 ~ ���� �����Ѵ�. ���� ��񿡼� �ִ�� ����� �� �ִ� ���� 121 �����̰�, 122���ʹ� ��ȥ������ ����Ѵ�.
#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
	const uint32_t c_New_Equipment_Start = c_Equipment_Start + 21;
	const uint32_t c_New_Equipment_Count = 3;
	const uint32_t c_Equipment_Ring1 = c_New_Equipment_Start + 0;
	const uint32_t c_Equipment_Ring2 = c_New_Equipment_Start + 1;
	const uint32_t c_Equipment_Belt  = c_New_Equipment_Start + 2;;
#endif

enum EDragonSoulGradeTypes
{
	DRAGON_SOUL_GRADE_NORMAL,
	DRAGON_SOUL_GRADE_BRILLIANT,
	DRAGON_SOUL_GRADE_RARE,
	DRAGON_SOUL_GRADE_ANCIENT,
	DRAGON_SOUL_GRADE_LEGENDARY,
	DRAGON_SOUL_GRADE_MAX,

};

enum EDragonSoulStepTypes
{
	DRAGON_SOUL_STEP_LOWEST,
	DRAGON_SOUL_STEP_LOW,
	DRAGON_SOUL_STEP_MID,
	DRAGON_SOUL_STEP_HIGH,
	DRAGON_SOUL_STEP_HIGHEST,
	DRAGON_SOUL_STEP_MAX,
};

	const uint32_t c_Costume_Slot_Start	= c_Equipment_Start + 19;	// [����] ����(19) �ϵ��ڵ� ����. ���� �������� �ڽ��� ������ 19������. ���� common/length.h ������ EWearPositions ������ ����.
	const uint32_t	c_Costume_Slot_Body		= c_Costume_Slot_Start + 0;
	const uint32_t	c_Costume_Slot_Hair		= c_Costume_Slot_Start + 1;
	const uint32_t c_Costume_Slot_Count	= 2;
	const uint32_t c_Costume_Slot_End		= c_Costume_Slot_Start + c_Costume_Slot_Count;


// [����] ����(32) �ϵ��ڵ� ����. ���� �������� ��ȥ�� ������ 32������. 
// ���� common/length.h ������ EWearPositions �������� 32���� Ȯ��� ���� �����ϰ�(32 �̻��� Ȯ�� �ϱ� ����� �Ǿ�����.), 
// �� ���ĺ��͸� ��ȥ�� ���� �������� ���.
const uint32_t c_Wear_Max = 32;
const uint32_t c_DragonSoul_Equip_Start = c_ItemSlot_Count + c_Wear_Max;
const uint32_t c_DragonSoul_Equip_Slot_Max = 6;
const uint32_t c_DragonSoul_Equip_End = c_DragonSoul_Equip_Start + c_DragonSoul_Equip_Slot_Max * DRAGON_SOUL_DECK_MAX_NUM;

// NOTE: 2013�� 2�� 5�� ����... ��ȥ�� ��ũ�� 2���� �����ϴµ�, ���� Ȯ�� ���ɼ��� �־ 3�� ��ũ �������� �Ҵ� �� ��. �� �� ������ ��Ʈ �κ��丮�� ���
const uint32_t c_DragonSoul_Equip_Reserved_Count = c_DragonSoul_Equip_Slot_Max * 3;		

#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
	// ��Ʈ �������� �����ϴ� �κ��丮
	const uint32_t c_Belt_Inventory_Slot_Start = c_DragonSoul_Equip_End + c_DragonSoul_Equip_Reserved_Count;
	const uint32_t c_Belt_Inventory_Width = 4;
	const uint32_t c_Belt_Inventory_Height= 4;
	const uint32_t c_Belt_Inventory_Slot_Count = c_Belt_Inventory_Width * c_Belt_Inventory_Height;
	const uint32_t c_Belt_Inventory_Slot_End = c_Belt_Inventory_Slot_Start + c_Belt_Inventory_Slot_Count;

	const uint32_t c_Inventory_Count	= c_Belt_Inventory_Slot_End;
#else
	const uint32_t c_Inventory_Count	= c_DragonSoul_Equip_End;
#endif

// ��ȥ�� ���� �κ��丮
const uint32_t c_DragonSoul_Inventory_Start = 0;
const uint32_t c_DragonSoul_Inventory_Box_Size = 32;
const uint32_t c_DragonSoul_Inventory_Count = ITEM::DS_SLOT_MAX * DRAGON_SOUL_GRADE_MAX * c_DragonSoul_Inventory_Box_Size;
const uint32_t c_DragonSoul_Inventory_End = c_DragonSoul_Inventory_Start + c_DragonSoul_Inventory_Count;

enum ESlotType
{
	SLOT_TYPE_NONE,
	SLOT_TYPE_INVENTORY,
	SLOT_TYPE_SKILL,
	SLOT_TYPE_EMOTION,
	SLOT_TYPE_SHOP,
	SLOT_TYPE_EXCHANGE_OWNER,
	SLOT_TYPE_EXCHANGE_TARGET,
	SLOT_TYPE_QUICK_SLOT,
	SLOT_TYPE_SAFEBOX,
	SLOT_TYPE_PRIVATE_SHOP,
	SLOT_TYPE_MALL,
	SLOT_TYPE_DRAGON_SOUL_INVENTORY,
	SLOT_TYPE_MAX,
};

enum EDSInventoryMaxNum
{
	DS_INVENTORY_MAX_NUM = c_DragonSoul_Inventory_Count,
	DS_REFINE_WINDOW_MAX_NUM = 15,
};

const uint32_t c_QuickBar_Line_Count = 3;
const uint32_t c_QuickBar_Slot_Count = 12;

const float c_Idle_WaitTime = 5.0f;

const int32_t c_Monster_Race_Start_Number = 6;
const int32_t c_Monster_Model_Start_Number = 20001;

const float c_fAttack_Delay_Time = 0.2f;
const float c_fHit_Delay_Time = 0.1f;
const float c_fCrash_Wave_Time = 0.2f;
const float c_fCrash_Wave_Distance = 3.0f;

const float c_fHeight_Step_Distance = 50.0f;

enum
{
	DISTANCE_TYPE_FOUR_WAY,
	DISTANCE_TYPE_EIGHT_WAY,
	DISTANCE_TYPE_ONE_WAY,
	DISTANCE_TYPE_MAX_NUM,
};

const float c_fMagic_Script_Version = 1.0f;
const float c_fSkill_Script_Version = 1.0f;
const float c_fMagicSoundInformation_Version = 1.0f;
const float c_fBattleCommand_Script_Version = 1.0f;
const float c_fEmotionCommand_Script_Version = 1.0f;
const float c_fActive_Script_Version = 1.0f;
const float c_fPassive_Script_Version = 1.0f;

// Used by PushMove
const float c_fWalkDistance = 175.0f;
const float c_fRunDistance = 310.0f;

#define FILE_MAX_LEN 128

#pragma pack(push)
#pragma pack(1)

typedef struct SQuickSlot
{
	uint8_t Type;
	uint8_t Position;
} TQuickSlot;

typedef struct packet_item
{
    uint32_t       vnum;
    uint8_t        count;
	uint32_t		flags;
	uint32_t		anti_flags;
	int32_t		alSockets[ITEM::SOCKET_MAX_NUM];
    TPlayerItemAttribute aAttr[ITEM::ATTRIBUTE_MAX_NUM];
} TItemData;

#pragma pack(pop)

inline float GetSqrtDistance(int32_t ix1, int32_t iy1, int32_t ix2, int32_t iy2) // By sqrt
{
	float dx, dy;

	dx = float(ix1 - ix2);
	dy = float(iy1 - iy2);

	return sqrtf(dx*dx + dy*dy);
}

void SetGuildSymbolPath(const char * c_szPathName);
const char * GetGuildSymbolFileName(uint32_t dwGuildID);
uint8_t SlotTypeToInvenType(uint8_t bSlotType);
