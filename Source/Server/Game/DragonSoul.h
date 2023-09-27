#pragma once

#include <Common/length.h>

class CHARACTER;
class CItem;

class DragonSoulTable;

class DSManager : public Singleton<DSManager>
{
public:
	DSManager();
	~DSManager();
	bool	ReadDragonSoulTableFile(const char* c_pszFileName);

	void	GetDragonSoulInfo(uint32_t dwVnum, OUT uint8_t& bType, OUT uint8_t& bGrade, OUT uint8_t& bStep, OUT uint8_t& bRefine) const;
	// fixme : titempos
	uint16_t	GetBasePosition(const LPITEM pItem) const;
	bool	IsValidCellForThisItem(const LPITEM pItem, const TItemPos& Cell) const;
	int32_t		GetDuration(const LPITEM pItem) const;
	
	// A function that receives a dragon soul stone and extracts a specific dragon spirit
	bool	ExtractDragonHeart(LPCHARACTER ch, LPITEM pItem, LPITEM pExtractor = nullptr);

	// Determines success or failure when removing a specific dragon soul stone (pItem) from the equipment window,
	// A function that gives a by-product in case of failure. (The by-product is defined in dragon_soul_table.txt)
	// If an invalid value is entered in DestCell, the Dragon Soul Stone is automatically added to the empty space upon success.
	// In case of failure, the dragon soul stone (pItem) is deleted.
	// If there is an extraction item, the extraction success probability increases by pExtractor->GetValue(0)%.
	// Byproducts are always added automatically.
	bool	PullOut(LPCHARACTER ch, TItemPos DestCell, IN OUT LPITEM& pItem, LPITEM pExtractor = nullptr);

	// Dragon Soul Stone Upgrade Function
	bool	DoRefineGrade(LPCHARACTER ch, TItemPos (&aItemPoses)[DRAGON_SOUL_REFINE_GRID_SIZE]);
	bool	DoRefineStep(LPCHARACTER ch, TItemPos (&aItemPoses)[DRAGON_SOUL_REFINE_GRID_SIZE]);
	bool	DoRefineStrength(LPCHARACTER ch, TItemPos (&aItemPoses)[DRAGON_SOUL_REFINE_GRID_SIZE]);
	
	bool	DragonSoulItemInitialize(LPITEM pItem);

	bool	IsTimeLeftDragonSoul(LPITEM pItem) const;
	int32_t		LeftTime(LPITEM pItem) const;
	bool	ActivateDragonSoul(LPITEM pItem);
	bool	DeactivateDragonSoul(LPITEM pItem, bool bSkipRefreshOwnerActiveState = false);
	bool	IsActiveDragonSoul(LPITEM pItem) const;
private:
	void	SendRefineResultPacket(LPCHARACTER ch, uint8_t bSubHeader, const TItemPos& pos);

	// A function that looks at the character's dragon soul stone deck and turns off the character's dragon soul stone activation state if there is no active dragon soul stone.
	void	RefreshDragonSoulState(LPCHARACTER ch);

	uint32_t	MakeDragonSoulVnum(uint8_t bType, uint8_t grade, uint8_t step, uint8_t refine);
	bool	PutAttributes(LPITEM pDS);
	bool	RefreshItemAttributes(LPITEM pItem);

	DragonSoulTable*	m_pTable;
};
