#include "StdAfx.h"
#include "FlyingObjectManager.h"
#include "RaceMotionData.h"

#include <EffectLib/EffectManager.h>
#include <Core/Race/RaceMotionConstants.hpp>
#include <Storm/StringUtil.hpp>

CRaceMotionData::CRaceMotionData() : m_eType(MOTION_TYPE_NONE), m_eName(MOTION_NONE)
{
    m_iLoopCount = 0;
    m_fMotionDuration = 0.0f;
    m_accumulationPosition.x = 0.0f;
    m_accumulationPosition.y = 0.0f;
    m_accumulationPosition.z = 0.0f;
    m_fLoopStartTime = 0.0f;
    m_fLoopEndTime = 0.0f;

    m_isAccumulationMotion = false;
    m_isComboMotion = false;
    m_isLoopMotion = false;
    m_isAttackingMotion = false;
    m_bCancelEnableSkill = false;

    m_hasSplashEvent = false;

    m_isLock = false;

    m_eType = MOTION_TYPE_NONE;
    m_eName = MOTION_NONE;

    m_MotionEventDataVector.clear();
    m_SoundInstanceVector.clear();
}

void CRaceMotionData::SetName(uint32_t eName)
{
    m_eName = eName;

    switch (m_eName)
    {
    case MOTION_NONE:
        SetType(MOTION_TYPE_NONE);
        break;

    case MOTION_WAIT:
    case MOTION_INTRO_WAIT:
    case MOTION_STOP:
        SetType(MOTION_TYPE_WAIT);
        break;

    case MOTION_WALK:
    case MOTION_RUN:
        SetType(MOTION_TYPE_MOVE);
        break;

    case MOTION_DAMAGE:
    case MOTION_DAMAGE_BACK:
        SetType(MOTION_TYPE_DAMAGE);
        break;

    case MOTION_DAMAGE_FLYING:
    case MOTION_DAMAGE_FLYING_BACK:
        SetType(MOTION_TYPE_KNOCKDOWN);
        break;

    case MOTION_STAND_UP:
    case MOTION_STAND_UP_BACK:
        SetType(MOTION_TYPE_STANDUP);
        break;

    case MOTION_SPAWN:
    case MOTION_CHANGE_WEAPON:
    case MOTION_INTRO_SELECTED:
    case MOTION_INTRO_NOT_SELECTED:
    case MOTION_SPECIAL_1:
    case MOTION_SPECIAL_2:
    case MOTION_SPECIAL_3:
    case MOTION_SPECIAL_4:
    case MOTION_SPECIAL_5:
    case MOTION_SPECIAL_6:
    case MOTION_DIG:

        SetType(MOTION_TYPE_EVENT);
        break;

    case MOTION_CLAP:
    case MOTION_DANCE_1:
    case MOTION_DANCE_2:
    case MOTION_DANCE_3:
    case MOTION_DANCE_4:
    case MOTION_DANCE_5:
    case MOTION_DANCE_6:
    case MOTION_CONGRATULATION:
    case MOTION_FORGIVE:
    case MOTION_ANGRY:
    case MOTION_ATTRACTIVE:
    case MOTION_SAD:
    case MOTION_SHY:
    case MOTION_CHEERUP:
    case MOTION_BANTER:
    case MOTION_JOY:
    case MOTION_CHEERS_1:
    case MOTION_CHEERS_2:
    case MOTION_KISS_WITH_WARRIOR:
    case MOTION_KISS_WITH_ASSASSIN:
    case MOTION_KISS_WITH_SURA:
    case MOTION_KISS_WITH_SHAMAN:
    case MOTION_FRENCH_KISS_WITH_WARRIOR:
    case MOTION_FRENCH_KISS_WITH_ASSASSIN:
    case MOTION_FRENCH_KISS_WITH_SURA:
    case MOTION_FRENCH_KISS_WITH_SHAMAN:
    case MOTION_SLAP_HIT_WITH_WARRIOR:
    case MOTION_SLAP_HIT_WITH_ASSASSIN:
    case MOTION_SLAP_HIT_WITH_SURA:
    case MOTION_SLAP_HIT_WITH_SHAMAN:
    case MOTION_SLAP_HURT_WITH_WARRIOR:
    case MOTION_SLAP_HURT_WITH_ASSASSIN:
    case MOTION_SLAP_HURT_WITH_SURA:
    case MOTION_SLAP_HURT_WITH_SHAMAN:
        SetType(MOTION_TYPE_EMOTION);
        break;

    case MOTION_DEAD:
    case MOTION_DEAD_BACK:
        SetType(MOTION_TYPE_DIE);
        break;

    case MOTION_NORMAL_ATTACK:
        SetType(MOTION_TYPE_ATTACK);
        break;

    case MOTION_COMBO_ATTACK_1:
    case MOTION_COMBO_ATTACK_2:
    case MOTION_COMBO_ATTACK_3:
    case MOTION_COMBO_ATTACK_4:
    case MOTION_COMBO_ATTACK_5:
    case MOTION_COMBO_ATTACK_6:
    case MOTION_COMBO_ATTACK_7:
    case MOTION_COMBO_ATTACK_8:
        SetType(MOTION_TYPE_COMBO);
        break;

    case MOTION_FISHING_THROW:
    case MOTION_FISHING_WAIT:
    case MOTION_FISHING_REACT:
    case MOTION_FISHING_CATCH:
    case MOTION_FISHING_FAIL:
    case MOTION_FISHING_STOP:
        SetType(MOTION_TYPE_FISHING);
        break;

    default:
        if (eName >= MOTION_SKILL && eName <= MOTION_SKILL + SKILL_MAX_NUM)
            SetType(MOTION_TYPE_SKILL);
        else
            SysLog("CRaceMotionData::SetName - UNKNOWN NAME {0}", eName);
        break;
    }
}

void CRaceMotionData::SetType(uint32_t eType)
{
    m_eType = eType;

    switch (m_eType)
    {
    case MOTION_TYPE_ATTACK:
    case MOTION_TYPE_COMBO:
    case MOTION_TYPE_SKILL:
        m_isLock = true;
        break;

    default:
        m_isLock = false;
        break;
    }
}

uint32_t CRaceMotionData::GetType() const
{
    return m_eType;
}

bool CRaceMotionData::IsLock() const
{
    return m_isLock;
}

int CRaceMotionData::GetLoopCount() const
{
    return m_iLoopCount;
}

float CRaceMotionData::GetMotionDuration()
{
    return m_fMotionDuration;
}

void CRaceMotionData::SetMotionDuration(float fDuration)
{
    m_fMotionDuration = fDuration;
}

// Combo
bool CRaceMotionData::IsComboInputTimeData() const
{
    return m_isComboMotion;
}

float CRaceMotionData::GetComboInputStartTime() const
{
    assert(m_isComboMotion);
    return m_ComboInputData.fInputStartTime;
}

float CRaceMotionData::GetNextComboTime() const
{
    assert(m_isComboMotion);
    return m_ComboInputData.fNextComboTime;
}

float CRaceMotionData::GetComboInputEndTime() const
{
    assert(m_isComboMotion);
    return m_ComboInputData.fInputEndTime;
}

// Attacking
bool CRaceMotionData::isAttackingMotion() const
{
    return m_isAttackingMotion;
}

const NRaceData::TMotionAttackData* CRaceMotionData::GetMotionAttackDataPointer() const
{
    return &m_MotionAttackData;
}

const NRaceData::TMotionAttackData& CRaceMotionData::GetMotionAttackDataReference() const
{
    assert(m_isAttackingMotion);
    return m_MotionAttackData;
}

bool CRaceMotionData::HasSplashMotionEvent() const
{
    return m_hasSplashEvent;
}

// Skill
bool CRaceMotionData::IsCancelEnableSkill() const
{
    return m_bCancelEnableSkill;
}

// Loop
bool CRaceMotionData::IsLoopMotion() const
{
    return m_isLoopMotion;
}

float CRaceMotionData::GetLoopStartTime() const
{
    return m_fLoopStartTime;
}

float CRaceMotionData::GetLoopEndTime() const
{
    return m_fLoopEndTime;
}

// Motion Event Data
uint32_t CRaceMotionData::GetMotionEventDataCount() const
{
    return m_MotionEventDataVector.size();
}

bool CRaceMotionData::GetMotionEventDataPointer(uint8_t byIndex, const CRaceMotionData::TMotionEventData** c_ppData) const
{
    if (byIndex >= m_MotionEventDataVector.size())
        return false;

    *c_ppData = m_MotionEventDataVector[byIndex].get();
    return true;
}

bool CRaceMotionData::GetMotionAttackingEventDataPointer(uint8_t byIndex, const CRaceMotionData::TMotionAttackingEventData** c_ppData) const
{
    if (byIndex >= m_MotionEventDataVector.size())
        return false;

    const CRaceMotionData::TMotionEventData* pData = m_MotionEventDataVector[byIndex].get();
    const CRaceMotionData::TMotionAttackingEventData* pAttackingEvent = (const CRaceMotionData::TMotionAttackingEventData*)pData;

    if (MOTION_EVENT_TYPE_SPECIAL_ATTACKING == pAttackingEvent->iType)
        return false;

    *c_ppData = pAttackingEvent;

    return true;
}

int CRaceMotionData::GetEventType(uint32_t dwIndex) const
{
    if (dwIndex >= m_MotionEventDataVector.size())
        return MOTION_EVENT_TYPE_NONE;

    return m_MotionEventDataVector[dwIndex]->iType;
}

float CRaceMotionData::GetEventStartTime(uint32_t dwIndex) const
{
    if (dwIndex >= m_MotionEventDataVector.size())
        return 0.0f;

    return m_MotionEventDataVector[dwIndex]->fStartingTime;
}

const NSound::TSoundInstanceVector* CRaceMotionData::GetSoundInstanceVectorPointer() const
{
    return &m_SoundInstanceVector;
}

void CRaceMotionData::SetAccumulationPosition(glm::vec3 c_rPos)
{
    m_accumulationPosition = c_rPos;
    m_isAccumulationMotion = true;
}

bool CRaceMotionData::LoadMotionData(const char* c_szFileName)
{
    const float c_fFrameTime = 1.0f / g_fGameFPS;

    CTextFileLoader* pkTextFileLoader = CTextFileLoader::Cache(c_szFileName);
    if (!pkTextFileLoader)
        return false;

    CTextFileLoader& rkTextFileLoader = *pkTextFileLoader;

    if (rkTextFileLoader.IsEmpty())
        return false;

    rkTextFileLoader.SetTop();

    if (!rkTextFileLoader.GetTokenString("motionfilename", &m_strMotionFileName))
        return false;

    if (!rkTextFileLoader.GetTokenFloat("motionduration", &m_fMotionDuration))
        return false;

    CTokenVector* pTokenVector;

    if (rkTextFileLoader.GetTokenVector("accumulation", &pTokenVector))
    {
        if (pTokenVector->size() != 3)
        {
            return false;
        }

        float x, y, z;
        storm::ParseNumber(pTokenVector->at(0), x);
        storm::ParseNumber(pTokenVector->at(1), y);
        storm::ParseNumber(pTokenVector->at(2), z);

        glm::vec3 pos(x, y, z);

        SetAccumulationPosition(pos);
    }

    std::string strNodeName;
    for (uint32_t i = 0; i < rkTextFileLoader.GetChildNodeCount(); ++i)
    {
        CTextFileLoader::CGotoChild GotoChild(&rkTextFileLoader, i);

        rkTextFileLoader.GetCurrentNodeName(&strNodeName);

        if (0 == strNodeName.compare("comboinputdata"))
        {
            m_isComboMotion = true;

            if (!rkTextFileLoader.GetTokenFloat("preinputtime", &m_ComboInputData.fInputStartTime))
                return false;
            if (!rkTextFileLoader.GetTokenFloat("directinputtime", &m_ComboInputData.fNextComboTime))
                return false;
            if (!rkTextFileLoader.GetTokenFloat("inputlimittime", &m_ComboInputData.fInputEndTime))
                return false;
        }
        else if (0 == strNodeName.compare("attackingdata"))
        {
            m_isAttackingMotion = true;

            if (!NRaceData::LoadMotionAttackData(rkTextFileLoader, &m_MotionAttackData))
                return false;
        }
        else if (0 == strNodeName.compare("loopdata"))
        {
            m_isLoopMotion = true;
            if (!rkTextFileLoader.GetTokenInteger("motionloopcount", &m_iLoopCount))
            {
                m_iLoopCount = -1;
            }
            if (!rkTextFileLoader.GetTokenBoolean("loopcancelenable", &m_bCancelEnableSkill))
            {
                m_bCancelEnableSkill = false;
            }
            if (!rkTextFileLoader.GetTokenFloat("loopstarttime", &m_fLoopStartTime))
                return false;
            if (!rkTextFileLoader.GetTokenFloat("loopendtime", &m_fLoopEndTime))
                return false;
        }
        else if (0 == strNodeName.compare("motioneventdata"))
        {
            uint32_t dwMotionEventDataCount;

            if (!rkTextFileLoader.GetTokenDoubleWord("motioneventdatacount", &dwMotionEventDataCount))
                continue;

            m_MotionEventDataVector.clear();
            m_MotionEventDataVector.reserve(dwMotionEventDataCount);
            m_MotionEventDataVector.resize(dwMotionEventDataCount);

            for (uint32_t j = 0; j < m_MotionEventDataVector.size(); ++j)
            {
                if (!rkTextFileLoader.SetChildNode("event", j))
                    return false;

                int32_t iType;
                if (!rkTextFileLoader.GetTokenInteger("motioneventtype", &iType))
                    return false;

                TMotionEventData* pData = nullptr;
                switch (iType)
                {
                case MOTION_EVENT_TYPE_FLY:
                    pData = new TMotionFlyEventData;
                    break;
                case MOTION_EVENT_TYPE_EFFECT:
                    pData = new TMotionEffectEventData;
                    break;
                case MOTION_EVENT_TYPE_SCREEN_WAVING:
                    pData = new TScreenWavingEventData;
                    break;
                case MOTION_EVENT_TYPE_SPECIAL_ATTACKING:
                    pData = new TMotionAttackingEventData;
                    m_hasSplashEvent = true;
                    break;
                case MOTION_EVENT_TYPE_SOUND:
                    pData = new TMotionSoundEventData;
                    break;
                case MOTION_EVENT_TYPE_CHARACTER_SHOW:
                    pData = new TMotionCharacterShowEventData;
                    break;
                case MOTION_EVENT_TYPE_CHARACTER_HIDE:
                    pData = new TMotionCharacterHideEventData;
                    break;
                case MOTION_EVENT_TYPE_WARP:
                    pData = new TMotionWarpEventData;
                    break;
                case MOTION_EVENT_TYPE_EFFECT_TO_TARGET:
                    pData = new TMotionEffectToTargetEventData;
                    break;
                default:
                    assert(!" CRaceMotionData::LoadMotionData - Strange Event Type");
                    return false;
                    break;
                }
                m_MotionEventDataVector[j].reset(static_cast<TMotionEventData*>(pData));
                m_MotionEventDataVector[j]->Load(rkTextFileLoader);
                m_MotionEventDataVector[j]->iType = iType;
                if (!rkTextFileLoader.GetTokenFloat("startingtime", &m_MotionEventDataVector[j]->fStartingTime))
                    return false;

                m_MotionEventDataVector[j]->dwFrame = static_cast<uint32_t>((m_MotionEventDataVector[j]->fStartingTime / c_fFrameTime));

                rkTextFileLoader.SetParentNode();
            }
        }
    }

    std::string strSoundFileNameTemp = c_szFileName;
    strSoundFileNameTemp = CFileNameHelper::NoExtension(strSoundFileNameTemp);
    strSoundFileNameTemp += ".mss";

    if (strSoundFileNameTemp.length() > 13)
    {
        const char* c_szHeader = &strSoundFileNameTemp[13];

        m_strSoundScriptDataFileName = "sound/";
        m_strSoundScriptDataFileName += c_szHeader;

        LoadSoundScriptData(m_strSoundScriptDataFileName.c_str());
    }

    return true;
}

bool CRaceMotionData::SaveMotionData(const char* c_szFileName)
{
    FILE* File;

    SetFileAttributes(c_szFileName, FILE_ATTRIBUTE_NORMAL);
    File = fopen(c_szFileName, "we");

    if (!File)
    {
        SysLog("CRaceMotionData::SaveMotionData : cannot open file for writing (filename: {0})", c_szFileName);
        return false;
    }

    PrintfTabs(File, 0, "ScriptType               MotionData\n");
    PrintfTabs(File, 0, "\n");

    PrintfTabs(File, 0, "MotionFileName           \"%s\"\n", m_strMotionFileName.c_str());
    PrintfTabs(File, 0, "MotionDuration           %f\n", m_fMotionDuration);

    if (m_isAccumulationMotion)
        PrintfTabs(File, 0, "Accumulation           %.2f\t%.2f\t%.2f\n", m_accumulationPosition.x,
            m_accumulationPosition.y, m_accumulationPosition.z);

    PrintfTabs(File, 0, "\n");

    if (m_isComboMotion)
    {
        PrintfTabs(File, 0, "Group ComboInputData\n");
        PrintfTabs(File, 0, "{\n");
        PrintfTabs(File, 1, "PreInputTime             %f\n", m_ComboInputData.fInputStartTime);
        PrintfTabs(File, 1, "DirectInputTime          %f\n", m_ComboInputData.fNextComboTime);
        PrintfTabs(File, 1, "InputLimitTime           %f\n", m_ComboInputData.fInputEndTime);
        PrintfTabs(File, 0, "}\n");
        PrintfTabs(File, 0, "\n");
    }

    if (m_isAttackingMotion)
    {
        PrintfTabs(File, 0, "Group AttackingData\n");
        PrintfTabs(File, 0, "{\n");
        NRaceData::SaveMotionAttackData(File, 1, m_MotionAttackData);
        PrintfTabs(File, 0, "}\n");
        PrintfTabs(File, 0, "\n");
    }

    if (m_isLoopMotion)
    {
        PrintfTabs(File, 0, "Group LoopData\n");
        PrintfTabs(File, 0, "{\n");
        PrintfTabs(File, 1, "MotionLoopCount          %d\n", m_iLoopCount);
        PrintfTabs(File, 1, "LoopCancelEnable         %d\n", m_bCancelEnableSkill);
        PrintfTabs(File, 1, "LoopStartTime            %f\n", m_fLoopStartTime);
        PrintfTabs(File, 1, "LoopEndTime              %f\n", m_fLoopEndTime);
        PrintfTabs(File, 0, "}\n");
        PrintfTabs(File, 0, "\n");
    }

    if (!m_MotionEventDataVector.empty())
    {
        PrintfTabs(File, 0, "Group MotionEventData\n");
        PrintfTabs(File, 0, "{\n");
        PrintfTabs(File, 1, "MotionEventDataCount     %d\n", m_MotionEventDataVector.size());

        for (uint32_t j = 0; j < m_MotionEventDataVector.size(); ++j)
        {
            const auto& c_pData = m_MotionEventDataVector[j];

            PrintfTabs(File, 1, "Group Event%02d\n", j);
            PrintfTabs(File, 1, "{\n");
            PrintfTabs(File, 2, "MotionEventType      %d\n", c_pData->iType);
            PrintfTabs(File, 2, "StartingTime         %f\n", c_pData->fStartingTime);
            c_pData->Save(File, 2);
            PrintfTabs(File, 1, "}\n");
        }

        PrintfTabs(File, 0, "}\n");
    }

    return true;
}

bool CRaceMotionData::LoadSoundScriptData(const char* c_szFileName)
{
    if (!CallFS().Exists(c_szFileName))
        return false;

    NSound::TSoundDataVector SoundDataVector;
    if (!NSound::LoadSoundInformationPiece(c_szFileName, SoundDataVector))
    {
        return false;
    }

    NSound::DataToInstance(SoundDataVector, &m_SoundInstanceVector);
    return true;
}

const char* CRaceMotionData::GetMotionFileName() const
{
    return m_strMotionFileName.c_str();
}

const char* CRaceMotionData::GetSoundScriptFileName() const
{
    return m_strSoundScriptDataFileName.c_str();
}
