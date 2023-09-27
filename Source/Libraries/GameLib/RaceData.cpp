#include "StdAfx.h"
#include "RaceData.h"
#include "RaceMotionData.h"

#include <EterLib/ResourceManager.h>
#include <EterLib/AttributeInstance.h>
#include <EterBase/Utils.h>
#include <EterGrnLib/ThingInstance.h>
#include <Core/Race/RaceMotionConstants.hpp>
#include <Core/Race/MotionProto.hpp>

uint32_t CRaceData::GetSmokeEffectID(uint32_t eSmoke)
{
    if (eSmoke >= SMOKE_NUM)
    {
        SysLog("CRaceData::GetSmokeEffectID(eSmoke={0})", eSmoke);
        return 0;
    }

    return m_adwSmokeEffectID[eSmoke];
}

CRaceData::SHair* CRaceData::FindHair(uint32_t eHair)
{
    auto f = m_kMap_dwHairKey_kHair.find(eHair);
    if (m_kMap_dwHairKey_kHair.end() == f)
    {
        SysLog("Hair number {0} is not exist.", eHair);
        return nullptr;
    }

    return &f->second;
}

void CRaceData::SetHairSkin(uint32_t eHair, uint32_t ePart, const char* c_szModelFileName, const char* c_szSrcFileName, const char* c_szDstFileName)
{
    SSkin kSkin;
    kSkin.m_ePart = ePart;
    kSkin.m_stSrcFileName = c_szSrcFileName;
    kSkin.m_stDstFileName = c_szDstFileName;

    CFileNameHelper::ChangeDosPath(kSkin.m_stSrcFileName);
    m_kMap_dwHairKey_kHair[eHair].m_kVct_kSkin.emplace_back(kSkin);
    m_kMap_dwHairKey_kHair[eHair].m_stModelFileName = c_szModelFileName;
}

void CRaceData::SetHairModel(uint32_t eHair, const char* c_szModelFileName)
{
    m_kMap_dwHairKey_kHair[eHair].m_stModelFileName = c_szModelFileName;
}

CRaceData::SShape* CRaceData::FindShape(uint32_t eShape)
{
    auto f = m_kMap_dwShapeKey_kShape.find(eShape);
    if (m_kMap_dwShapeKey_kShape.end() == f)
        return NULL;

    return &f->second;
}

void CRaceData::SetShapeModel(uint32_t eShape, const char* c_szModelFileName)
{
    m_kMap_dwShapeKey_kShape[eShape].m_stModelFileName = c_szModelFileName;
}

void CRaceData::AppendShapeSkin(uint32_t eShape, uint32_t ePart, const char* c_szSrcFileName, const char* c_szDstFileName)
{
    SSkin kSkin;
    kSkin.m_ePart = ePart;
    kSkin.m_stSrcFileName = c_szSrcFileName;
    kSkin.m_stDstFileName = c_szDstFileName;

    CFileNameHelper::ChangeDosPath(kSkin.m_stSrcFileName);
    m_kMap_dwShapeKey_kShape[eShape].m_kVct_kSkin.emplace_back(kSkin);
}

bool CRaceData::GetMotionKey(uint16_t wMotionModeIndex, uint16_t wMotionIndex, uint32_t* pMotionKey)
{
    TMotionModeData* pMotionModeData;
    if (!GetMotionModeDataPointer(wMotionModeIndex, &pMotionModeData))
        return false;

    if (pMotionModeData->MotionVectorMap.end() == pMotionModeData->MotionVectorMap.find(wMotionIndex))
    {
        uint16_t wGeneralMode = MOTION_MODE_GENERAL;

        switch (wMotionModeIndex)
        {
        case MOTION_MODE_HORSE_ONEHAND_SWORD:
        case MOTION_MODE_HORSE_TWOHAND_SWORD:
        case MOTION_MODE_HORSE_DUALHAND_SWORD:
        case MOTION_MODE_HORSE_FAN:
        case MOTION_MODE_HORSE_BELL:
        case MOTION_MODE_HORSE_BOW:
            wGeneralMode = MOTION_MODE_HORSE;
            break;

        default:
            wGeneralMode = MOTION_MODE_GENERAL;
            break;
        }

        TMotionModeData* pMotionModeGeneralData;
        if (!GetMotionModeDataPointer(wGeneralMode, &pMotionModeGeneralData))
            return false;

        if (pMotionModeGeneralData->MotionVectorMap.end() == pMotionModeGeneralData->MotionVectorMap.find(wMotionIndex))
            return false;

        *pMotionKey = MakeMotionKey(static_cast<uint8_t>(wGeneralMode), wMotionIndex);
    }
    else
    {
        *pMotionKey = MakeMotionKey(static_cast<uint8_t>(wMotionModeIndex), wMotionIndex);
    }

    return true;
}

bool CRaceData::GetMotionModeDataPointer(uint16_t wMotionMode, TMotionModeData** ppMotionModeData)
{
    auto itor = m_pMotionModeDataMap.find(wMotionMode);
    if (itor == m_pMotionModeDataMap.end())
        return false;

    *ppMotionModeData = &itor->second;
    return true;
}

bool CRaceData::GetMotionVectorPointer(uint16_t wMotionMode, uint16_t wMotionIndex, TMotionVector** ppMotionVector)
{
    TMotionModeData* pMotionModeData;
    if (!GetMotionModeDataPointer(wMotionMode, &pMotionModeData))
        return false;

    const auto itor = pMotionModeData->MotionVectorMap.find(wMotionIndex);
    if (pMotionModeData->MotionVectorMap.end() == itor)
        return false;

    *ppMotionVector = &itor->second;
    return true;
}

bool CRaceData::GetMotionDataPointer(uint16_t wMotionMode, uint16_t wMotionIndex, uint16_t wMotionSubIndex, CRaceMotionData** c_ppMotionData)
{
    const TMotionVector* c_pMotionVector;
    if (!GetMotionVectorPointer(wMotionMode, wMotionIndex, &c_pMotionVector))
        return false;

    if (wMotionSubIndex >= c_pMotionVector->size())
        return false;

    const TMotion& c_rMotion = c_pMotionVector->at(wMotionSubIndex);
    if (!c_rMotion.pMotionData)
        return false;

    *c_ppMotionData = c_rMotion.pMotionData.get();
    return true;
}

bool CRaceData::GetMotionDataPointer(uint32_t dwMotionKey, CRaceMotionData** c_ppMotionData)
{
    MotionId id = MakeMotionId(dwMotionKey);
    return GetMotionDataPointer(id.mode, id.index, id.subIndex, c_ppMotionData);
}

bool CRaceData::GetMotionVectorPointer(uint16_t wMotionMode, uint16_t wMotionIndex, const TMotionVector** c_ppMotionVector)
{
    TMotionVector* pMotionVector;
    if (!GetMotionVectorPointer(wMotionMode, wMotionIndex, &pMotionVector))
        return false;

    *c_ppMotionVector = pMotionVector;
    return true;
}

void CRaceData::RegisterAttachingBoneName(uint32_t dwPartIndex, const char* c_szBoneName)
{
    m_AttachingBoneNameMap.emplace(dwPartIndex, c_szBoneName);
}

void CRaceData::RegisterMotionMode(uint16_t wMotionModeIndex)
{
    TMotionModeData motionModeData;
    motionModeData.wMotionModeIndex = wMotionModeIndex;
    m_pMotionModeDataMap.emplace(wMotionModeIndex, std::move(motionModeData));
}

CGraphicThing* CRaceData::RegisterMotion(std::unique_ptr<CRaceMotionData> pkMotionData, uint16_t wMotionModeIndex, uint16_t wMotionIndex, const char* c_szFileName, uint8_t byPercentage)
{
    auto pMotionThing = CResourceManager::GetInstance()->LoadResource<CGraphicThing>(c_szFileName);
    if (!pMotionThing)
    {
        SysLog("Could not load motion mode {0} index {1} file {2}", wMotionModeIndex, wMotionIndex, c_szFileName);
        return nullptr;
    }

    TMotionModeData* pMotionModeData;
    if (!GetMotionModeDataPointer(wMotionModeIndex, &pMotionModeData))
    {
        AssertLog("Failed getting motion mode data!");
        return NULL;
    }

    TMotion kMotion;
    kMotion.byPercentage = byPercentage;
    kMotion.pMotion = pMotionThing;
    kMotion.pMotionData = std::move(pkMotionData);
    pMotionModeData->MotionVectorMap[wMotionIndex].emplace_back(std::move(kMotion));

    MotionId motionId = MakeMotionId(static_cast<uint8_t>(wMotionModeIndex), wMotionIndex);
    m_registeredMotionCache.emplace(motionId.key, c_szFileName);

    return pMotionThing;
}

CGraphicThing* CRaceData::RegisterMotionData(uint16_t wMotionMode, uint16_t wMotionIndex, const std::string& c_szFileName, uint8_t byPercentage)
{
    auto id = MakeMotionId(static_cast<uint8_t>(wMotionMode), wMotionIndex);
    auto it = m_registeredMotionCache.find(id.key);
    if (it != m_registeredMotionCache.end())
    {
        CGraphicThing* pMotionThing = CResourceManager::GetInstance()->LoadResource<CGraphicThing>(c_szFileName);
        return pMotionThing;
    }

    std::unique_ptr<CRaceMotionData> pRaceMotionData = std::make_unique<CRaceMotionData>();
    if (!pRaceMotionData->LoadMotionData(c_szFileName.c_str()))
    {
        SysLog("Failed to load motion-data {0}", c_szFileName);
        return nullptr;
    }

    pRaceMotionData->SetName(wMotionIndex);
    const char* motionFileName = pRaceMotionData->GetMotionFileName();
    return RegisterMotion(std::move(pRaceMotionData), wMotionMode, wMotionIndex, motionFileName, byPercentage);
}

bool CRaceData::SetMotionRandomWeight(uint16_t wMotionModeIndex, uint16_t wMotionIndex, uint16_t wMotionSubIndex, uint8_t byPercentage)
{
    TMotionModeData* pMotionModeData;
    if (!GetMotionModeDataPointer(wMotionModeIndex, &pMotionModeData))
    {
        return false;
    }

    auto itor = pMotionModeData->MotionVectorMap.find(wMotionIndex);
    if (pMotionModeData->MotionVectorMap.end() != itor)
    {
        TMotionVector& rMotionVector = itor->second;
        if (wMotionSubIndex < rMotionVector.size())
            rMotionVector[wMotionSubIndex].byPercentage = byPercentage;
        else
            return false;
    }
    else
    {
        return false;
    }

    return true;
}

void CRaceData::RegisterNormalAttack(uint16_t wMotionModeIndex, uint16_t wMotionIndex)
{
    m_NormalAttackIndexMap.emplace(wMotionModeIndex, wMotionIndex);
}

bool CRaceData::GetNormalAttackIndex(uint16_t wMotionModeIndex, uint16_t* pwMotionIndex)
{
    auto itor = m_NormalAttackIndexMap.find(wMotionModeIndex);
    if (m_NormalAttackIndexMap.end() == itor)
        return false;

    *pwMotionIndex = static_cast<uint16_t>(itor->second);
    return true;
}

void CRaceData::ReserveComboAttack(uint16_t wMotionModeIndex, uint16_t wComboType, uint32_t dwComboCount)
{
    TComboData ComboData;
    ComboData.ComboIndexVector.clear();
    ComboData.ComboIndexVector.resize(dwComboCount);
    m_ComboAttackDataMap.emplace(MAKE_COMBO_KEY(wMotionModeIndex, wComboType), ComboData);
}

void CRaceData::RegisterComboAttack(uint16_t wMotionModeIndex, uint16_t wComboType, uint32_t dwComboIndex, uint16_t wMotionIndex)
{
    auto itor = m_ComboAttackDataMap.find(MAKE_COMBO_KEY(wMotionModeIndex, wComboType));
    if (m_ComboAttackDataMap.end() == itor)
        return;

    TComboIndexVector& rComboIndexVector = itor->second.ComboIndexVector;
    if (dwComboIndex >= rComboIndexVector.size())
    {
        AssertLog("CRaceData::RegisterCombo - Strange combo index!");
        return;
    }

    rComboIndexVector[dwComboIndex] = wMotionIndex;
}

bool CRaceData::GetComboDataPointer(uint16_t wMotionModeIndex, uint16_t wComboType, TComboData** ppComboData)
{
    auto itor = m_ComboAttackDataMap.find(MAKE_COMBO_KEY(wMotionModeIndex, wComboType));
    if (m_ComboAttackDataMap.end() == itor)
        return false;

    *ppComboData = &itor->second;
    return true;
}

CGraphicThing* CRaceData::GetBaseModelThing()
{
    if (!m_pBaseModelThing)
        m_pBaseModelThing = CResourceManager::GetInstance()->LoadResource<CGraphicThing>(m_strBaseModelFileName);

    return m_pBaseModelThing;
}

CAttributeData* CRaceData::GetAttributeDataPtr()
{
    if (m_strAttributeFileName.empty())
        return nullptr;

    return CResourceManager::GetInstance()->LoadResource<CAttributeData>(m_strAttributeFileName);
}

bool CRaceData::CreateMotionModeIterator(TMotionModeDataIterator& itor)
{
    if (m_pMotionModeDataMap.empty())
        return false;

    itor = m_pMotionModeDataMap.begin();

    return true;
}

bool CRaceData::NextMotionModeIterator(TMotionModeDataIterator& itor)
{
    ++itor;
    return m_pMotionModeDataMap.end() != itor;
}

void CRaceData::LoadMotions()
{
    if (!m_motionsLoaded)
    {
        for (const auto& p : GetMotionModes())
        {
            for (const auto& p2 : p.second.MotionVectorMap)
            {
                uint32_t subIndex = 0;
                for (const auto& motion : p2.second)
                {
                    const uint32_t key = MakeMotionKey(static_cast<uint8_t>(p.first), p2.first, static_cast<uint8_t>(subIndex++));
                    CGraphicThingInstance::RegisterMotionThing(m_dwRaceIndex, key, motion.pMotion);
                }
            }
        }
        m_motionsLoaded = true;
    }
}

bool CRaceData::GetAttachingBoneName(uint32_t dwPartIndex, const char** c_pszBoneName)
{
    TAttachingBoneNameMap::iterator itor = m_AttachingBoneNameMap.find(dwPartIndex);
    if (itor == m_AttachingBoneNameMap.end())
        return false;

    const std::string& c_rstrBoneName = itor->second;

    *c_pszBoneName = c_rstrBoneName.c_str();
    return true;
}

CRaceData::CRaceData()
    : m_strMotionListFileName("motlist.txt")
    , m_pBaseModelThing(nullptr)
    , m_dwRaceIndex(0)
{
    std::memset(m_adwSmokeEffectID, 0, sizeof(m_adwSmokeEffectID));
}

bool CRaceData::LoadRaceData(const char* c_szFileName)
{
    CTextFileLoader TextFileLoader;
    if (!TextFileLoader.Load(c_szFileName))
        return false;

    TextFileLoader.SetTop();

    TextFileLoader.GetTokenString("basemodelfilename", &m_strBaseModelFileName);
    TextFileLoader.GetTokenString("treefilename", &m_strTreeFileName);
    TextFileLoader.GetTokenString("attributefilename", &m_strAttributeFileName);
    TextFileLoader.GetTokenString("smokebonename", &m_strSmokeBoneName);
    TextFileLoader.GetTokenString("motionlistfilename", &m_strMotionListFileName);

    if (!m_strTreeFileName.empty())
        CFileNameHelper::StringPath(m_strTreeFileName);

    CTokenVector* pSmokeTokenVector;
    if (TextFileLoader.GetTokenVector("smokefilename", &pSmokeTokenVector))
    {
        if (pSmokeTokenVector->size() % 2 != 0)
        {
            SysLog("SmokeFileName ArgCount[{0}]%%2==0", pSmokeTokenVector->size());
            return false;
        }

        uint32_t uLineCount = pSmokeTokenVector->size() / 2;

        for (uint32_t uLine = 0; uLine < uLineCount; ++uLine)
        {
            __int64 eSmoke = atoi(pSmokeTokenVector->at(static_cast<std::vector<std::string, std::allocator<std::string>>::size_type>(uLine) * 2 + 0).c_str());
            if (eSmoke < 0 || eSmoke >= SMOKE_NUM)
            {
                SysLog("SmokeFileName SmokeNum[{0}] OUT OF RANGE", eSmoke);
                return false;
            }

            const std::string& c_rstrEffectFileName = pSmokeTokenVector->at(static_cast<std::vector<std::string, std::allocator<std::string>>::size_type>(uLine) * 2 + 1);

            uint32_t& rdwCRCEft = m_adwSmokeEffectID[eSmoke];
            if (!CEffectManager::GetInstance()->RegisterEffect(c_rstrEffectFileName.c_str(), &rdwCRCEft))
            {
                SysLog("CRaceData::RegisterEffect({0}) ERROR", c_rstrEffectFileName.c_str());
                rdwCRCEft = 0;
                return false;
            }
        }
    }

    if (TextFileLoader.SetChildNode("shapedata"))
    {
        std::string strPathName;
        uint32_t dwShapeDataCount = 0;
        if (TextFileLoader.GetTokenString("pathname", &strPathName) &&
            TextFileLoader.GetTokenDoubleWord("shapedatacount", &dwShapeDataCount))
        {
            for (uint32_t i = 0; i < dwShapeDataCount; ++i)
            {
                if (!TextFileLoader.SetChildNode("shapedata", i))
                    continue;

                /////////////////////////
                TextFileLoader.GetTokenString("specialpath", &strPathName);
                /////////////////////////

                uint32_t dwShapeIndex = 0;
                if (!TextFileLoader.GetTokenDoubleWord("shapeindex", &dwShapeIndex))
                    continue;

                // LOCAL_PATH_SUPPORT
                std::string strModel;
                if (TextFileLoader.GetTokenString("model", &strModel))
                    SetShapeModel(dwShapeIndex, (strPathName + strModel).c_str());
                else
                {
                    if (!TextFileLoader.GetTokenString("local_model", &strModel))
                        continue;

                    SetShapeModel(dwShapeIndex, strModel.c_str());
                }
                // END_OF_LOCAL_PATH_SUPPORT

                std::string strSourceSkin;
                std::string strTargetSkin;
                static char __szSkin1[11 + 1];
                static char __szSkin2[11 + 1];

                // LOCAL_PATH_SUPPORT
                if (TextFileLoader.GetTokenString("local_sourceskin", &strSourceSkin) && TextFileLoader.GetTokenString("local_targetskin", &strTargetSkin))
                {
                    AppendShapeSkin(dwShapeIndex, 0, strSourceSkin.c_str(), strTargetSkin.c_str());
                }
                // END_OF_LOCAL_PATH_SUPPORT

                if (TextFileLoader.GetTokenString("sourceskin", &strSourceSkin) && TextFileLoader.GetTokenString("targetskin", &strTargetSkin))
                {
                    AppendShapeSkin(dwShapeIndex, 0, (strPathName + strSourceSkin).c_str(), (strPathName + strTargetSkin).c_str());
                }
                for (uint32_t i = 2; i <= 9; i++)
                {
                    _snprintf_s(__szSkin1, sizeof(__szSkin1), "sourceskin%u", i);
                    _snprintf_s(__szSkin2, sizeof(__szSkin2), "targetskin%u", i);
                    if (TextFileLoader.GetTokenString(__szSkin1, &strSourceSkin) && TextFileLoader.GetTokenString(__szSkin2, &strTargetSkin))
                    {
                        AppendShapeSkin(dwShapeIndex, 0, (strPathName + strSourceSkin).c_str(), (strPathName + strTargetSkin).c_str());
                    }
                }
                TextFileLoader.SetParentNode();
            }
        }

        TextFileLoader.SetParentNode();
    }

    if (TextFileLoader.SetChildNode("hairdata"))
    {
        std::string strPathName;
        uint32_t dwHairDataCount = 0;
        if (TextFileLoader.GetTokenString("pathname", &strPathName) && TextFileLoader.GetTokenDoubleWord("hairdatacount", &dwHairDataCount))
        {

            for (uint32_t i = 0; i < dwHairDataCount; ++i)
            {
                if (!TextFileLoader.SetChildNode("hairdata", i))
                    continue;

                TextFileLoader.GetTokenString("specialpath", &strPathName);

                uint32_t dwShapeIndex = 0;
                if (!TextFileLoader.GetTokenDoubleWord("hairindex", &dwShapeIndex))
                    continue;

                std::string strModel;
                std::string strSourceSkin;
                std::string strTargetSkin;
                if (TextFileLoader.GetTokenString("model", &strModel) && TextFileLoader.GetTokenString("sourceskin", &strSourceSkin) && TextFileLoader.GetTokenString("targetskin", &strTargetSkin))
                {
                    SetHairSkin(dwShapeIndex, 0, (strPathName + strModel).c_str(), (strPathName + strSourceSkin).c_str(), (strPathName + strTargetSkin).c_str());
                }

                TextFileLoader.SetParentNode();
            }
        }

        TextFileLoader.SetParentNode();
    }


    if (TextFileLoader.SetChildNode("attachingdata"))
    {
        if (!NRaceData::LoadAttachingData(TextFileLoader, &m_AttachingDataVector))
            return false;

        TextFileLoader.SetParentNode();
    }

    return true;
}