#pragma once
#include <Core/Constants/Shop.hpp>
/*
 *	상점 처리
 *
 *	2003-01-16 anoa	일차 완료
 *	2003-12-26 levites 수정
 *
 *	2012-10-29 rtsummit 새로운 화폐 출현 및 tab 기능 추가로 인한 shop 확장.
 *
 */

class CPythonShop : public Singleton<CPythonShop>
{
	public:
		CPythonShop(void);
		virtual ~CPythonShop(void);

		void Clear();
		
		void SetItemData(uint32_t dwIndex, const TShopItemData & c_rShopItemData);
		BOOL GetItemData(uint32_t dwIndex, const TShopItemData ** c_ppItemData);

		void SetItemData(uint8_t tabIdx, uint32_t dwSlotPos, const TShopItemData & c_rShopItemData);
		BOOL GetItemData(uint8_t tabIdx, uint32_t dwSlotPos, const TShopItemData ** c_ppItemData);
		
		void SetTabCount(uint8_t bTabCount) { m_bTabCount = bTabCount; }
		uint8_t GetTabCount() { return m_bTabCount; }

		void SetTabCoinType(uint8_t tabIdx, uint8_t coinType);
		uint8_t GetTabCoinType(uint8_t tabIdx);

		void SetTabName(uint8_t tabIdx, const char* name);
		const char* GetTabName(uint8_t tabIdx);


		//BOOL GetSlotItemID(uint32_t dwSlotPos, uint32_t* pdwItemID);

		void Open(BOOL isPrivateShop, BOOL isMainPrivateShop);
		void Close();
		BOOL IsOpen();
		BOOL IsPrivateShop();
		BOOL IsMainPlayerPrivateShop();

		void ClearPrivateShopStock();
		void AddPrivateShopItemStock(TItemPos ItemPos, uint8_t byDisplayPos, uint32_t dwPrice);
		void DelPrivateShopItemStock(TItemPos ItemPos);
		int32_t GetPrivateShopItemPrice(TItemPos ItemPos);
		void BuildPrivateShop(const char * c_szName);

	protected:
		BOOL	CheckSlotIndex(uint32_t dwIndex);

	protected:
		BOOL				m_isShoping;
		BOOL				m_isPrivateShop;
		BOOL				m_isMainPlayerPrivateShop;

		struct ShopTab
		{
			ShopTab()
			{
				coinType = SHOP_COIN_TYPE_GOLD;
			}
			uint8_t				coinType;
			std::string			name;
			TShopItemData		items[SHOP_HOST_ITEM_MAX_NUM];
		};
		
		uint8_t m_bTabCount;
		ShopTab m_aShoptabs[SHOP_TAB_COUNT_MAX];

		typedef std::map<TItemPos, TShopItemTable> TPrivateShopItemStock;
		TPrivateShopItemStock	m_PrivateShopItemStock;
};
