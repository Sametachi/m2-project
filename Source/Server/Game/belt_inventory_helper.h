#pragma once

#include "char.h"
#include "item.h"

class CBeltInventoryHelper
{
public:
	typedef uint8_t	TGradeUnit;

	static TGradeUnit GetBeltGradeByRefineLevel(int32_t level)
	{
		static TGradeUnit beltGradeByLevelTable[] = 
		{
			0,			// Belt+0
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

		if (level >= _countof(beltGradeByLevelTable))
		{
			SysLog("CBeltInventoryHelper::GetBeltGradeByRefineLevel - Overflow level ({}", level);
			return 0;
		}

		return beltGradeByLevelTable[level];
	}

	// Return which cells are available based on the current belt level
	static const TGradeUnit* GetAvailableRuleTableByGrade()
	{
		/**
				The belt can have a total of +0 ~ +9 levels, and the inventory is activated as it is divided into 7 grades according to the level.
				The available cells according to the belt level are shown in the figure below. Available if current grade >= active grade.
				(However, if the current level is 0, it cannot be used unconditionally, and the number in parentheses is the grade)
			
				2(1)  4(2)  6(4)  8(6)
				5(3)  5(3)  6(4)  8(6)
				7(5)  7(5)  7(5)  8(6)
				9(7)  9(7)  9(7)  9(7)

				The size of the belt inventory is 4x4 (16 spaces).
		*/

		static TGradeUnit availableRuleByGrade[BELT_INVENTORY_SLOT_COUNT] = {
			1, 2, 4, 6,
			3, 3, 4, 6,
			5, 5, 5, 6,
			7, 7, 7, 7
		};

		return availableRuleByGrade;
	}

	static bool IsAvailableCell(uint16_t cell, int32_t beltGrade /*int32_t beltLevel*/)
	{
		// Plan changed again... Ahh...
		//const TGradeUnit beltGrade = GetBeltGradeByRefineLevel(beltLevel);		
		const TGradeUnit* ruleTable = GetAvailableRuleTableByGrade();

		return ruleTable[cell] <= beltGrade;
	}

	/// A function to check if there is at least one item in the PC's belt inventory.
	static bool IsExistItemInBeltInventory(LPCHARACTER pc)
	{
		for (uint16_t i = BELT_INVENTORY_SLOT_START; i < BELT_INVENTORY_SLOT_END; ++i)
		{
			LPITEM beltInventoryItem = pc->GetInventoryItem(i);

			if (NULL != beltInventoryItem)
				return true;
		}

		return false;
	}

	/// A function that checks if the item is of a type that can fit into the belt inventory. (These rules are determined by the planner)
	static bool CanMoveIntoBeltInventory(LPITEM item)
	{
		bool canMove = false;

		if (item->GetType() == ITEM::TYPE_USE)
		{
			switch (item->GetSubType())
			{
				case ITEM::USE_POTION:
				case ITEM::USE_POTION_NODELAY:
				case ITEM::USE_ABILITY_UP:
					canMove = true;
					break;
			}
		}

		return canMove;
	}

};
