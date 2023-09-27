#pragma once
#include "RaceMotionDataEvent.h"

#include <glm/vec3.hpp>

class CRaceMotionData
{
public:
    typedef struct SComboInputData
    {
        float fInputStartTime;
        float fNextComboTime;
        float fInputEndTime;
    } TComboInputData;

    typedef NMotionEvent::SMotionEventData TMotionEventData;
    typedef NMotionEvent::SMotionEventDataScreenWaving TScreenWavingEventData;
    typedef NMotionEvent::SMotionEventDataScreenFlashing TScreenFlashingEventData;
    typedef NMotionEvent::SMotionEventDataEffect TMotionEffectEventData;
    typedef NMotionEvent::SMotionEventDataFly TMotionFlyEventData;
    typedef NMotionEvent::SMotionEventDataAttack TMotionAttackingEventData;
    typedef NMotionEvent::SMotionEventDataSound TMotionSoundEventData;
    typedef NMotionEvent::SMotionEventDataCharacterShow TMotionCharacterShowEventData;
    typedef NMotionEvent::SMotionEventDataCharacterHide TMotionCharacterHideEventData;
    typedef NMotionEvent::SMotionEventDataWarp TMotionWarpEventData;
    typedef NMotionEvent::SMotionEventDataEffectToTarget TMotionEffectToTargetEventData;
    typedef NMotionEvent::SMotionEventDataRelativeMoveOn TMotionRelativeMoveOnEventData;
    typedef NMotionEvent::SMotionEventDataRelativeMoveOff TMotionRelativeMoveOffEventData;

    typedef std::vector<std::unique_ptr<TMotionEventData>> TMotionEventDataVector;

public:
    CRaceMotionData();

    void SetName(uint32_t eName);

    uint32_t GetType() const;
    bool IsLock() const;

    int32_t GetLoopCount() const;

    const char* GetMotionFileName() const;
    const char* GetSoundScriptFileName() const;

    void SetMotionDuration(float fDur);
    float GetMotionDuration();

    bool IsAccumulationMotion() const
    {
        return m_isAccumulationMotion;
    }
    void SetAccumulationPosition(glm::vec3 c_rPos);
    const glm::vec3& GetAccumulationPosition() const
    {
        return m_accumulationPosition;
    }

    bool IsComboInputTimeData() const;

    float GetComboInputStartTime() const;
    float GetNextComboTime() const;
    float GetComboInputEndTime() const;

    // Attacking
    bool isAttackingMotion() const;
    const NRaceData::TMotionAttackData* GetMotionAttackDataPointer() const;
    const NRaceData::TMotionAttackData& GetMotionAttackDataReference() const;
    bool HasSplashMotionEvent() const;

    // Skill
    bool IsCancelEnableSkill() const;

    // Loop
    bool IsLoopMotion() const;
    float GetLoopStartTime() const;
    float GetLoopEndTime() const;

    // Motion Event Data
    uint32_t GetMotionEventDataCount() const;
    bool GetMotionEventDataPointer(uint8_t byIndex, const CRaceMotionData::TMotionEventData** c_ppData) const;

    bool GetMotionAttackingEventDataPointer(uint8_t byIndex, const CRaceMotionData::TMotionAttackingEventData** c_ppMotionEventData) const;

    int GetEventType(uint32_t dwIndex) const;
    float GetEventStartTime(uint32_t dwIndex) const;

    // Sound Data
    const NSound::TSoundInstanceVector* GetSoundInstanceVectorPointer() const;

    // File
    bool SaveMotionData(const char* c_szFileName);

    bool LoadMotionData(const char* c_szFileName);
    bool LoadSoundScriptData(const char* c_szFileName);

protected:
    void SetType(uint32_t eType);

    uint32_t m_eType;
    uint32_t m_eName;
    bool m_isLock = false;
    int m_iLoopCount = 0;

    std::string m_strMotionFileName;
    std::string m_strSoundScriptDataFileName;
    float m_fMotionDuration = 0.0f;

    bool m_isAccumulationMotion = false;
    glm::vec3 m_accumulationPosition{ 0.0f, 0.0f, 0.0f };

    bool m_isComboMotion = false;
    TComboInputData m_ComboInputData{};

    bool m_isLoopMotion = false;
    float m_fLoopStartTime = 0.0f;
    float m_fLoopEndTime = 0.0f;

    bool m_isAttackingMotion = false;
    NRaceData::TMotionAttackData m_MotionAttackData{};

    bool m_bCancelEnableSkill = false;

    TMotionEventDataVector m_MotionEventDataVector;
    NSound::TSoundInstanceVector m_SoundInstanceVector;

    bool m_hasSplashEvent = false;
};
