#pragma once
#include <Core/Constants/Controls.hpp>
#include <Core/Constants/Item.hpp>

#define WORD_MAX 0xffff
enum EMisc
{
	MAX_HOST_LENGTH					= 15,
	IP_ADDRESS_LENGTH				= 15,
	LOGIN_MAX_LEN					= 30,
	PASSWD_MAX_LEN					= 16,
	PLAYER_PER_ACCOUNT				= 4,
	ACCOUNT_STATUS_MAX_LEN			= 8,
	SHOP_SIGN_MAX_LEN				= 32,
	INVENTORY_MAX_NUM				= 90,
	ABILITY_MAX_NUM					= 50,
	EMPIRE_MAX_NUM					= 4,
	BANWORD_MAX_LEN					= 24,
	SMS_MAX_LEN						= 80,
	MOBILE_MAX_LEN					= 32,
	SOCIAL_ID_MAX_LEN				= 18,

	GUILD_NAME_MAX_LEN				= 12,

	SHOP_HOST_ITEM_MAX_NUM			= 40,
	SHOP_GUEST_ITEM_MAX_NUM 		= 18,

	SHOP_PRICELIST_MAX_NUM			= 40,

	CHAT_MAX_LEN					= 512,

	JOURNAL_MAX_NUM					= 2,

	QUERY_MAX_LEN					= 8192,

	FILE_MAX_LEN					= 128,

	PLAYER_EXP_TABLE_MAX			= 120,
	PLAYER_MAX_LEVEL_CONST			= 120,

	GUILD_MAX_LEVEL					= 20,
	MOB_MAX_LEVEL					= 100,

	ATTRIBUTE_MAX_VALUE				= 20,
	CHARACTER_PATH_MAX_NUM			= 64,
	SKILLBOOK_DELAY_MIN				= 64800,
	SKILLBOOK_DELAY_MAX				= 108000, 
	SKILL_MAX_LEVEL					= 40,

	APPLY_NAME_MAX_LEN				= 32,
	EVENT_FLAG_NAME_MAX_LEN 		= 32,

    POINT_MAX_NUM					= 255,
	
	MAX_AMOUNT_OF_MALL_BONUS		= 20,

	WEAR_MAX_NUM					= 32,

	GOLD_MAX						= 2000000000,

	MAX_PASSPOD						= 8 ,

	OPENID_AUTHKEY_LEN				= 32, 

	SHOP_TAB_NAME_MAX 				= 32,
	SHOP_TAB_COUNT_MAX				= 3,

	BELT_INVENTORY_SLOT_WIDTH		= 4,
	BELT_INVENTORY_SLOT_HEIGHT		= 4,

	BELT_INVENTORY_SLOT_COUNT = BELT_INVENTORY_SLOT_WIDTH * BELT_INVENTORY_SLOT_HEIGHT,
};

enum EMatrixCard
{
	MATRIX_CODE_MAX_LEN		= 192,
	MATRIX_ANSWER_MAX_LEN	= 8,
};

enum EWearPositions
{
	WEAR_BODY,
	WEAR_HEAD,
	WEAR_FOOTS,
	WEAR_WRIST,
	WEAR_WEAPON,
	WEAR_NECK,
	WEAR_EAR,
	WEAR_UNIQUE1,
	WEAR_UNIQUE2,
	WEAR_ARROW,
	WEAR_SHIELD,
    WEAR_ABILITY1,
    WEAR_ABILITY2,
    WEAR_ABILITY3,
    WEAR_ABILITY4,
    WEAR_ABILITY5,
    WEAR_ABILITY6,
    WEAR_ABILITY7,
    WEAR_ABILITY8,
	WEAR_COSTUME_BODY,
	WEAR_COSTUME_HAIR,
	
	WEAR_RING1,
	WEAR_RING2,

	WEAR_BELT,

	WEAR_MAX = 32
};

enum EDragonSoulDeckType
{
	DRAGON_SOUL_DECK_0,
	DRAGON_SOUL_DECK_1,
	
	DRAGON_SOUL_DECK_MAX_NUM = 2,
	DRAGON_SOUL_DECK_RESERVED_MAX_NUM = 3,
};

enum ESex
{
	SEX_MALE,
	SEX_FEMALE
};

enum EDirection
{
	DIR_NORTH,
	DIR_NORTHEAST,
	DIR_EAST,
	DIR_SOUTHEAST,
	DIR_SOUTH,
	DIR_SOUTHWEST,
	DIR_WEST,
	DIR_NORTHWEST,
	DIR_MAX_NUM
};

enum EAbilityDifficulty
{
	DIFFICULTY_EASY,
	DIFFICULTY_NORMAL,
	DIFFICULTY_HARD,
	DIFFICULTY_VERY_HARD,
	DIFFICULTY_NUM_TYPES
};

enum EAbilityCategory
{
	CATEGORY_PHYSICAL,
	CATEGORY_MENTAL,
	CATEGORY_ATTRIBUTE,
	CATEGORY_NUM_TYPES
};

enum EJobs
{
	JOB_WARRIOR,
	JOB_ASSASSIN,
	JOB_SURA,
	JOB_SHAMAN,
	JOB_MAX_NUM
};

enum ESkillGroups
{
	SKILL_GROUP_MAX_NUM = 2,
};

enum ELoads
{
	LOAD_NONE,
	LOAD_LIGHT,
	LOAD_NORMAL,
	LOAD_HEAVY,
	LOAD_MASSIVE
};

enum
{
	QUICKSLOT_TYPE_NONE,
	QUICKSLOT_TYPE_ITEM,
	QUICKSLOT_TYPE_SKILL,
	QUICKSLOT_TYPE_COMMAND,
	QUICKSLOT_TYPE_MAX_NUM,
};

enum EChatType
{
	CHAT_TYPE_TALKING,
	CHAT_TYPE_INFO,
	CHAT_TYPE_NOTICE,
	CHAT_TYPE_PARTY,
	CHAT_TYPE_GUILD,
	CHAT_TYPE_COMMAND,
	CHAT_TYPE_SHOUT,
	CHAT_TYPE_WHISPER,
	CHAT_TYPE_BIG_NOTICE,
	CHAT_TYPE_MONARCH_NOTICE,
	CHAT_TYPE_MAX_NUM
};

enum EWhisperType
{
	WHISPER_TYPE_NORMAL				= 0,
	WHISPER_TYPE_NOT_EXIST			= 1,
	WHISPER_TYPE_TARGET_BLOCKED		= 2,
	WHISPER_TYPE_SENDER_BLOCKED		= 3,
	WHISPER_TYPE_ERROR				= 4,
	WHISPER_TYPE_GM					= 5,
	WHISPER_TYPE_SYSTEM				= 0xFF
};

enum ECharacterPosition
{
	POSITION_GENERAL,
	POSITION_BATTLE,
	POSITION_DYING,
	POSITION_SITTING_CHAIR,
	POSITION_SITTING_GROUND,
	POSITION_INTRO,
	POSITION_MAX_NUM
};

enum EGMLevels
{
	GM_PLAYER,
	GM_LOW_WIZARD,
	GM_WIZARD,
	GM_HIGH_WIZARD,
	GM_GOD,
	GM_IMPLEMENTOR
};

enum EOnClickEvents
{
	ON_CLICK_NONE,
	ON_CLICK_SHOP,
	ON_CLICK_TALK,
	ON_CLICK_MAX_NUM
};

enum EOnIdleEvents
{
	ON_IDLE_NONE,
	ON_IDLE_GENERAL,
	ON_IDLE_MAX_NUM
};

enum EWindows
{
	RESERVED_WINDOW,
	INVENTORY,
	EQUIPMENT,
	SAFEBOX,
	MALL,
	DRAGON_SOUL_INVENTORY,
	BELT_INVENTORY,

	GROUND
};

enum
{
	SKILL_ATTR_TYPE_NORMAL = 1,
	SKILL_ATTR_TYPE_MELEE,
	SKILL_ATTR_TYPE_RANGE,
	SKILL_ATTR_TYPE_MAGIC
	/*
		SKILL_ATTR_TYPE_FIRE,
		SKILL_ATTR_TYPE_ICE,
		SKILL_ATTR_TYPE_ELEC,
		SKILL_ATTR_TYPE_DARK,
	*/
};

enum
{
	SKILL_NORMAL,
	SKILL_MASTER,
	SKILL_GRAND_MASTER,
	SKILL_PERFECT_MASTER,
};

enum EGuildWarType
{
	GUILD_WAR_TYPE_FIELD,
	GUILD_WAR_TYPE_BATTLE,
	GUILD_WAR_TYPE_FLAG,
	GUILD_WAR_TYPE_MAX_NUM
};

enum EGuildWarState
{
	GUILD_WAR_NONE,
	GUILD_WAR_SEND_DECLARE,
	GUILD_WAR_REFUSE,
	GUILD_WAR_RECV_DECLARE,
	GUILD_WAR_WAIT_START,
	GUILD_WAR_CANCEL,
	GUILD_WAR_ON_WAR,
	GUILD_WAR_END,
	GUILD_WAR_OVER,
	GUILD_WAR_RESERVE,

	GUILD_WAR_DURATION		= 30 * 60,
	GUILD_WAR_WIN_POINT		= 1000,
	GUILD_WAR_LADDER_HALF_PENALTY_TIME	= 12 * 60 * 60,
};

enum EAttributeSet 
{            
	ATTRIBUTE_SET_WEAPON,
	ATTRIBUTE_SET_BODY, 
	ATTRIBUTE_SET_WRIST, 
	ATTRIBUTE_SET_FOOTS,
	ATTRIBUTE_SET_NECK,
	ATTRIBUTE_SET_HEAD,
	ATTRIBUTE_SET_SHIELD,
	ATTRIBUTE_SET_EAR,
	ATTRIBUTE_SET_MAX_NUM
};  

enum EPrivType
{
	PRIV_NONE,
	PRIV_ITEM_DROP,
	PRIV_GOLD_DROP,
	PRIV_GOLD10_DROP,
	PRIV_EXP_PCT,
	MAX_PRIV_NUM,
};

enum EMoneyLogType
{
	MONEY_LOG_RESERVED,
	MONEY_LOG_MONSTER,
	MONEY_LOG_SHOP,
	MONEY_LOG_REFINE,
	MONEY_LOG_QUEST,
	MONEY_LOG_GUILD,
	MONEY_LOG_MISC,
	MONEY_LOG_MONSTER_KILL,
	MONEY_LOG_DROP,
	MONEY_LOG_TYPE_MAX_NUM,
};

enum EPremiumTypes
{
	PREMIUM_EXP,
	PREMIUM_ITEM,
	PREMIUM_SAFEBOX,
	PREMIUM_AUTOLOOT,
	PREMIUM_FISH_MIND,
	PREMIUM_MARRIAGE_FAST,
	PREMIUM_GOLD,
	
	PREMIUM_MAX_NUM = 9
};

enum SPECIAL_EFFECT
{
	SE_NONE,

	SE_HPUP_RED,
	SE_SPUP_BLUE,
	SE_SPEEDUP_GREEN,
	SE_DXUP_PURPLE,
	SE_CRITICAL,
	SE_PENETRATE,
	SE_BLOCK,
	SE_DODGE,
	SE_CHINA_FIREWORK,
	SE_SPIN_TOP,
	SE_SUCCESS,
	SE_FAIL,
	SE_FR_SUCCESS,
	SE_LEVELUP_ON_14_FOR_GERMANY,
	SE_LEVELUP_UNDER_15_FOR_GERMANY,
	SE_PERCENT_DAMAGE1,
	SE_PERCENT_DAMAGE2,
	SE_PERCENT_DAMAGE3,

	SE_AUTO_HPUP,
	SE_AUTO_SPUP,

	SE_EQUIP_RAMADAN_RING,
	SE_EQUIP_HALLOWEEN_CANDY,
	SE_EQUIP_HAPPINESS_RING,
	SE_EQUIP_LOVE_PENDANT,
} ;

enum ETeenFlags
{
	TEENFLAG_NONE = 0,
	TEENFLAG_1HOUR,
	TEENFLAG_2HOUR,
	TEENFLAG_3HOUR,
	TEENFLAG_4HOUR,
	TEENFLAG_5HOUR,
};

enum EDragonSoulRefineWindowSize
{
	DRAGON_SOUL_REFINE_GRID_MAX	= 15,
};

enum EMisc2
{
	DRAGON_SOUL_EQUIP_SLOT_START = INVENTORY_MAX_NUM + WEAR_MAX_NUM,
	DRAGON_SOUL_EQUIP_SLOT_END = DRAGON_SOUL_EQUIP_SLOT_START + (ITEM::DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM),
	DRAGON_SOUL_EQUIP_RESERVED_SLOT_END = DRAGON_SOUL_EQUIP_SLOT_END + (ITEM::DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM),

	BELT_INVENTORY_SLOT_START = DRAGON_SOUL_EQUIP_RESERVED_SLOT_END,
	BELT_INVENTORY_SLOT_END = BELT_INVENTORY_SLOT_START + BELT_INVENTORY_SLOT_COUNT,

	INVENTORY_AND_EQUIP_SLOT_MAX = BELT_INVENTORY_SLOT_END,
};

#pragma pack(push, 1)

typedef struct SItemPos
{
	uint8_t window_type;
	uint16_t cell;
    SItemPos ()
    {
        window_type = INVENTORY;
		cell = WORD_MAX;
    }

	SItemPos (uint8_t _window_type, uint16_t _cell)
    {
        window_type = _window_type;
        cell = _cell;
    }

	bool IsValidItemPosition() const
	{
		switch (window_type)
		{
			case RESERVED_WINDOW:
				return false;
				
			case INVENTORY:
			case EQUIPMENT:
			case BELT_INVENTORY:
				return cell < INVENTORY_AND_EQUIP_SLOT_MAX;
				
			case DRAGON_SOUL_INVENTORY:
				return cell < (ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM);

			case SAFEBOX:
			case MALL:
				return false;
				
			default:
				return false;
		}
		return false;
	}
	
	bool IsEquipPosition() const
	{
		return ((INVENTORY == window_type || EQUIPMENT == window_type) && cell >= INVENTORY_MAX_NUM && cell < INVENTORY_MAX_NUM + WEAR_MAX_NUM)
			|| IsDragonSoulEquipPosition();
	}

	bool IsDragonSoulEquipPosition() const
	{
		return (DRAGON_SOUL_EQUIP_SLOT_START <= cell) && (DRAGON_SOUL_EQUIP_SLOT_END > cell);
	}

	bool IsBeltInventoryPosition() const
	{
		return (BELT_INVENTORY_SLOT_START <= cell) && (BELT_INVENTORY_SLOT_END > cell);
	}

	bool IsDefaultInventoryPosition() const
	{
		return INVENTORY == window_type && cell < INVENTORY_MAX_NUM;
	}

	bool operator==(const struct SItemPos& rhs) const
	{
		return (window_type == rhs.window_type) && (cell == rhs.cell);
	}
	bool operator<(const struct SItemPos& rhs) const
	{
		return (window_type < rhs.window_type) || ((window_type == rhs.window_type) && (cell < rhs.cell));
	}
} TItemPos;

const TItemPos NPOS (RESERVED_WINDOW, WORD_MAX);

typedef enum
{
	SHOP_COIN_TYPE_GOLD,
	SHOP_COIN_TYPE_SECONDARY_COIN,
} EShopCoinType;

#pragma pack(pop)
