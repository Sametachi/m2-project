#include "stdafx.h"
#include "config.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "char.h"
#include "affect.h"
#include "db.h"

namespace quest
{
	//
	// "affect" Lua functions
	//
	int32_t affect_add(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("invalid argument");
			return 0;
		}

		auto q = CQuestManager::GetInstance();

		uint8_t applyOn = (uint8_t) lua_tonumber(L, 1);

		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		if (applyOn >= ITEM::MAX_APPLY_NUM || applyOn < 1)
		{
			SysLog("apply is out of range : {}", applyOn);
			return 0;
		}

		if (ch->FindAffect(AFFECT_QUEST_START_IDX, applyOn))
			return 0;

		int32_t value = (int32_t) lua_tonumber(L, 2);
		int32_t duration = (int32_t) lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_QUEST_START_IDX, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	int32_t affect_remove(lua_State * L)
	{
		auto q = CQuestManager::GetInstance();
		int32_t iType;

		if (lua_isnumber(L, 1))
		{
			iType = (int32_t) lua_tonumber(L, 1);

			if (iType == 0)
				iType = q->GetCurrentPC()->GetCurrentQuestIndex() + AFFECT_QUEST_START_IDX;
		}
		else
			iType = q->GetCurrentPC()->GetCurrentQuestIndex() + AFFECT_QUEST_START_IDX;

		q->GetCurrentCharacterPtr()->RemoveAffect(iType);

		return 0;
	}

	int32_t affect_remove_bad(lua_State * L) 
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		ch->RemoveBadAffect();
		return 0;
	}

	int32_t affect_remove_good(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		ch->RemoveGoodAffect();
		return 0;
	}

	int32_t affect_add_hair(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("invalid argument");
			return 0;
		}

		auto q = CQuestManager::GetInstance();

		uint8_t applyOn = (uint8_t) lua_tonumber(L, 1);

		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		if (applyOn >= ITEM::MAX_APPLY_NUM || applyOn < 1)
		{
			SysLog("apply is out of range : {}", applyOn);
			return 0;
		}

		int32_t value = (int32_t) lua_tonumber(L, 2);
		int32_t duration = (int32_t) lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_HAIR, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	int32_t affect_remove_hair(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		CAffect* pAff = ch->FindAffect(AFFECT_HAIR);

		if (pAff != nullptr)
		{
			lua_pushnumber(L, pAff->lDuration);
			ch->RemoveAffect(pAff);
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}
	
	int32_t affect_get_apply_on(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1))
		{
			SysLog("invalid argument");
			return 0;
		}

		uint32_t affectType = lua_tonumber(L, 1);

		CAffect* pAff = ch->FindAffect(affectType);

		if (pAff != nullptr)
			lua_pushnumber(L, pAff->bApplyOn);
		else
			lua_pushnil(L);

		return 1;

	}

	int32_t affect_add_collect(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("invalid argument");
			return 0;
		}

		auto q = CQuestManager::GetInstance();

		uint8_t applyOn = (uint8_t) lua_tonumber(L, 1);

		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		if (applyOn >= ITEM::MAX_APPLY_NUM || applyOn < 1)
		{
			SysLog("apply is out of range : {}", applyOn);
			return 0;
		}

		int32_t value = (int32_t) lua_tonumber(L, 2);
		int32_t duration = (int32_t) lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_COLLECT, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	int32_t affect_add_collect_point(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("invalid argument");
			return 0;
		}

		auto q = CQuestManager::GetInstance();

		uint8_t point_type = (uint8_t) lua_tonumber(L, 1);

		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		if (point_type >= POINT_MAX_NUM || point_type < 1)
		{
			SysLog("point is out of range : {}", point_type);
			return 0;
		}

		int32_t value = (int32_t) lua_tonumber(L, 2);
		int32_t duration = (int32_t) lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_COLLECT, point_type, value, 0, duration, 0, false);

		return 0;
	}

	int32_t affect_remove_collect(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch != nullptr)
		{
			uint8_t bApply = (uint8_t)lua_tonumber(L, 1);

			if (bApply >= ITEM::MAX_APPLY_NUM) return 0;

			bApply = aApplyInfo[bApply].bPointType;
			int32_t value = (int32_t)lua_tonumber(L, 2);

			const std::list<CAffect*>& rList = ch->GetAffectContainer();
			const CAffect* pAffect = nullptr;

			for (std::list<CAffect*>::const_iterator iter = rList.begin(); iter != rList.end(); ++iter)
			{
				pAffect = *iter;

				if (pAffect->dwType == AFFECT_COLLECT)
				{
					if (pAffect->bApplyOn == bApply && pAffect->lApplyValue == value)
					{
						break;
					}
				}

				pAffect = nullptr;
			}

			if (pAffect != nullptr)
			{
				ch->RemoveAffect(const_cast<CAffect*>(pAffect));
			}
		}

		return 0;
	}

	int32_t affect_remove_all_collect(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch != nullptr)
		{
			ch->RemoveAffect(AFFECT_COLLECT);
		}

		return 0;
	}

	void RegisterAffectFunctionTable()
	{
		luaL_reg affect_functions[] =
		{
			{ "add",		affect_add		},
			{ "remove",		affect_remove		},
			{ "remove_bad",	affect_remove_bad	},
			{ "remove_good",	affect_remove_good	},
			{ "add_hair",		affect_add_hair		},
			{ "remove_hair",	affect_remove_hair		},
			{ "add_collect",		affect_add_collect		},
			{ "add_collect_point",		affect_add_collect_point		},
			{ "remove_collect",		affect_remove_collect	},
			{ "remove_all_collect",	affect_remove_all_collect	},
			{ "get_apply_on",	affect_get_apply_on },

			{ NULL,		NULL			}
		};

		CQuestManager::GetInstance()->AddLuaFunctionTable("affect", affect_functions);
	}
};
