#pragma once

//        Icon, Model (droped on ground), Game Data

#include "../eterLib/GrpSubImage.h"
#include "../eterGrnLib/Thing.h"
#include <Core/Tables.hpp>
#include <Core/Constants/Item.hpp>

class CItemData
{
	public:
		CItemData();
		virtual ~CItemData();

		void Clear();
		void SetSummary(const std::string& c_rstSumm);
		void SetDescription(const std::string& c_rstDesc);
		void SetName(const std::string& c_rstName);

		CGraphicThing * GetModelThing();
		CGraphicThing * GetSubModelThing();
		CGraphicThing * GetDropModelThing();
		CGraphicSubImage * GetIconImage();

		uint32_t GetLODModelThingCount();
		BOOL GetLODModelThingPointer(uint32_t dwIndex, CGraphicThing ** ppModelThing);

		uint32_t GetAttachingDataCount();
		BOOL GetCollisionDataPointer(uint32_t dwIndex, const NRaceData::TAttachingData ** c_ppAttachingData);
		BOOL GetAttachingDataPointer(uint32_t dwIndex, const NRaceData::TAttachingData ** c_ppAttachingData);

		/////
		const TItemTable*	GetTable() const;
		uint32_t GetIndex() const;
		const char * GetName() const;
		const char * GetDescription() const;
		const char * GetSummary() const;
		uint8_t GetType() const;
		uint8_t GetSubType() const;
		UINT GetRefine() const;
		const char* GetUseTypeString() const;
		uint32_t GetWeaponType() const;
		uint8_t GetSize() const;
		BOOL IsAntiFlag(uint32_t dwFlag) const;
		BOOL IsFlag(uint32_t dwFlag) const;
		BOOL IsWearableFlag(uint32_t dwFlag) const;
		BOOL HasNextGrade() const;
		uint32_t GetWearFlags() const;
		uint32_t GetIBuyItemPrice() const;
		uint32_t GetISellItemPrice() const;
		BOOL GetLimit(uint8_t byIndex, TItemLimit * pItemLimit) const;
		BOOL GetApply(uint8_t byIndex, TItemApply * pItemApply) const;
		int32_t GetValue(uint8_t byIndex) const;
		int32_t GetSocket(uint8_t byIndex) const;
		int32_t SetSocket(uint8_t byIndex,uint32_t value);
		int32_t GetSocketCount() const;
		uint32_t GetIconNumber() const;

		UINT	GetSpecularPoweru() const;
		float	GetSpecularPowerf() const;

		/////

		BOOL IsEquipment() const;

		/////

		//BOOL LoadItemData(const char * c_szFileName);
		void SetDefaultItemData(const char * c_szIconFileName, const char * c_szModelFileName  = NULL);
		void SetItemTableData(TItemTable * pItemTable);

	protected:
		void __LoadFiles();
		void __SetIconImage(const char * c_szFileName);

	protected:
		std::string m_strModelFileName;
		std::string m_strSubModelFileName;
		std::string m_strDropModelFileName;
		std::string m_strIconFileName;
		std::string m_strDescription;
		std::string m_strSummary;
		std::string m_strName;
		std::vector<std::string> m_strLODModelFileNameVector;

		CGraphicThing * m_pModelThing;
		CGraphicThing * m_pSubModelThing;
		CGraphicThing * m_pDropModelThing;
		CGraphicSubImage * m_pIconImage;
		std::vector<CGraphicThing *> m_pLODModelThingVector;

		NRaceData::TAttachingDataVector m_AttachingDataVector;
		uint32_t		m_dwVnum;
		TItemTable m_ItemTable;

	public:
		static void DestroySystem();

		static CItemData* New();
		static void Delete(CItemData* pkItemData);

		static CDynamicPool<CItemData>		ms_kPool;
};
