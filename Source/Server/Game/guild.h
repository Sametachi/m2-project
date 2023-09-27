#pragma once

#include <Core/Constants/Guild.hpp>
#include "skill.h"

typedef struct _SQLMsg SQLMsg;

struct SGuildMaster
{
	uint32_t pid;
};


typedef struct SGuildMember
{
	uint32_t pid;
	uint8_t grade;
	uint8_t is_general;
	uint8_t job;
	uint8_t level;
	uint32_t offer_exp;
	uint8_t _dummy;

	std::string name;

	SGuildMember(LPCHARACTER ch, uint8_t grade, uint32_t offer_exp);
	SGuildMember(uint32_t pid, uint8_t grade, uint8_t is_general, uint8_t job, uint8_t level, uint32_t offer_exp, char* name);

} TGuildMember;

#pragma pack(1)
typedef struct SGuildMemberPacketData
{   
	uint32_t pid;
	uint8_t grade;
	uint8_t is_general;
	uint8_t job;
	uint8_t level;
	uint32_t offer;
	uint8_t name_flag;
	char name[CHARACTER_NAME_MAX_LEN+1];
} TGuildMemberPacketData;

typedef struct SGuildGrade
{
	char grade_name[GUILD_GRADE_NAME_MAX_LEN+1];
	uint8_t auth_flag;
} TGuildGrade;

struct TOneGradeNamePacket
{
	uint8_t grade;
	char grade_name[GUILD_GRADE_NAME_MAX_LEN+1];
};

struct TOneGradeAuthPacket
{
	uint8_t grade;
	uint8_t auth;
};
#pragma pack()

typedef struct SGuildData
{
	uint32_t	guild_id;
	uint32_t	master_pid;
	uint32_t	exp;
	uint8_t	level;
	char	name[GUILD_NAME_MAX_LEN+1];

	TGuildGrade	grade_array[GUILD_GRADE_COUNT];

	uint8_t	skill_point;
	uint8_t	abySkill[GUILD_SKILL_COUNT];

	int32_t		power;
	int32_t		max_power;

	int32_t		ladder_point;

	int32_t		win;
	int32_t		draw;
	int32_t		loss;

	int32_t		gold;
} TGuildData;

struct TGuildCreateParameter
{
	LPCHARACTER master;
	char name[GUILD_NAME_MAX_LEN+1];
};

typedef struct SGuildWar
{
	uint32_t war_start_time;
	uint32_t score;
	uint32_t state;
	uint8_t type;
	uint32_t map_index;

	SGuildWar(uint8_t type)
		: war_start_time(0),
	score(0),
	state(GUILD_WAR_RECV_DECLARE),
	type(type),
	map_index(0)
	{
	}
	bool IsWarBegin() const
	{
		return state == GUILD_WAR_ON_WAR;
	}
} TGuildWar;

class CGuild
{
	public:
		CGuild(TGuildCreateParameter& cp);
		explicit CGuild(uint32_t guild_id) { Load(guild_id); }
		~CGuild();

		uint32_t		GetID() const	{ return m_data.guild_id; }
		const char*	GetName() const	{ return m_data.name; }
		int32_t		GetSP() const		{ return m_data.power; }
		int32_t		GetMaxSP() { return m_data.max_power; }
		uint32_t		GetMasterPID() const	{ return m_data.master_pid; }
		LPCHARACTER	GetMasterCharacter();
		uint8_t		GetLevel() const		{ return m_data.level; }

		void		Reset() { m_data.power = m_data.max_power; }

		void		RequestDisband(uint32_t pid);
		void		Disband();

		void		RequestAddMember(LPCHARACTER ch, int32_t grade = 15);
		void		AddMember(TPacketDGGuildMember* p);

		bool		RequestRemoveMember(uint32_t pid);
		bool		RemoveMember(uint32_t pid);

		void		LoginMember(LPCHARACTER ch);
		void		P2PLoginMember(uint32_t pid);

		void		LogoutMember(LPCHARACTER ch);
		void		P2PLogoutMember(uint32_t pid);

		void		ChangeMemberGrade(uint32_t pid, uint8_t grade);
		bool		OfferExp(LPCHARACTER ch, int32_t amount);
		void		LevelChange(uint32_t pid, uint8_t level);
		void		ChangeMemberData(uint32_t pid, uint32_t offer, uint8_t level, uint8_t grade);

		void		ChangeGradeName(uint8_t grade, const char* grade_name);
		void		ChangeGradeAuth(uint8_t grade, uint8_t auth);
		void		P2PChangeGrade(uint8_t grade);

		bool		ChangeMemberGeneral(uint32_t pid, uint8_t is_general);

		bool		ChangeMasterTo(uint32_t dwPID);

		void		Packet(const void* buf, int32_t size);

		void		SendOnlineRemoveOnePacket(uint32_t pid);
		void		SendAllGradePacket(LPCHARACTER ch);
		void		SendListPacket(LPCHARACTER ch);
		void		SendListOneToAll(uint32_t pid);
		void		SendListOneToAll(LPCHARACTER ch);
		void		SendLoginPacket(LPCHARACTER ch, LPCHARACTER chLogin);
		void		SendLogoutPacket(LPCHARACTER ch, LPCHARACTER chLogout);
		void		SendLoginPacket(LPCHARACTER ch, uint32_t pid);
		void		SendLogoutPacket(LPCHARACTER ch, uint32_t pid);
		void		SendGuildInfoPacket(LPCHARACTER ch);
		void		SendGuildDataUpdateToAllMember(SQLMsg* pmsg);

		void		Load(uint32_t guild_id);
		void		SaveLevel();
		void		SaveSkill();
		void		SaveMember(uint32_t pid);

		int32_t		GetMaxMemberCount(); 
		int32_t		GetMemberCount() { return m_member.size(); }
		int32_t		GetTotalLevel() const;

		void		SetMemberCountBonus(int32_t iBonus);
		void		BroadcastMemberCountBonus();
		
		int32_t		GetMaxGeneralCount() const	{ return 1 /*+ GetSkillLevel(GUILD_SKILL_DEUNGYONG)/3*/;}
		int32_t		GetGeneralCount() const		{ return m_general_count; }

		TGuildMember*	GetMember(uint32_t pid);
		uint32_t			GetMemberPID(const std::string& strName);

		bool		HasGradeAuth(int32_t grade, int32_t auth_flag) const	{ return (bool)(m_data.grade_array[grade-1].auth_flag & auth_flag);}

		void		AddComment(LPCHARACTER ch, const std::string& str);
		void		DeleteComment(LPCHARACTER ch, uint32_t comment_id);

		void		RefreshComment(LPCHARACTER ch);
		void		RefreshCommentForce(uint32_t player_id);

		int32_t			GetSkillLevel(uint32_t vnum);
		void		SkillLevelUp(uint32_t dwVnum);
		void		UseSkill(uint32_t dwVnum, LPCHARACTER ch, uint32_t pid);

		void		SendSkillInfoPacket(LPCHARACTER ch) const;
		void		ComputeGuildPoints();

		void		GuildPointChange(uint8_t type, int32_t amount, bool save = false);

		void		UpdateSkill(uint8_t grade, uint8_t* skill_levels);
		void		SendDBSkillUpdate(int32_t amount = 0);

		void		SkillRecharge();
		bool		ChargeSP(LPCHARACTER ch, int32_t iSP);

		void		Chat(const char* c_pszText); 
		void		P2PChat(const char* c_pszText);

		void		SkillUsableChange(uint32_t dwSkillVnum, bool bUsable);
		void		AdvanceLevel(int32_t iLevel);

		// Guild Money
		void		RequestDepositMoney(LPCHARACTER ch, int32_t iGold);
		void		RequestWithdrawMoney(LPCHARACTER ch, int32_t iGold);

		void		RecvMoneyChange(int32_t iGold);
		void		RecvWithdrawMoneyGive(int32_t iChangeGold);

		int32_t		GetGuildMoney() const	{ return m_data.gold; }

		void		GuildWarPacket(uint32_t guild_id, uint8_t bWarType, uint8_t bWarState);
		void		SendEnemyGuild(LPCHARACTER ch);

		int32_t		GetGuildWarState(uint32_t guild_id);
		bool		CanStartWar(uint8_t bGuildWarType);
		uint32_t		GetWarStartTime(uint32_t guild_id);
		bool		UnderWar(uint32_t guild_id);
		uint32_t		UnderAnyWar(uint8_t bType = GUILD_WAR_TYPE_MAX_NUM);

		void		SetGuildWarMapIndex(uint32_t dwGuildID, int32_t lMapIndex);
		int32_t			GetGuildWarType(uint32_t dwGuildOpponent);
		uint32_t		GetGuildWarMapIndex(uint32_t dwGuildOpponent);

		void		GuildWarEntryAsk(uint32_t guild_opp);
		void		GuildWarEntryAccept(uint32_t guild_opp, LPCHARACTER ch);

		void		NotifyGuildMaster(const char* msg);
		void		RequestDeclareWar(uint32_t guild_id, uint8_t type);
		void		RequestRefuseWar(uint32_t guild_id); 

		bool		DeclareWar(uint32_t guild_id, uint8_t type, uint8_t state); 
		void		RefuseWar(uint32_t guild_id); 
		bool		WaitStartWar(uint32_t guild_id); 
		bool		CheckStartWar(uint32_t guild_id);
		void		StartWar(uint32_t guild_id);
		void		EndWar(uint32_t guild_id);
		void		ReserveWar(uint32_t guild_id, uint8_t type);

		void		SetWarScoreAgainstTo(uint32_t guild_opponent, int32_t newpoint);
		int32_t			GetWarScoreAgainstTo(uint32_t guild_opponent);

		int32_t			GetLadderPoint() const	{ return m_data.ladder_point; }
		void		SetLadderPoint(int32_t point);

		void		SetWarData(int32_t iWin, int32_t iDraw, int32_t iLoss) { m_data.win = iWin, m_data.draw = iDraw, m_data.loss = iLoss; }

		void		ChangeLadderPoint(int32_t iChange);

		int32_t			GetGuildWarWinCount() const { return m_data.win; }
		int32_t			GetGuildWarDrawCount() const { return m_data.draw; }
		int32_t			GetGuildWarLossCount() const { return m_data.loss; }

		bool		HasLand();

		void		Invite(LPCHARACTER pchInviter, LPCHARACTER pchInvitee);

		void		InviteAccept(LPCHARACTER pchInvitee);

		void		InviteDeny(uint32_t dwPID);
		
	private:
		void		Initialize();

		TGuildData	m_data;
		int32_t		m_general_count;

		int32_t		m_iMemberCountBonus;
		
		typedef std::map<uint32_t, TGuildMember> TGuildMemberContainer;
		TGuildMemberContainer m_member;

		typedef CHARACTER_SET TGuildMemberOnlineContainer;
		TGuildMemberOnlineContainer m_memberOnline;

		typedef std::set<uint32_t>	TGuildMemberP2POnlineContainer;
		TGuildMemberP2POnlineContainer m_memberP2POnline;

		void LoadGuildData(SQLMsg* pmsg);
		void LoadGuildGradeData(SQLMsg* pmsg);
		void LoadGuildMemberData(SQLMsg* pmsg);

		void __P2PUpdateGrade(SQLMsg* pmsg);

		typedef std::map<uint32_t, TGuildWar> TEnemyGuildContainer;
		TEnemyGuildContainer m_EnemyGuild;

		std::map<uint32_t, uint32_t> m_mapGuildWarEndTime;

		bool	abSkillUsable[GUILD_SKILL_COUNT];

		enum GuildJoinErrCode {
			GERR_NONE			= 0,
			GERR_WITHDRAWPENALTY,
			GERR_COMMISSIONPENALTY,		
			GERR_ALREADYJOIN,
			GERR_GUILDISFULL,
			GERR_GUILD_IS_IN_WAR,
			GERR_INVITE_LIMIT,
			GERR_MAX
		};

		GuildJoinErrCode	VerifyGuildJoinableCondition(const LPCHARACTER pchInvitee);

		typedef std::map< uint32_t, LPEVENT >	EventMap;
		EventMap	m_GuildInviteEventMap;
};
