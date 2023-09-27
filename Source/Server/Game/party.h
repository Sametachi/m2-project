#pragma once

#include <Core/Constants/Group.hpp>
#include "char.h"

enum 
{
	PARTY_ENOUGH_MINUTE_FOR_EXP_BONUS = 60,
	PARTY_HEAL_COOLTIME_LONG = 60,
	PARTY_HEAL_COOLTIME_SHORT = 30,
	PARTY_MAX_MEMBER = 8,
	PARTY_DEFAULT_RANGE = 5000,
};

class CParty;
class CDungeon;

class CPartyManager : public Singleton<CPartyManager>
{
	public:
		typedef std::map<uint32_t, LPPARTY> TPartyMap;
		typedef std::set<LPPARTY> TPCPartySet;

	public:
		CPartyManager();
		virtual ~CPartyManager();

		void		Initialize();

		void		EnablePCParty() { m_bEnablePCParty = true; PyLog("PARTY Enable"); }
		void		DisablePCParty() { m_bEnablePCParty = false; PyLog("PARTY Disable"); }
		bool		IsEnablePCParty() { return m_bEnablePCParty; }

		LPPARTY		CreateParty(LPCHARACTER pLeader);
		void		DeleteParty(LPPARTY pParty);
		void		DeleteAllParty();
		bool		SetParty(LPCHARACTER pChr);

		void		SetPartyMember(uint32_t dwPID, LPPARTY pParty);

		void		P2PLogin(uint32_t pid, const char* name);
		void		P2PLogout(uint32_t pid);

		LPPARTY		P2PCreateParty(uint32_t pid);
		void		P2PDeleteParty(uint32_t pid);
		void		P2PJoinParty(uint32_t leader, uint32_t pid, uint8_t role = 0);
		void		P2PQuitParty(uint32_t pid);

	private:
		TPartyMap	m_map_pParty;
		TPartyMap	m_map_pMobParty;

		TPCPartySet	m_set_pPCParty;

		bool		m_bEnablePCParty;
};

enum EPartyMessages
{
	PM_ATTACK,		// Attack him
	PM_RETURN,		// Return back to position
	PM_ATTACKED_BY,	// I was attacked by someone
	PM_AGGRO_INCREASE,	// My aggro is increased
};

class CParty
{
	public:
		typedef struct SMember
		{
			LPCHARACTER	pCharacter;
			bool	bNear;
			uint8_t	bRole;
			uint8_t	bLevel;
			std::string strName;
		} TMember;

		typedef std::map<uint32_t, TMember> TMemberMap;

		typedef std::map<std::string, int32_t> TFlagMap;

	public:
		CParty();
		virtual ~CParty();

		void		P2PJoin(uint32_t dwPID);
		void		P2PQuit(uint32_t dwPID);
		virtual void	Join(uint32_t dwPID);
		void		Quit(uint32_t dwPID);
		void		Link(LPCHARACTER pChr);
		void		Unlink(LPCHARACTER pChr);

		void		ChatPacketToAllMember(uint8_t type, const char* format, ...);	

		void		UpdateOnlineState(uint32_t dwPID, const char* name);
		void		UpdateOfflineState(uint32_t dwPID);

		uint32_t		GetLeaderPID();
		LPCHARACTER	GetLeaderCharacter();
		LPCHARACTER	GetLeader() { return m_pChrLeader; }

		uint32_t		GetMemberCount();
		uint32_t		GetNearMemberCount()	{ return m_iCountNearPartyMember; }

		bool		IsMember(uint32_t pid) { return m_memberMap.find(pid) != m_memberMap.end(); }

		bool		IsNearLeader(uint32_t pid);

		bool		IsPositionNearLeader(LPCHARACTER ch);

		void		SendMessage(LPCHARACTER ch, uint8_t bMsg, uint32_t dwArg1, uint32_t dwArg2);

		void		SendPartyJoinOneToAll(uint32_t dwPID);
		void		SendPartyJoinAllToOne(LPCHARACTER ch);
		void		SendPartyRemoveOneToAll(uint32_t dwPID);

		void		SendPartyInfoOneToAll(uint32_t pid);
		void		SendPartyInfoOneToAll(LPCHARACTER ch);
		void		SendPartyInfoAllToOne(LPCHARACTER ch);

		void		SendPartyLinkOneToAll(LPCHARACTER ch);
		void		SendPartyLinkAllToOne(LPCHARACTER ch);
		void		SendPartyUnlinkOneToAll(LPCHARACTER ch);

		int32_t		GetPartyBonusExpPercent()	{ return m_iExpBonus; }
		int32_t		GetPartyBonusAttackGrade()	{ return m_iAttBonus; }
		int32_t		GetPartyBonusDefenseGrade()	{ return m_iDefBonus; }

		int32_t	ComputePartyBonusExpPercent();
		inline int32_t	ComputePartyBonusAttackGrade();
		inline int32_t	ComputePartyBonusDefenseGrade();

		template <class Func> void ForEachMember(Func & f);
		template <class Func> void ForEachMemberPtr(Func & f);
		template <class Func> void ForEachOnlineMember(Func & f);
		template <class Func> void ForEachNearMember(Func & f);
		template <class Func> void ForEachOnMapMember (Func & f, int32_t lMapIndex);
		template <class Func> bool ForEachOnMapMemberBool (Func & f, int32_t lMapIndex);

		void		Update();

		int32_t		GetExpBonusPercent();

		bool		SetRole(uint32_t pid, uint8_t bRole, bool on);
		uint8_t		GetRole(uint32_t pid);
		bool		IsRole(uint32_t pid, uint8_t bRole);

		uint8_t		GetMemberMaxLevel();
		uint8_t		GetMemberMinLevel();

		void		ComputeRolePoint(LPCHARACTER ch, uint8_t bRole, bool bAdd);

		void		HealParty();
		void		SummonToLeader(uint32_t pid);

		void		SetPCParty(bool b) { m_bPCParty = b; }

		LPCHARACTER	GetNextOwnership(LPCHARACTER ch, int32_t x, int32_t y);

		void		SetFlag(const std::string& name, int32_t value);
		int32_t		GetFlag(const std::string& name);

		void		SetDungeon(LPDUNGEON pDungeon);
		LPDUNGEON	GetDungeon();

		uint8_t		CountMemberByVnum(uint32_t dwVnum);

		void		SetParameter(int32_t iMode);
		int32_t		GetExpDistributionMode();

		void		SetExpCentralizeCharacter(uint32_t pid);
		LPCHARACTER	GetExpCentralizeCharacter();

		void		RequestSetMemberLevel(uint32_t pid, uint8_t level);
		void		P2PSetMemberLevel(uint32_t pid, uint8_t level);

	protected:
		void		IncreaseOwnership();

		virtual void	Initialize();
		void		Destroy();
		void		RemovePartyBonus();

		void		RemoveBonus();
		void		RemoveBonusForOne(uint32_t pid);

		void		SendParameter(LPCHARACTER ch);
		void		SendParameterToAll();

		TMemberMap	m_memberMap;
		uint32_t		m_dwLeaderPID;
		LPCHARACTER	m_pChrLeader;

		LPEVENT		m_eventUpdate;

		TMemberMap::iterator m_itNextOwner;

	private:
		int32_t		m_iExpDistributionMode;
		LPCHARACTER	m_pChrExpCentralize;

		uint32_t		m_dwPartyStartTime;

		uint32_t		m_dwPartyHealTime;
		bool		m_bPartyHealReady;
		bool		m_bCanUsePartyHeal;

		int32_t		m_anRoleCount[PARTY_ROLE_MAX_NUM];
		int32_t		m_anMaxRole[PARTY_ROLE_MAX_NUM];

		int32_t		m_iLongTimeExpBonus;

		// used in Update
		int32_t		m_iLeadership;
		int32_t		m_iExpBonus;
		int32_t		m_iAttBonus;
		int32_t		m_iDefBonus;

		// changed only in Update
		int32_t		m_iCountNearPartyMember;

		bool		m_bPCParty;

		TFlagMap	m_map_iFlag;

		LPDUNGEON	m_pDungeon;
		LPDUNGEON	m_pDungeon_for_Only_party;
	public:
		void SetDungeon_for_Only_party(LPDUNGEON pDungeon);
		LPDUNGEON GetDungeon_for_Only_party();
};

template <class Func> void CParty::ForEachMember(Func & f)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
		f(it->first);
}

template <class Func> void CParty::ForEachMemberPtr(Func & f)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
		f(it->second.pCharacter);
}

template <class Func> void CParty::ForEachOnlineMember(Func & f)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
		if (it->second.pCharacter)
			f(it->second.pCharacter);
}

template <class Func> void CParty::ForEachNearMember(Func & f)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
		if (it->second.pCharacter && it->second.bNear)
			f(it->second.pCharacter);
}

template <class Func> void CParty::ForEachOnMapMember (Func & f, int32_t lMapIndex)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		LPCHARACTER ch = it->second.pCharacter;
		if (ch)
		{
			if (ch->GetMapIndex () == lMapIndex)
				f(ch);
		}
	}
}

template <class Func> bool CParty::ForEachOnMapMemberBool(Func & f, int32_t lMapIndex)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		LPCHARACTER ch = it->second.pCharacter;
		if (ch)
		{
			if (ch->GetMapIndex () == lMapIndex)
			{
				if(f(ch) == false)
				{
					return false;
			
				}
			}
		}
	}
	return true;
}

inline int32_t CParty::ComputePartyBonusAttackGrade()
{
	/*
	   if (GetNearMemberCount() <= 1)
	   return 0;

	   int32_t leadership = GetLeaderCharacter()->GetLeadershipSkillLevel();
	   int32_t n = GetNearMemberCount();

	   if (n >= 3 && leadership >= 10)
	   return 2;

	   if (n >= 2 && leadership >= 4)
	   return 1;
	 */
	return 0;
}

inline int32_t CParty::ComputePartyBonusDefenseGrade()
{
	/*
	   if (GetNearMemberCount() <= 1)
	   return 0;

	   int32_t leadership = GetLeaderCharacter()->GetLeadershipSkillLevel();
	   int32_t n = GetNearMemberCount();

	   if (n >= 5 && leadership >= 24)
	   return 2;

	   if (n >= 4 && leadership >= 16)
	   return 1;
	 */
	return 0;
}
