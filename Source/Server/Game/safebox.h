#pragma once

class CHARACTER;
class CItem;
class CGrid;

class CSafebox
{
	public:
		CSafebox(LPCHARACTER pChrOwner, int32_t iSize, uint32_t dwGold);
		~CSafebox();

		bool		Add(uint32_t dwPos, LPITEM pItem);
		LPITEM		Get(uint32_t dwPos);
		LPITEM		Remove(uint32_t dwPos);
		void		ChangeSize(int32_t iSize);

		bool		MoveItem(uint8_t bCell, uint8_t bDestCell, uint8_t count);
		LPITEM		GetItem(uint8_t bCell);

		void		Save();

		bool		IsEmpty(uint32_t dwPos, uint8_t bSize);
		bool		IsValidPosition(uint32_t dwPos);

		void		SetWindowMode(uint8_t bWindowMode);

	protected:
		void		__Destroy();

		LPCHARACTER	m_pChrOwner;
		LPITEM		m_pItems[SAFEBOX_MAX_NUM];
		CGrid *		m_pGrid;
		int32_t		m_iSize;
		int32_t		m_lGold;

		uint8_t		m_bWindowMode;
};
