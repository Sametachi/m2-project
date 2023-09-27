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

// 새로 추가된 신규 반지 & 벨트
// 장착형 아이템에 할당할 수 있는 위치가 기존 장비, 채기랍 퀘스트 보상, 코스튬 시스템 등으로 인해서 공간이 잘려있다.
// 이게 다 채기랍 보상 버프를 장착아이템처럼 구현한 ㅅㄲ 때문에 난리났따... ㅆㅂ
// 
// 정리하면, 기존 장비창들은 서버DB상 아이템 포지션이 90 ~ 102 이고,
// 2013년 초에 새로 추가되는 슬롯들은 111 ~ 부터 시작한다. 착용 장비에서 최대로 사용할 수 있는 값은 121 까지이고, 122부터는 용혼석에서 사용한다.
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

	const uint32_t c_Costume_Slot_Start	= c_Equipment_Start + 19;	// [주의] 숫자(19) 하드코딩 주의. 현재 서버에서 코스츔 슬롯은 19부터임. 서버 common/length.h 파일의 EWearPositions 열거형 참고.
	const uint32_t	c_Costume_Slot_Body		= c_Costume_Slot_Start + 0;
	const uint32_t	c_Costume_Slot_Hair		= c_Costume_Slot_Start + 1;
	const uint32_t c_Costume_Slot_Count	= 2;
	const uint32_t c_Costume_Slot_End		= c_Costume_Slot_Start + c_Costume_Slot_Count;


// [주의] 숫자(32) 하드코딩 주의. 현재 서버에서 용혼석 슬롯은 32부터임. 
// 서버 common/length.h 파일의 EWearPositions 열거형이 32까지 확장될 것을 염두하고(32 이상은 확장 하기 힘들게 되어있음.), 
// 그 이후부터를 용혼석 장착 슬롯으로 사용.
const uint32_t c_Wear_Max = 32;
const uint32_t c_DragonSoul_Equip_Start = c_ItemSlot_Count + c_Wear_Max;
const uint32_t c_DragonSoul_Equip_Slot_Max = 6;
const uint32_t c_DragonSoul_Equip_End = c_DragonSoul_Equip_Start + c_DragonSoul_Equip_Slot_Max * DRAGON_SOUL_DECK_MAX_NUM;

// NOTE: 2013년 2월 5일 현재... 용혼석 데크는 2개가 존재하는데, 향후 확장 가능성이 있어서 3개 데크 여유분을 할당 해 둠. 그 뒤 공간은 벨트 인벤토리로 사용
const uint32_t c_DragonSoul_Equip_Reserved_Count = c_DragonSoul_Equip_Slot_Max * 3;		

#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
	// 벨트 아이템이 제공하는 인벤토리
	const uint32_t c_Belt_Inventory_Slot_Start = c_DragonSoul_Equip_End + c_DragonSoul_Equip_Reserved_Count;
	const uint32_t c_Belt_Inventory_Width = 4;
	const uint32_t c_Belt_Inventory_Height= 4;
	const uint32_t c_Belt_Inventory_Slot_Count = c_Belt_Inventory_Width * c_Belt_Inventory_Height;
	const uint32_t c_Belt_Inventory_Slot_End = c_Belt_Inventory_Slot_Start + c_Belt_Inventory_Slot_Count;

	const uint32_t c_Inventory_Count	= c_Belt_Inventory_Slot_End;
#else
	const uint32_t c_Inventory_Count	= c_DragonSoul_Equip_End;
#endif

// 용혼석 전용 인벤토리
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
