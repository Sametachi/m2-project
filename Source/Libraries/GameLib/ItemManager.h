#pragma once
#include "ItemData.h"

class CItemManager : public Singleton<CItemManager>
{
public:
	enum EItemDescCol
	{
		ITEMDESC_COL_VNUM,
		ITEMDESC_COL_NAME,
		ITEMDESC_COL_DESC,
		ITEMDESC_COL_SUMM,
		ITEMDESC_COL_NUM,
	};

	using TItemMap = std::map<uint32_t, CItemData*>;
	using TItemNameMap = std::map<std::string, CItemData*>;

	CItemManager();
	virtual ~CItemManager();

	void Destroy();

	bool SelectItemData(uint32_t dwIndex);
	CItemData* GetSelectedItemDataPointer();

	bool GetItemDataPointer(uint32_t dwItemID, CItemData ** ppItemData);
	bool GetItemDataPointer(const char* c_szInput, CItemData** ppItemData);

	bool LoadItemDesc(const char* c_szFileName);
	bool LoadItemList(const char* c_szFileName);
	bool LoadItemTable(const char* c_szFileName);

	CItemData* GetProto(int32_t dwItemID);
	CItemData* MakeItemData(uint32_t dwIndex);

protected:
	TItemMap m_ItemMap;
	std::vector<CItemData*> m_vec_ItemRange;
	CItemData* m_pSelectedItemData;
};
