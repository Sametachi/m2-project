#pragma once

class CPythonSafeBox : public Singleton<CPythonSafeBox>
{
	public:
		enum
		{
			SAFEBOX_SLOT_X_COUNT = 5,
			SAFEBOX_SLOT_Y_COUNT = 9,
			SAFEBOX_PAGE_SIZE = SAFEBOX_SLOT_X_COUNT * SAFEBOX_SLOT_Y_COUNT,
		};
		typedef std::vector<TItemData> TItemInstanceVector;

	public:
		CPythonSafeBox();
		virtual ~CPythonSafeBox();

		void OpenSafeBox(int32_t iSize);
		void SetItemData(uint32_t dwSlotIndex, const TItemData & rItemData);
		void DelItemData(uint32_t dwSlotIndex);

		void SetMoney(uint32_t dwMoney);
		uint32_t GetMoney();
		
		BOOL GetSlotItemID(uint32_t dwSlotIndex, uint32_t* pdwItemID);

		int32_t GetCurrentSafeBoxSize();
		BOOL GetItemDataPtr(uint32_t dwSlotIndex, TItemData ** ppInstance);

	protected:
		TItemInstanceVector m_ItemInstanceVector;
		uint32_t m_dwMoney;
};