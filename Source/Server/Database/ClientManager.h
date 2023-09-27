#pragma once

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include <Common/stl.h>
#include <Common/building.h>
#include <Core/Tables.hpp>
#include <Core/Constants/Item.hpp>

#include "Peer.h"
#include "DBManager.h"
#include "LoginData.h"

class CPlayerTableCache;
class CItemCache;
class CItemPriceListTableCache;

class CPacketInfo
{
public:
	void Add(int32_t header);
	void Reset();

	std::map<int32_t, int32_t> m_map_info;
};

size_t CreatePlayerSaveQuery(char* pszQuery, size_t querySize, TPlayerTable* pTab);

class CClientManager : public CNetBase, public Singleton<CClientManager>
{
public:
	typedef std::list<std::unique_ptr<CPeer> >			TPeerList;
	typedef boost::unordered_map<uint32_t, std::unique_ptr<CPlayerTableCache> > TPlayerTableCacheMap;

	typedef boost::unordered_map<uint32_t, std::unique_ptr<CItemCache> > TItemCacheMap;
	typedef boost::unordered_set<CItemCache*, boost::hash<CItemCache*> > TItemCacheSet;
	typedef boost::unordered_map<uint32_t, std::unique_ptr<TItemCacheSet> > TItemCacheSetPtrMap;

	typedef boost::unordered_map<uint32_t, std::unique_ptr<CItemPriceListTableCache> > TItemPriceListCacheMap;

	typedef boost::unordered_map<int16_t, uint8_t> TChannelStatusMap;
	typedef std::pair< uint32_t, uint32_t >		TItemPricelistReqInfo;

	class ClientHandleInfo
	{
	public:
		uint32_t	dwHandle;
		uint32_t	account_id;
		uint32_t	player_id;
		uint8_t	account_index;
		char	login[LOGIN_MAX_LEN + 1];
		char	safebox_password[SAFEBOX_PASSWORD_MAX_LEN + 1];
		char	ip[MAX_HOST_LENGTH + 1];

		TAccountTable* pAccountTable;
		TSafeboxTable* pSafebox;

		ClientHandleInfo(uint32_t argHandle, uint32_t dwPID = 0)
		{
		    dwHandle = argHandle;
		    pSafebox = nullptr;
		    pAccountTable = nullptr;
		    player_id = dwPID;
			account_id = 0;
			account_index = 0;
			memset(login, 0, sizeof(login));
			memset(safebox_password, 0, sizeof(safebox_password));
			memset(ip, 0, sizeof(ip));
		};

		ClientHandleInfo(uint32_t argHandle, uint32_t dwPID, uint32_t accountId)
		{
		    dwHandle = argHandle;
		    pSafebox = nullptr;
		    pAccountTable = nullptr;
		    player_id = dwPID;
			account_id = accountId;

			account_index = 0;
			memset(login, 0, sizeof(login));
			memset(safebox_password, 0, sizeof(safebox_password));
			memset(ip, 0, sizeof(ip));
		};

		~ClientHandleInfo()
		{
		    if (pSafebox)
			{
				delete pSafebox;
				pSafebox = nullptr;
			}
		}
	};

public:
	CClientManager();
	~CClientManager();

	bool	Initialize();
	time_t	GetCurrentTime();

	void	MainLoop();
	void	Quit();

	void	SetTablePostfix(const char* c_pszTablePostfix);
	void	SetPlayerIDStart(int32_t iIDStart);
	int32_t	GetPlayerIDStart() { return m_iPlayerIDStart; }

	int32_t	GetPlayerDeleteLevelLimit() { return m_iPlayerDeleteLevelLimit; }

	void	SetChinaEventServer(bool flag) { m_bChinaEventServer = flag; }
	bool	IsChinaEventServer() { return m_bChinaEventServer; }

	uint32_t	GetUserCount();

	void	SendAllGuildSkillRechargePacket();
	void	SendTime();

	CPlayerTableCache*	GetPlayerCache(uint32_t id);
	void			PutPlayerCache(TPlayerTable* pNew);

	void			CreateItemCacheSet(uint32_t dwID);
	TItemCacheSet *		GetItemCacheSet(uint32_t dwID);
	void			FlushItemCacheSet(uint32_t dwID);

	CItemCache *		GetItemCache(uint32_t id);
	void			PutItemCache(TPlayerItem* pNew, bool bSkipQuery = false);
	bool			DeleteItemCache(uint32_t id);

	void			UpdatePlayerCache();
	void			UpdateItemCache();

	CItemPriceListTableCache*	GetItemPriceListCache(uint32_t dwID);
	void			PutItemPriceListCache(const TItemPriceListTable* pItemPriceList);
	void			UpdateItemPriceListCache(void);


	void			SendGuildSkillUsable(uint32_t guild_id, uint32_t dwSkillVnum, bool bUsable);

	void			SetCacheFlushCountLimit(int32_t iLimit);

	template <class Func>
	Func		for_each_peer(Func f);

	LPPEER		GetAnyPeer();

	void			ForwardPacket(uint8_t header, const void* data, int32_t size, uint8_t bChannel = 0, LPPEER except = nullptr);

	void			SendNotice(const char* c_pszFormat, ...);

	char*			GetCommand(char* str);

protected:
	void	Destroy();

private:
	bool		InitializeTables();
	bool		InitializeShopTable();
	bool		InitializeMobTable();
	bool		InitializeItemTable();
	bool		InitializeQuestItemTable();
	bool		InitializeSkillTable();
	bool		InitializeRefineTable();
	bool		InitializeBanwordTable();
	bool		InitializeItemAttrTable();
	bool		InitializeItemRareTable();
	bool		InitializeLandTable();
	bool		InitializeObjectProto();
	bool		InitializeObjectTable();
	bool		InitializeMonarch();

	void		AddPeer(socket_t fd);
	void		RemovePeer(LPPEER pPeer);
	LPPEER		GetPeer(IDENT ident);

	int32_t		AnalyzeQueryResult(SQLMsg* msg);
	int32_t		AnalyzeErrorMsg(LPPEER pPeer, SQLMsg* msg);

	int32_t		Process();

        void            ProcessPackets(LPPEER pPeer);

	CLoginData *	GetLoginData(uint32_t dwKey);
	CLoginData *	GetLoginDataByLogin(const char* c_pszLogin);
	CLoginData *	GetLoginDataByAID(uint32_t dwAID);

	void		InsertLoginData(CLoginData* pLD);
	void		DeleteLoginData(CLoginData* pLD);

	bool		InsertLogonAccount(const char* c_pszLogin, uint32_t dwHandle, const char* c_pszIP);
	bool		DeleteLogonAccount(const char* c_pszLogin, uint32_t dwHandle);
	bool		FindLogonAccount(const char* c_pszLogin);

	void		GuildCreate(LPPEER pPeer, uint32_t dwGuildID);
	void		GuildSkillUpdate(LPPEER pPeer, TPacketGuildSkillUpdate* p);
	void		GuildExpUpdate(LPPEER pPeer, TPacketGuildExpUpdate* p);
	void		GuildAddMember(LPPEER pPeer, TPacketGDGuildAddMember* p);
	void		GuildChangeGrade(LPPEER pPeer, TPacketGuild* p);
	void		GuildRemoveMember(LPPEER pPeer, TPacketGuild* p);
	void		GuildChangeMemberData(LPPEER pPeer, TPacketGuildChangeMemberData* p);
	void		GuildDisband(LPPEER pPeer, TPacketGuild* p);
	void		GuildWar(LPPEER pPeer, TPacketGuildWar* p);
	void		GuildWarScore(LPPEER pPeer, TPacketGuildWarScore* p);
	void		GuildChangeLadderPoint(TPacketGuildLadderPoint* p);
	void		GuildUseSkill(TPacketGuildUseSkill* p);
	void		GuildDepositMoney(TPacketGDGuildMoney* p);
	void		GuildWithdrawMoney(LPPEER pPeer, TPacketGDGuildMoney* p);
	void		GuildWithdrawMoneyGiveReply(TPacketGDGuildMoneyWithdrawGiveReply* p);
	void		GuildWarBet(TPacketGDGuildWarBet* p);
	void		GuildChangeMaster(TPacketChangeGuildMaster* p);

	void		SetGuildWarEndTime(uint32_t guild_id1, uint32_t guild_id2, time_t tEndTime);

	void		QUERY_BOOT(LPPEER pPeer, TPacketGDBoot* p);

	void		QUERY_LOGIN(LPPEER pPeer, uint32_t dwHandle, SLoginPacket* data);
	void		QUERY_LOGOUT(LPPEER pPeer, uint32_t dwHandle, const char*);

	void		RESULT_LOGIN(LPPEER pPeer, SQLMsg *msg);

	void		QUERY_PLAYER_LOAD(LPPEER pPeer, uint32_t dwHandle, TPlayerLoadPacket*);
	void		RESULT_COMPOSITE_PLAYER(LPPEER pPeer, SQLMsg* pMsg, uint32_t dwQID);
	void		RESULT_PLAYER_LOAD(LPPEER pPeer, MYSQL_RES* pRes, ClientHandleInfo* pInfo);
	void		RESULT_ITEM_LOAD(LPPEER pPeer, MYSQL_RES* pRes, uint32_t dwHandle, uint32_t dwPID);
	void		RESULT_QUEST_LOAD(LPPEER pPeer, MYSQL_RES* pRes, uint32_t dwHandle, uint32_t dwPID);
	void		RESULT_AFFECT_LOAD(LPPEER pPeer, MYSQL_RES* pRes, uint32_t dwHandle);

	void		RESULT_PLAYER_INDEX_CREATE(LPPEER pPeer, SQLMsg *msg);

	void		RESULT_PRICELIST_LOAD(LPPEER pPeer, SQLMsg* pMsg);
	void		RESULT_PRICELIST_LOAD_FOR_UPDATE(SQLMsg* pMsg);
	
	void		QUERY_PLAYER_SAVE(LPPEER pPeer, uint32_t dwHandle, TPlayerTable*);

	void		__QUERY_PLAYER_CREATE(LPPEER pPeer, uint32_t dwHandle, TPlayerCreatePacket*);
	void		__QUERY_PLAYER_DELETE(LPPEER pPeer, uint32_t dwHandle, TPlayerDeletePacket*);
	void		__RESULT_PLAYER_DELETE(LPPEER pPeer, SQLMsg* msg);

	void		QUERY_PLAYER_COUNT(LPPEER pPeer, TPlayerCountPacket*);

	void		QUERY_ITEM_SAVE(LPPEER pPeer, const char* c_pData);
	void		QUERY_ITEM_DESTROY(LPPEER pPeer, const char* c_pData);
	void		QUERY_ITEM_FLUSH(LPPEER pPeer, const char* c_pData);


	void		QUERY_QUEST_SAVE(LPPEER pPeer, TQuestTable*, uint32_t dwLen);
	void		QUERY_ADD_AFFECT(LPPEER pPeer, TPacketGDAddAffect* p);
	void		QUERY_REMOVE_AFFECT(LPPEER pPeer, TPacketGDRemoveAffect* p);

	void		QUERY_SAFEBOX_LOAD(LPPEER pPeer, uint32_t dwHandle, TSafeboxLoadPacket* packet, bool bMall);
	void		QUERY_SAFEBOX_SAVE(LPPEER pPeer, TSafeboxTable* pTable);
	void		QUERY_SAFEBOX_CHANGE_SIZE(LPPEER pPeer, uint32_t dwHandle, TSafeboxChangeSizePacket* p);
	void		QUERY_SAFEBOX_CHANGE_PASSWORD(LPPEER pPeer, uint32_t dwHandle, TSafeboxChangePasswordPacket* p);

	void		RESULT_SAFEBOX_LOAD(LPPEER pPeer, SQLMsg* msg);
	void		RESULT_SAFEBOX_CHANGE_SIZE(LPPEER pPeer, SQLMsg* msg);
	void		RESULT_SAFEBOX_CHANGE_PASSWORD(LPPEER pPeer, SQLMsg* msg);
	void		RESULT_SAFEBOX_CHANGE_PASSWORD_SECOND(LPPEER pPeer, SQLMsg* msg);

	void		QUERY_EMPIRE_SELECT(LPPEER pPeer, uint32_t dwHandle, TEmpireSelectPacket* p);
	void		QUERY_SETUP(LPPEER pPeer, uint32_t dwHandle, const char* c_pData);

	void		SendPartyOnSetup(LPPEER pPeer);

	void		QUERY_HIGHSCORE_REGISTER(LPPEER pPeer, TPacketGDHighscore* data);
	void		RESULT_HIGHSCORE_REGISTER(LPPEER pPeer, SQLMsg* msg);

	void		QUERY_FLUSH_CACHE(LPPEER pPeer, const char* c_pData);

	void		QUERY_PARTY_CREATE(LPPEER pPeer, TPacketPartyCreate* p);
	void		QUERY_PARTY_DELETE(LPPEER pPeer, TPacketPartyDelete* p);
	void		QUERY_PARTY_ADD(LPPEER pPeer, TPacketPartyAdd* p);
	void		QUERY_PARTY_REMOVE(LPPEER pPeer, TPacketPartyRemove* p);
	void		QUERY_PARTY_STATE_CHANGE(LPPEER pPeer, TPacketPartyStateChange* p);
	void		QUERY_PARTY_SET_MEMBER_LEVEL(LPPEER pPeer, TPacketPartySetMemberLevel* p);

	void		QUERY_RELOAD_PROTO();

	void		QUERY_CHANGE_NAME(LPPEER pPeer, uint32_t dwHandle, TPacketGDChangeName* p);
	void		GetPlayerFromRes(TPlayerTable* player_table, MYSQL_RES* res);

	void		QUERY_LOGIN_KEY(LPPEER pPeer, TPacketGDLoginKey* p);

	void		AddGuildPriv(TPacketGiveGuildPriv* p);
	void		AddEmpirePriv(TPacketGiveEmpirePriv* p);
	void		AddCharacterPriv(TPacketGiveCharacterPriv* p);

	void		MoneyLog(TPacketMoneyLog* p);

	void		QUERY_AUTH_LOGIN(LPPEER pPeer, uint32_t dwHandle, TPacketGDAuthLogin* p);

	void		QUERY_LOGIN_BY_KEY(LPPEER pPeer, uint32_t dwHandle, TPacketGDLoginByKey* p);
	void		RESULT_LOGIN_BY_KEY(LPPEER pPeer, SQLMsg* msg);

	void		ChargeCash(const TRequestChargeCash* p);

	void		LoadEventFlag();
	void		SetEventFlag(TPacketSetEventFlag* p);
	void		SendEventFlagsOnSetup(LPPEER pPeer);

	void		MarriageAdd(TPacketMarriageAdd* p);
	void		MarriageUpdate(TPacketMarriageUpdate* p);
	void		MarriageRemove(TPacketMarriageRemove* p);

	void		WeddingRequest(TPacketWeddingRequest* p);
	void		WeddingReady(TPacketWeddingReady* p);
	void		WeddingEnd(TPacketWeddingEnd* p);

	void		MyshopPricelistUpdate(const TPacketMyshopPricelistHeader* pPacket);
	void		MyshopPricelistRequest(LPPEER pPeer, uint32_t dwHandle, uint32_t dwPlayerID);

	void		CreateObject(TPacketGDCreateObject* p);
	void		DeleteObject(uint32_t dwID);
	void		UpdateLand(uint32_t* pdw);

	void		BlockChat(TPacketBlockChat* p);
   
private:
	int32_t						m_bLooping;
	socket_t				m_fdAccept;
	TPeerList				m_peerList;

	LPPEER					m_pAuthPeer;

	typedef boost::unordered_map<uint32_t, CLoginData *> TLoginDataByLoginKey;
	TLoginDataByLoginKey			m_map_pLoginData;

	typedef boost::unordered_map<std::string, CLoginData *> TLoginDataByLogin;
	TLoginDataByLogin			m_map_pLoginDataByLogin;

	typedef boost::unordered_map<uint32_t, CLoginData *> TLoginDataByAID;
	TLoginDataByAID				m_map_pLoginDataByAID;

	typedef boost::unordered_map<std::string, CLoginData *> TLogonAccountMap;
	TLogonAccountMap			m_map_kLogonAccount;

	int32_t					m_iPlayerIDStart;
	int32_t					m_iPlayerDeleteLevelLimit;
	int32_t					m_iPlayerDeleteLevelLimitLower;
	bool					m_bChinaEventServer;

	std::vector<TMobTable>			m_vec_mobTable;
	std::vector<TItemTable>			m_vec_itemTable;
	std::map<uint32_t, TItemTable *>		m_map_itemTableByVnum;

	int32_t					m_iShopTableSize;
	TShopTable *				m_pShopTable;

	int32_t					m_iRefineTableSize;
	TRefineTable*				m_pRefineTable;

	std::vector<TSkillTable>		m_vec_skillTable;
	std::vector<TBanwordTable>		m_vec_banwordTable;
	std::vector<TItemAttrTable>		m_vec_itemAttrTable;
	std::vector<TItemAttrTable>		m_vec_itemRareTable;

	std::vector<building::TLand>		m_vec_kLandTable;
	std::vector<building::TObjectProto>	m_vec_kObjectProto;
	std::map<uint32_t, building::TObject *>	m_map_pObjectTable;

	std::queue<TPacketGDVCard>		m_queue_vcard;

	bool					m_bShutdowned;

	TPlayerTableCacheMap			m_map_playerCache;

	TItemCacheMap				m_map_itemCache;
	TItemCacheSetPtrMap			m_map_pItemCacheSetPtr;

	TItemPriceListCacheMap m_mapItemPriceListCache;

	TChannelStatusMap m_mChannelStatus;

	struct TPartyInfo
	{
	    uint8_t bRole;
	    uint8_t bLevel;

		TPartyInfo() :bRole(0), bLevel(0)
		{
		}
	};

	typedef std::map<uint32_t, TPartyInfo>	TPartyMember;
	typedef std::map<uint32_t, TPartyMember>	TPartyMap;
	typedef std::map<uint8_t, TPartyMap>	TPartyChannelMap;
	TPartyChannelMap m_map_pChannelParty;

	typedef std::map<std::string, int32_t>	TEventFlagMap;
	TEventFlagMap m_map_lEventFlag;

	uint8_t					m_bLastHeader;
	int32_t					m_iCacheFlushCount;
	int32_t					m_iCacheFlushCountLimit;

    private :
	TItemIDRangeTable m_itemRange;

    public :
	bool InitializeNowItemID();
	uint32_t GetItemID();
	uint32_t GainItemID();
	TItemIDRangeTable GetItemRange() { return m_itemRange; }

public:
	bool InitializeLocalization(); 

private:
	std::vector<tLocale> m_vec_Locale;

	bool __GetAdminInfo(const char* szIP, std::vector<tAdminInfo>& rAdminVec);
	bool __GetHostInfo(std::vector<std::string>& rIPVec);

	void ReloadAdmin(LPPEER pPeer, TPacketReloadAdmin* p);
	void BreakMarriage(LPPEER pPeer, const char* data);

	struct TLogoutPlayer
	{
	    uint32_t	pid;
	    time_t	time;

	    bool operator < (const TLogoutPlayer& r) 
	    {
		return (pid < r.pid);
	    }
	};

	typedef boost::unordered_map<uint32_t, TLogoutPlayer*> TLogoutPlayerMap;
	TLogoutPlayerMap m_map_logout;
	
	void InsertLogoutPlayer(uint32_t pid);
	void DeleteLogoutPlayer(uint32_t pid);
	void UpdateLogoutPlayer();
	void UpdateItemCacheSet(uint32_t pid);
	void FlushPlayerCacheSet(uint32_t pid);

	void Election(LPPEER pPeer, uint32_t dwHandle, const char* p);
	void Candidacy(LPPEER pPeer, uint32_t dwHandle, const char* p);
	void AddMonarchMoney(LPPEER pPeer, uint32_t dwHandle, const char* p);
	void TakeMonarchMoney(LPPEER pPeer, uint32_t dwHandle, const char* p);
	void ComeToVote(LPPEER pPeer, uint32_t dwHandle, const char* p);
	void RMCandidacy(LPPEER pPeer, uint32_t dwHandle, const char* p);
	void SetMonarch(LPPEER pPeer, uint32_t dwHandle, const char* p);
	void RMMonarch(LPPEER pPeer, uint32_t dwHandle, const char* p);
	void DecMonarchMoney(LPPEER pPeer, uint32_t dwHandle, const char* p);

	void ChangeMonarchLord(LPPEER pPeer, uint32_t dwHandle, TPacketChangeMonarchLord* info);
	void SendSpareItemIDRange(LPPEER pPeer);

	void UpdateHorseName(TPacketUpdateHorseName* data, LPPEER pPeer);
	void AckHorseName(uint32_t dwPID, LPPEER pPeer);
	void DeleteLoginKey(TPacketDC *data);
	void ResetLastPlayerID(const TPacketNeedLoginLogInfo* data);

	void UpdateChannelStatus(TChannelStatus* pData);
	void RequestChannelStatus(LPPEER pPeer, uint32_t dwHandle);
};

template<class Func>	
Func CClientManager::for_each_peer(Func f)
{
	for (const auto& upPeer : m_peerList)
	{
		LPPEER pPeer = upPeer.get();

		f(pPeer);
    }

    return f;
}
