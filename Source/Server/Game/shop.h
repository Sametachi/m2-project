#pragma once

enum
{
	SHOP_MAX_DISTANCE = 1000
};

class CGrid;

/* ---------------------------------------------------------------------------------- */
class CShop
{
	public:
		typedef struct shop_item
		{
			uint32_t	vnum;
			int32_t	price;
			uint8_t	count;

			LPITEM	pItem;
			int32_t		itemid;

			shop_item()
			{
				vnum = 0;
				price = 0;
				count = 0;
				itemid = 0;
				pItem = nullptr;
			}
		} SHOP_ITEM;

		CShop();
		~CShop();

		bool	Create(uint32_t dwVnum, uint32_t dwNPCVnum, TShopItemTable* pItemTable);
		void	SetShopItems(TShopItemTable* pItemTable, uint8_t bItemCount);

		virtual void	SetPCShop(LPCHARACTER ch);
		virtual bool	IsPCShop()	{ return m_pPC ? true : false; }

		virtual bool	AddGuest(LPCHARACTER ch,uint32_t owner_vid, bool bOtherEmpire);
		void	RemoveGuest(LPCHARACTER ch);

		virtual int32_t	Buy(LPCHARACTER ch, uint8_t pos);

		void	BroadcastUpdateItem(uint8_t pos);

		int32_t		GetNumberByVnum(uint32_t dwVnum);

		virtual bool	IsSellingItem(uint32_t itemID);

		uint32_t	GetVnum() { return m_dwVnum; }
		uint32_t	GetNPCVnum() { return m_dwNPCVnum; }

	protected:
		void	Broadcast(const void* data, int32_t bytes);

	protected:
		uint32_t				m_dwVnum;
		uint32_t				m_dwNPCVnum;

		CGrid *				m_pGrid;

		typedef TR1_NS::unordered_map<LPCHARACTER, bool> GuestMapType;
		GuestMapType m_map_guest;
		std::vector<SHOP_ITEM>		m_itemVector;

		LPCHARACTER			m_pPC;
};
