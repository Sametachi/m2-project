#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "item.h"
#include "item_addon.h"

CItemAddonManager::CItemAddonManager()
{
}

CItemAddonManager::~CItemAddonManager()
{
}

void CItemAddonManager::ApplyAddonTo(int32_t iAddonType, LPITEM pItem)
{
	if (!pItem)
	{
		SysLog("ITEM pointer null");
		return;
	}

	// TODO Lets hard-coding apply only to normal skill level changes.
	
	int32_t iSkillBonus = MINMAX(-30, (int32_t) (gauss_random(0, 5) + 0.5f), 30);
	int32_t iNormalHitBonus = 0;
	if (abs(iSkillBonus) <= 20)
		iNormalHitBonus = -2 * iSkillBonus + abs(number(-8, 8) + number(-8, 8)) + number(1, 4);
	else
		iNormalHitBonus = -2 * iSkillBonus + number(1, 5);

	pItem->RemoveAttributeType(ITEM::APPLY_SKILL_DAMAGE_BONUS);
	pItem->RemoveAttributeType(ITEM::APPLY_NORMAL_HIT_DAMAGE_BONUS);
	pItem->AddAttribute(ITEM::APPLY_NORMAL_HIT_DAMAGE_BONUS, iNormalHitBonus);
	pItem->AddAttribute(ITEM::APPLY_SKILL_DAMAGE_BONUS, iSkillBonus);
}
