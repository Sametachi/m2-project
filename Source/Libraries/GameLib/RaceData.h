#pragma once
#include "GameType.h"
#include "RaceMotionData.h"

#include <EterGrnLib/Thing.h>
#include <EterLib/AttributeData.h>
#include <Core/Race/RaceMotionConstants.hpp>
#include <memory>

class CRaceData
{
public:
    enum
    {
        SMOKE_NUM = 4,
    };

    // Model
    using TGraphicThingMap = robin_hood::unordered_map<uint16_t, CGraphicThing*>;
    using TAttachingBoneNameMap = robin_hood::unordered_map<uint32_t, std::string>;
    using TRegisteredMotionCacheMap = robin_hood::unordered_map<uint32_t, std::string>;

    // Motion
    typedef struct SMotion
    {
        SMotion() = default;
        SMotion(const SMotion&) = delete;
        SMotion(SMotion&& o) : byPercentage(o.byPercentage), pMotion(std::move(o.pMotion)), pMotionData(std::move(o.pMotionData)) {}

        uint8_t byPercentage;
        CGraphicThing* pMotion;
        std::unique_ptr<CRaceMotionData> pMotionData;
    } TMotion;

    typedef std::vector<TMotion> TMotionVector;
    typedef robin_hood::unordered_map<uint16_t, TMotionVector> TMotionVectorMap;

    typedef struct SMotionModeData
    {
        SMotionModeData() = default;
        SMotionModeData(SMotionModeData&& o) : wMotionModeIndex(o.wMotionModeIndex), MotionVectorMap(std::move(o.MotionVectorMap)) {}
        uint16_t wMotionModeIndex;
        TMotionVectorMap MotionVectorMap;
    } TMotionModeData;

    typedef robin_hood::unordered_map<uint16_t, TMotionModeData> TMotionModeDataMap;
    typedef TMotionModeDataMap::iterator TMotionModeDataIterator;

    /////////////////////////////////////////////////////////////////////////////////
    // Combo Data
    typedef std::vector<uint32_t> TComboIndexVector;
    typedef struct SComboAttackData
    {
        TComboIndexVector ComboIndexVector;
    } TComboData;

    typedef robin_hood::unordered_map<uint32_t, uint32_t> TNormalAttackIndexMap;
    typedef robin_hood::unordered_map<COMBO_KEY, TComboData> TComboAttackDataMap;
    typedef TComboAttackDataMap::iterator TComboAttackDataIterator;

    struct SSkin
    {
        SSkin() : m_ePart(0) {}
        int32_t m_ePart;
        std::string m_stSrcFileName;
        std::string m_stDstFileName;
    };

    struct SHair
    {
        std::string m_stModelFileName;
        std::vector<SSkin> m_kVct_kSkin;
    };

    struct SShape
    {
        SShape() : specular(0) {}
        float specular;
        std::string m_stModelFileName;
        std::vector<SSkin> m_kVct_kSkin;
        NRaceData::TAttachingDataVector m_attachingData;
    };

public:
    CRaceData();

    // Codes For Client
    const char* GetBaseModelFileName() const
    {
        return m_strBaseModelFileName.c_str();
    }

    const char* GetAttributeFileName() const
    {
        return m_strAttributeFileName.c_str();
    }

    const char* GetMotionListFileName() const
    {
        return m_strMotionListFileName.c_str();
    }

    CGraphicThing* GetBaseModelThing();
    CAttributeData* GetAttributeDataPtr();

    bool CreateMotionModeIterator(TMotionModeDataIterator& itor);
    bool NextMotionModeIterator(TMotionModeDataIterator& itor);
    void LoadMotions();
    const TMotionModeDataMap& GetMotionModes() const
    {
        return m_pMotionModeDataMap;
    }

    bool GetMotionKey(uint16_t wMotionModeIndex, uint16_t wMotionIndex, uint32_t* pMotionKey);
    bool GetMotionModeDataPointer(uint16_t wMotionMode, TMotionModeData** ppMotionModeData);
    bool GetMotionVectorPointer(uint16_t wMotionMode, uint16_t wMotionIndex, const TMotionVector** c_ppMotionVector);
    bool GetMotionDataPointer(uint16_t wMotionMode, uint16_t wMotionIndex, uint16_t wMotionSubIndex, CRaceMotionData** ppMotionData);
    bool GetMotionDataPointer(uint32_t dwMotionKey, CRaceMotionData** ppMotionData);
    bool GetAttachingBoneName(uint32_t dwPartIndex, const char** c_pszBoneName);

    const NRaceData::TAttachingDataVector& GetAttachingData() const
    {
        return m_AttachingDataVector;
    }
    void SetRace(uint32_t dwRaceIndex)
    {
        m_dwRaceIndex = dwRaceIndex;
    }
    bool IsTree() const
    {
        return !m_strTreeFileName.empty();
    }
    const char* GetTreeFileName() const
    {
        return m_strTreeFileName.c_str();
    }
    uint32_t GetRaceIndex() const
    {
        return m_dwRaceIndex;
    }

    ///////////////////////////////////////////////////////////////////
    // Setup by Script
    bool LoadRaceData(const char* c_szFileName);

    CGraphicThing* RegisterMotionData(uint16_t wMotionMode, uint16_t wMotionIndex, const std::string& c_szFileName, uint8_t byPercentage = 100);

    // Setup by Python
    void RegisterAttachingBoneName(uint32_t dwPartIndex, const char* c_szBoneName);

    void RegisterMotionMode(uint16_t wMotionModeIndex);
    void SetMotionModeParent(uint16_t wParentMotionModeIndex, uint16_t wMotionModeIndex);

    CGraphicThing* RegisterMotion(std::unique_ptr<CRaceMotionData> pkMotionData, uint16_t wMotionModeIndex, uint16_t wMotionIndex, const char* c_szFileName, uint8_t byPercentage = 100);

    bool SetMotionRandomWeight(uint16_t wMotionModeIndex, uint16_t wMotionIndex, uint16_t wMotionSubIndex, uint8_t byPercentage);

    void RegisterNormalAttack(uint16_t wMotionModeIndex, uint16_t wMotionIndex);
    bool GetNormalAttackIndex(uint16_t wMotionModeIndex, uint16_t* pwMotionIndex);

    void ReserveComboAttack(uint16_t wMotionModeIndex, uint16_t wComboType, uint32_t dwComboCount);
    void RegisterComboAttack(uint16_t wMotionModeIndex, uint16_t wComboType, uint32_t dwComboIndex, uint16_t wMotionIndex);
    bool GetComboDataPointer(uint16_t wMotionModeIndex, uint16_t wComboType, TComboData** ppComboData);

    void SetShapeModel(uint32_t eShape, const char* c_szModelFileName);
    void AppendShapeSkin(uint32_t eShape, uint32_t ePart, const char* c_szSrcFileName, const char* c_szDstFileName);

    void SetHairSkin(uint32_t eHair, uint32_t ePart, const char* c_szModelFileName, const char* c_szSrcFileName, const char* c_szDstFileName);
    void SetHairModel(uint32_t eHair, const char* c_szModelFileName);

    uint32_t GetSmokeEffectID(uint32_t eSmoke);
    const std::string& GetSmokeBone() const
    {
        return m_strSmokeBoneName;
    }

    SHair* FindHair(uint32_t eHair);
    SShape* FindShape(uint32_t eShape);

protected:
    bool GetMotionVectorPointer(uint16_t wMotionMode, uint16_t wMotionIndex, TMotionVector** ppMotionVector);
    bool m_motionsLoaded = false;
    uint32_t m_dwRaceIndex;
    uint32_t m_adwSmokeEffectID[SMOKE_NUM];

    CGraphicThing* m_pBaseModelThing;

    std::string m_strBaseModelFileName;
    std::string m_strTreeFileName;
    std::string m_strAttributeFileName;
    std::string m_strMotionListFileName;
    std::string m_strSmokeBoneName;

    TMotionModeDataMap m_pMotionModeDataMap;
    TAttachingBoneNameMap m_AttachingBoneNameMap;
    TComboAttackDataMap m_ComboAttackDataMap;
    TNormalAttackIndexMap m_NormalAttackIndexMap;

    std::unordered_map<uint32_t, SHair> m_kMap_dwHairKey_kHair;
    std::unordered_map<uint32_t, SShape> m_kMap_dwShapeKey_kShape;

    NRaceData::TAttachingDataVector m_AttachingDataVector;
    TRegisteredMotionCacheMap m_registeredMotionCache;
};