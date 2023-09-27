#pragma once

#include "../../../Libraries/gamelib/Area.h"
#define CWE_AREA_ACCESSOR_MISSING_REFRESH

class CAreaAccessor : public CArea
{
	public:
		struct SSelectObject
		{
			uint32_t dwIndex;
			uint32_t dwCRC32;
		};
		typedef std::list<SSelectObject> TSelectObjectList;

		struct TCollisionDataCounter
		{
			std::string stName;
			uint32_t		count;
		};

		static void PrintCounter(const TCollisionDataCounter & r)
		{
			fprintf(ms_LogFile, "%2d : %s\n", r.count, r.stName.c_str());
		}

	public:
		CAreaAccessor();
		virtual ~CAreaAccessor();

		void Clear();

		bool Save(const std::string & c_rstrMapName);
		bool SaveCollisionData(const char * c_szLoadingAreaFileName, FILE * SavingFile);

		void RefreshArea();
		void UpdateObject(uint32_t dwIndex, const TObjectData * c_pObjectData); // Height, Rotation

		void AddObject(const TObjectData * c_pObjectData);
		bool GetObjectPointer(uint32_t dwIndex, TObjectData ** ppObjectData);

		int32_t GetPickedObjectIndex();
		BOOL IsSelectedObject(uint32_t dwIndex);
		void SelectObject(uint32_t dwIndex);

		int32_t GetSelectedObjectCount();
		const CArea::TObjectData* GetLastSelectedObjectData() const;

		BOOL Picking();
		BOOL PickObject();

		void RenderSelectedObject();
		void CancelSelect();
		BOOL SelectObject(float fxStart, float fyStart, float fxEnd, float fyEnd);
		void DeleteSelectedObject();
		void MoveSelectedObject(float fx, float fy);
		void MoveSelectedObjectHeight(float fHeight);
		void AddSelectedAmbienceScale(int32_t iAddScale);
		void AddSelectedAmbienceMaxVolumeAreaPercentage(float fPercentage);
		void AddSelectedObjectRotation(float fYaw, float fPitch, float fRoll);
		void SetSelectedObjectPortalNumber(int32_t iID);
		void DelSelectedObjectPortalNumber(int32_t iID);
		void CollectPortalNumber(std::set<int32_t> * pkSet_iPortalNumber);
		BOOL IsSelected();

		static void OpenCollisionDataCountMapLog();
		static void CloseCollisionDataCountMapLog();

		//////////////////////////////////////////////////////////////////////////
		// Shadow Map
		void RenderToShadowMap();

		void ReloadBuildingTexture();
		void __RefreshSelectedInfo();

	protected:
		bool CheckInstanceIndex(uint32_t dwIndex);
		bool GetInstancePointer(uint32_t dwIndex, TObjectInstance ** ppObjectInstance);

		bool __SaveObjects(const char * c_szFileName);
		bool __SaveAmbiences(const char * c_szOtherPathName, const char * c_szFileName);

		void __ClickObject(uint32_t dwIndex);
		void __DeselectObject(uint32_t dwIndex);
		void __RefreshObjectPosition(float fx, float fy, float fz);

	protected:
		TSelectObjectList							m_SelectObjectList;

	private:
		static FILE *								ms_LogFile;
		static std::set<std::string>				ms_NonAttributeObjectSet;
		static std::vector<TCollisionDataCounter>	ms_CollisionDataCountVec;
};
