#include "stdafx.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"
#include "questevent.h"

namespace quest
{
	void CancelTimerEvent(LPEVENT* ppEvent)
	{
		quest_event_info* info = dynamic_cast<quest_event_info*>((*ppEvent)->info);

		if (info)
		{
			M2_DELETE_ARRAY(info->name);
			info->name = nullptr;
		}

		event_cancel(ppEvent);
	}

	EVENTFUNC(quest_server_timer_event)
	{
		quest_server_event_info * info = dynamic_cast<quest_server_event_info *>(event->info);

		if (info == nullptr)
		{
			SysLog("quest_server_timer_event> <Factor> Null pointer");
			return 0;
		}

		auto q = CQuestManager::GetInstance();

		if (!q->ServerTimer(info->npc_id, info->arg))
			return passes_per_sec / 2 + 1;

		if (0 == info->time_cycle)	// If it is not a loop, terminate it.
		{
			q->ClearServerTimerNotCancel(info->name, info->arg);
			M2_DELETE_ARRAY(info->name);
			info->name = nullptr;
		}

		return info->time_cycle;
	}

	EVENTFUNC(quest_timer_event)
	{
		quest_event_info * info = dynamic_cast<quest_event_info *>(event->info);

		if (info == nullptr)
		{
			SysLog("quest_timer_event> <Factor> Null pointer");
			return 0;
		}

		auto q = CQuestManager::GetInstance();

		if (CHARACTER_MANAGER::GetInstance()->FindByPID(info->player_id))
		{
			if (!CQuestManager::GetInstance()->Timer(info->player_id, info->npc_id))
				return (passes_per_sec / 2 + 1);

			if (0 == info->time_cycle)	// If it is not a loop, terminate it.
				goto END_OF_TIMER_EVENT;
		}
		else
		{
END_OF_TIMER_EVENT:
			PC* pPC = q->GetPC(info->player_id);

			if (pPC)
				pPC->RemoveTimerNotCancel(info->name);
			else
				SysLog("quest::PC pointer null. player_id: {}", info->player_id);

			M2_DELETE_ARRAY(info->name);
			info->name = nullptr;
			return 0;
		}

		return info->time_cycle;
	}

	LPEVENT quest_create_server_timer_event(const char* name, double when, uint32_t timernpc, bool loop, uint32_t arg)
	{
		const int32_t nameCapacity = strlen(name) + 1;

		int32_t ltime_cycle = (int32_t) (rint(PASSES_PER_SEC(when)));

		quest_server_event_info* info = AllocEventInfo<quest_server_event_info>();

		info->npc_id = timernpc;
		info->time_cycle = loop ? ltime_cycle : 0;
		info->arg = arg;
		info->name		= M2_NEW char[nameCapacity];

		if (info->name)
			strlcpy(info->name, name, nameCapacity);

		return event_create(quest_server_timer_event, info, ltime_cycle);
	}

	LPEVENT quest_create_timer_event(const char* name, uint32_t player_id, double when, uint32_t npc_id, bool loop)
	{
		const int32_t nameCapacity = strlen(name) + 1;

		int32_t ltime_cycle = (int32_t) (rint(PASSES_PER_SEC(when)));

		quest_event_info* info = AllocEventInfo<quest_event_info>();

		info->player_id		= player_id;
		info->npc_id		= npc_id;
		info->name		= M2_NEW char[nameCapacity];

		if (info->name)
			strlcpy(info->name, name, nameCapacity);

		PyLog("QUEST timer name {} cycle {} pc {} npc {} loop? {}", name ? name : "<noname>", ltime_cycle, player_id, npc_id, loop ? 1 : 0);

		info->time_cycle	= loop ? ltime_cycle : 0;
		return event_create(quest_timer_event, info, ltime_cycle);
	}
}
