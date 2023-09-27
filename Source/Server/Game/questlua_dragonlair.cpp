
#include "stdafx.h"

#include "questmanager.h"
#include "DragonLair.h"
#include "char.h"
#include "guild.h"

namespace quest
{
	int32_t dl_startRaid(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t baseMapIndex = lua_tonumber(L, -1);

		CDragonLairManager::GetInstance()->Start(ch->GetMapIndex(), baseMapIndex, ch->GetGuild()->GetID());

		return 0;
	}

	void RegisterDragonLairFunctionTable()
	{
		luaL_reg dl_functions[] =
		{
			{	"startRaid",	dl_startRaid	},

			{	NULL,			NULL			}
		};

		CQuestManager::GetInstance()-> AddLuaFunctionTable("DragonLair", dl_functions);
	}
}

