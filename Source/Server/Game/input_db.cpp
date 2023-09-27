#include "stdafx.h" 
#include <Core/Net/PacketsCG.hpp>
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "packet.h"
#include "protocol.h"
#include "mob_manager.h"
#include "shop_manager.h"
#include "sectree_manager.h"
#include "skill.h"
#include "questmanager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "start_position.h"
#include "party.h"
#include "refine.h"
#include "banword.h"
#include "priv_manager.h"
#include "db.h"
#include "building.h"
#include "login_sim.h"
#include "wedding.h"
#include "login_data.h"
#include "unique_item.h"

#include "monarch.h"
#include "affect.h"
#include "castle.h"
#include "motion.h"
#include "log.h"
#include "horsename_manager.h"
#include "gm.h"
#include "map_location.h"
#include "DragonSoul.h"

extern bool		g_bAuthServer;

extern void gm_insert(const char* name, uint8_t level);
extern uint8_t	gm_get_level(const char* name, const char* host, const char* account);
extern void gm_host_insert(const char* host);
extern int32_t openid_server;

#define MAPNAME_DEFAULT	"none"

bool GetServerLocation(TAccountTable& rTab, uint8_t bEmpire)
{
	bool bFound = false;

	for (int32_t i = 0; i < PLAYER_PER_ACCOUNT; ++i)
	{
		if (0 == rTab.players[i].dwID)
			continue;

		bFound = true;
		int32_t lIndex = 0;

		if (!CMapLocation::GetInstance()->Get(rTab.players[i].x,
					rTab.players[i].y,
					lIndex,
					rTab.players[i].lAddr,
					rTab.players[i].wPort))
		{
			SysLog("location error name {} mapindex {} {} x {} empire {}",
					rTab.players[i].szName, lIndex, rTab.players[i].x, rTab.players[i].y, rTab.bEmpire);

			rTab.players[i].x = EMPIRE_START_X(rTab.bEmpire);
			rTab.players[i].y = EMPIRE_START_Y(rTab.bEmpire);

			lIndex = 0;

			if (!CMapLocation::GetInstance()->Get(rTab.players[i].x, rTab.players[i].y, lIndex, rTab.players[i].lAddr, rTab.players[i].wPort))
			{
				SysLog("cannot find server for mapindex {} {} x {} (name {})", 
						lIndex,
						rTab.players[i].x,
						rTab.players[i].y,
						rTab.players[i].szName);

				continue;
			}
		}

		struct in_addr in;
		in.s_addr = rTab.players[i].lAddr;
		PyLog("success to {}:{}", inet_ntoa(in), rTab.players[i].wPort);
	}

	return bFound;
}

extern std::map<uint32_t, CLoginSim *> g_sim;
extern std::map<uint32_t, CLoginSim *> g_simByPID;

void CInputDB::LoginSuccess(uint32_t dwHandle, const char* data)
{
	PyLog("LoginSuccess");

	TAccountTable* pTab = (TAccountTable *) data;

	auto it = g_sim.find(pTab->id);
	if (g_sim.end() != it)
	{
		PyLog("CInputDB::LoginSuccess - already exist sim [{}]", pTab->login);
		it->second->SendLoad();
		return;
	}

	LPDESC d = DESC_MANAGER::GetInstance()->FindByHandle(dwHandle);

	if (!d)
	{
		PyLog("CInputDB::LoginSuccess - cannot find handle [{}]", pTab->login);

		TLogoutPacket pack;

		strlcpy(pack.login, pTab->login, sizeof(pack.login));
		db_clientdesc->DBPacket(HEADER_GD_LOGOUT, dwHandle, &pack, sizeof(pack));
		return;
	}

	if (strcmp(pTab->status, "OK"))
	{
		PyLog("CInputDB::LoginSuccess - status[{}] is not OK [{}]", pTab->status, pTab->login);

		TLogoutPacket pack;

		strlcpy(pack.login, pTab->login, sizeof(pack.login));
		db_clientdesc->DBPacket(HEADER_GD_LOGOUT, dwHandle, &pack, sizeof(pack));

		LoginFailure(d, pTab->status);
		return;
	}

	for (int32_t i = 0; i != PLAYER_PER_ACCOUNT; ++i)
	{
		TSimplePlayer& player = pTab->players[i];
		PyLog("\tplayer({}).job({})", player.szName, player.byJob);
	}

	bool bFound = GetServerLocation(*pTab, pTab->bEmpire);

	d->BindAccountTable(pTab);

	if (!bFound)
	{
		TPacketGCEmpire pe;
		pe.bHeader = HEADER_GC_EMPIRE;
		pe.bEmpire = number(1, 3);
		d->Packet(&pe, sizeof(pe));
	}
	else
	{
		TPacketGCEmpire pe;
		pe.bHeader = HEADER_GC_EMPIRE;
		pe.bEmpire = d->GetEmpire();
		d->Packet(&pe, sizeof(pe));
	}

	d->SetPhase(PHASE_SELECT);
	d->SendLoginSuccessPacket();

	PyLog("InputDB::login_success: {}", pTab->login);
}

void CInputDB::PlayerCreateFailure(LPDESC d, uint8_t bType)
{
	if (!d)
		return;

	TPacketGCCreateFailure pack;

	pack.header	= HEADER_GC_CHARACTER_CREATE_FAILURE;
	pack.bType	= bType;

	d->Packet(&pack, sizeof(pack));
}

void CInputDB::PlayerCreateSuccess(LPDESC d, const char* data)
{
	if (!d)
		return;

	TPacketDGCreateSuccess* pPacketDB = (TPacketDGCreateSuccess *) data;

	if (pPacketDB->bAccountCharacterIndex >= PLAYER_PER_ACCOUNT)
	{
		d->Packet(encode_byte(HEADER_GC_CHARACTER_CREATE_FAILURE), 1);
		return;
	}

	int32_t lIndex = 0;

	if (!CMapLocation::GetInstance()->Get(pPacketDB->player.x,
				pPacketDB->player.y,
				lIndex,
				pPacketDB->player.lAddr,
				pPacketDB->player.wPort))
	{
		SysLog("InputDB::PlayerCreateSuccess: cannot find server for mapindex {} {} x {} (name {})", 
				lIndex,
				pPacketDB->player.x,
				pPacketDB->player.y,
				pPacketDB->player.szName);
	}

	TAccountTable& r_Tab = d->GetAccountTable();
	r_Tab.players[pPacketDB->bAccountCharacterIndex] = pPacketDB->player;

	TPacketGCPlayerCreateSuccess pack;

	pack.header = HEADER_GC_CHARACTER_CREATE_SUCCESS;
	pack.bAccountCharacterIndex = pPacketDB->bAccountCharacterIndex;
	pack.player = pPacketDB->player;

	d->Packet(&pack, sizeof(TPacketGCPlayerCreateSuccess));

	TPlayerItem t;
	memset(&t, 0, sizeof(t));

	if (china_event_server)
	{
		t.window	= INVENTORY;
		t.count	= 1;
		t.owner	= r_Tab.players[pPacketDB->bAccountCharacterIndex].dwID;

		struct SInitialItem
		{
			uint32_t dwVnum;
			uint8_t pos;
		};

		const int32_t MAX_INITIAL_ITEM = 9;

		static SInitialItem initialItems[JOB_MAX_NUM][MAX_INITIAL_ITEM] = 
		{
			{ {11243,	2}, {12223,	3}, {15103,	4}, {   93,	1}, {16143,	8}, {17103,	9}, { 3083,	0}, {13193,	11}, {14103, 12}, },
			{ {11443,	0}, {12363,	3}, {15103,	4}, { 1053,	2}, { 2083,	1}, {16083,	7}, {17083,	8}, {13193,	9}, {14103,	10}, },
			{ {11643,	0}, {12503,	2}, {15103,	3}, {   93,	1}, {16123,	4}, {17143,	7}, {13193,	8}, {14103,	9}, {    0,	0}, },
			{ {11843,	0}, {12643,	1}, {15103,	2}, { 7083,	3}, { 5053,	4}, {16123,	6}, {17143,	7}, {13193,	8}, {14103,	9}, },
		};

		int32_t job = pPacketDB->player.byJob;
		for (int32_t i=0; i < MAX_INITIAL_ITEM; i++)
		{
			if (initialItems[job][i].dwVnum == 0)
				continue;

			t.id	= ITEM_MANAGER::GetInstance()->GetNewID();
			t.pos	= initialItems[job][i].pos;
			t.vnum	= initialItems[job][i].dwVnum;

			db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_SAVE, 0, sizeof(TPlayerItem));
			db_clientdesc->Packet(&t, sizeof(TPlayerItem));
		}
	}

	LogManager::GetInstance()->CharLog(pack.player.dwID, 0, 0, 0, "CREATE PLAYER", "", d->GetHostName());
}

void CInputDB::PlayerDeleteSuccess(LPDESC d, const char* data)
{
	if (!d)
		return;

	uint8_t account_index;
	account_index = decode_byte(data);
	d->BufferedPacket(encode_byte(HEADER_GC_CHARACTER_DELETE_SUCCESS),	1);
	d->Packet(encode_byte(account_index),			1);

	d->GetAccountTable().players[account_index].dwID = 0;
}

void CInputDB::PlayerDeleteFail(LPDESC d)
{
	if (!d)
		return;

	d->Packet(encode_byte(HEADER_GC_CHARACTER_DELETE_WRONG_SOCIAL_ID),	1);
}

void CInputDB::ChangeName(LPDESC d, const char* data)
{
	if (!d)
		return;

	TPacketDGChangeName* p = (TPacketDGChangeName *) data;

	TAccountTable& r = d->GetAccountTable();

	if (!r.id)
		return;

	for (size_t i = 0; i < PLAYER_PER_ACCOUNT; ++i)
		if (r.players[i].dwID == p->pid)
		{
			strlcpy(r.players[i].szName, p->name, sizeof(r.players[i].szName));
			r.players[i].bChangeName = 0;

			TPacketGCChangeName pgc;

			pgc.header = HEADER_GC_CHANGE_NAME;
			pgc.pid = p->pid;
			strlcpy(pgc.name, p->name, sizeof(pgc.name));

			d->Packet(&pgc, sizeof(TPacketGCChangeName));
			break;
		}
}

void CInputDB::PlayerLoad(LPDESC d, const char* data)
{
	TPlayerTable* pTab = (TPlayerTable *) data;

	if (!d)
		return;

	int32_t lMapIndex = pTab->lMapIndex;
	PIXEL_POSITION pos;

	if (lMapIndex == 0)
	{
		lMapIndex = SECTREE_MANAGER::GetInstance()->GetMapIndex(pTab->x, pTab->y);

		if (lMapIndex == 0)
		{
			lMapIndex = EMPIRE_START_MAP(d->GetAccountTable().bEmpire);
			pos.x = EMPIRE_START_X(d->GetAccountTable().bEmpire);
			pos.y = EMPIRE_START_Y(d->GetAccountTable().bEmpire);
		}
		else
		{
			pos.x = pTab->x;
			pos.y = pTab->y;
		}
	}
	pTab->lMapIndex = lMapIndex;

	// If you were in a private map and the private map is gone, you have to go back to the exit.
	// ----
	// But if you say you have to go back to the exit... Why are you looking for the location of the pulic map corresponding to the private map instead of the exit...
	// Since you don't know the history... Hard-coding again.
	// If it is an anglerfish cave, go to the exit...
	// by rtsummit
	if (!SECTREE_MANAGER::GetInstance()->GetValidLocation(pTab->lMapIndex, pTab->x, pTab->y, lMapIndex, pos, d->GetEmpire()))
	{
		SysLog("InputDB::PlayerLoad : cannot find valid location {} x {} (name: {})", pTab->x, pTab->y, pTab->name);
		d->SetPhase(PHASE_CLOSE);
		return;
	}

	pTab->x = pos.x;
	pTab->y = pos.y;
	pTab->lMapIndex = lMapIndex;

	if (d->GetCharacter() || d->IsPhase(PHASE_GAME))
	{
		LPCHARACTER p = d->GetCharacter();
		SysLog("login state already has main state (character {})", p->GetName());
		return;
	}

	if (NULL != CHARACTER_MANAGER::GetInstance()->FindPC(pTab->name))
	{
		SysLog("InputDB: PlayerLoad : {} already exist in game", pTab->name);
		return;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->CreateCharacter(pTab->name, pTab->id);

	ch->BindDesc(d);
	ch->SetPlayerProto(pTab);
	ch->SetEmpire(d->GetEmpire());

	d->BindCharacter(ch);
	
	{
		// P2P Login
		TPacketGGLogin p;

		p.bHeader = HEADER_GG_LOGIN;
		strlcpy(p.szName, ch->GetName(), sizeof(p.szName));
		p.dwPID = ch->GetPlayerID();
		p.bEmpire = ch->GetEmpire();
		p.lMapIndex = SECTREE_MANAGER::GetInstance()->GetMapIndex(ch->GetX(), ch->GetY());
		p.bChannel = g_bChannel;

		P2P_MANAGER::GetInstance()->Send(&p, sizeof(TPacketGGLogin));

		char buf[51];
		snprintf(buf, sizeof(buf), "%s %d %d %ld %d", 
				inet_ntoa(ch->GetDesc()->GetAddr().sin_addr), ch->GetGold(), g_bChannel, ch->GetMapIndex(), ch->GetAlignment());
		LogManager::GetInstance()->CharLog(ch, 0, "LOGIN", buf);

		LogManager::GetInstance()->LoginLog(true, 
				ch->GetDesc()->GetAccountTable().id, ch->GetPlayerID(), ch->GetLevel(), ch->GetJob(), ch->GetRealPoint(POINT_PLAYTIME));
	}

	d->SetPhase(PHASE_LOADING);
	ch->MainCharacterPacket();

	int32_t lPublicMapIndex = lMapIndex >= 10000 ? lMapIndex / 10000 : lMapIndex;

	if (!map_allow_find(lPublicMapIndex))
	{
		SysLog("InputDB::PlayerLoad : entering {} map is not allowed here (name: {}, empire {})", 
				lMapIndex, pTab->name, d->GetEmpire());

		ch->SetWarpLocation(EMPIRE_START_MAP(d->GetEmpire()),
				EMPIRE_START_X(d->GetEmpire()) / 100,
				EMPIRE_START_Y(d->GetEmpire()) / 100);

		d->SetPhase(PHASE_CLOSE);
		return;
	}

	quest::CQuestManager::GetInstance()->BroadcastEventFlagOnLogin(ch);

	for (int32_t i = 0; i < QUICKSLOT_MAX_NUM; ++i)
		ch->SetQuickslot(i, pTab->quickslot[i]);

	ch->PointsPacket();
	ch->SkillLevelPacket();

	PyLog("InputDB: player_load {} {}x{}x{} LEVEL {} MOV_SPEED {} JOB {} ATG {} DFG {} GMLv {}",
			pTab->name, 
			ch->GetX(), ch->GetY(), ch->GetZ(),
			ch->GetLevel(),
			ch->GetPoint(POINT_MOV_SPEED),
			ch->GetJob(),
			ch->GetPoint(POINT_ATT_GRADE),
			ch->GetPoint(POINT_DEF_GRADE),
			ch->GetGMLevel());

	ch->QuerySafeboxSize();
}

void CInputDB::Boot(const char* data)
{
	signal_timer_disable();

	uint32_t dwPacketSize = decode_4bytes(data);
	data += 4;

	uint8_t bVersion = decode_byte(data);
	data += 1;

	PyLog("BOOT: PACKET: {}", dwPacketSize);
	PyLog("BOOT: VERSION: {}", bVersion);
	if (bVersion != 6)
	{
		SysLog("boot version error");
		thecore_shutdown();
	}

	PyLog("sizeof(TMobTable) = {}", sizeof(TMobTable));
	PyLog("sizeof(TItemTable) = {}", sizeof(TItemTable));
	PyLog("sizeof(TShopTable) = {}", sizeof(TShopTable));
	PyLog("sizeof(TSkillTable) = {}", sizeof(TSkillTable));
	PyLog("sizeof(TRefineTable) = {}", sizeof(TRefineTable));
	PyLog("sizeof(TItemAttrTable) = {}", sizeof(TItemAttrTable));
	PyLog("sizeof(TItemRareTable) = {}", sizeof(TItemAttrTable));
	PyLog("sizeof(TBanwordTable) = {}", sizeof(TBanwordTable));
	PyLog("sizeof(TLand) = {}", sizeof(building::TLand));
	PyLog("sizeof(TObjectProto) = {}", sizeof(building::TObjectProto));
	PyLog("sizeof(TObject) = {}", sizeof(building::TObject));
	PyLog("sizeof(TAdminManager) = {}", sizeof (TAdminInfo));
	
	uint16_t size;

	/*
	 * MOB
	 */

	if (decode_2bytes(data)!=sizeof(TMobTable))
	{
		SysLog("mob table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	PyLog("BOOT: MOB: {}", size);

	if (size)
	{
		CMobManager::GetInstance()->Initialize((TMobTable *) data, size);
		data += size * sizeof(TMobTable);
	}

	/*
	 * ITEM
	 */

	if (decode_2bytes(data) != sizeof(TItemTable))
	{
		SysLog("item table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	PyLog("BOOT: ITEM: {}", size);


	if (size)
	{
		ITEM_MANAGER::GetInstance()->Initialize((TItemTable *) data, size);
		data += size * sizeof(TItemTable);
	}

	/*
	 * SHOP
	 */

	if (decode_2bytes(data) != sizeof(TShopTable))
	{
		SysLog("shop table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	PyLog("BOOT: SHOP: {}", size);


	if (size)
	{
		if (!CShopManager::GetInstance()->Initialize((TShopTable *) data, size))
		{
			SysLog("shop table Initialize error");
			thecore_shutdown();
			return;
		}
		data += size * sizeof(TShopTable);
	}

	/*
	 * SKILL
	 */

	if (decode_2bytes(data) != sizeof(TSkillTable))
	{
		SysLog("skill table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	PyLog("BOOT: SKILL: {}", size);

	if (size)
	{
		if (!CSkillManager::GetInstance()->Initialize((TSkillTable *) data, size))
		{
			SysLog("cannot initialize skill table");
			thecore_shutdown();
			return;
		}

		data += size * sizeof(TSkillTable);
	}
	/*
	 * REFINE RECIPE
	 */
	if (decode_2bytes(data) != sizeof(TRefineTable))
	{
		SysLog("refine table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	PyLog("BOOT: REFINE: {}", size);

	if (size)
	{
		CRefineManager::GetInstance()->Initialize((TRefineTable*) data, size);
		data += size * sizeof(TRefineTable);
	}

	/*
	 * ITEM ATTR
	 */
	if (decode_2bytes(data) != sizeof(TItemAttrTable))
	{
		SysLog("item attr table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	PyLog("BOOT: ITEM_ATTR: {}", size);

	if (size)
	{
		TItemAttrTable* p = (TItemAttrTable *) data;

		for (int32_t i = 0; i < size; ++i, ++p)
		{
			if (p->dwApplyIndex >= ITEM::MAX_APPLY_NUM)
				continue;

			g_map_itemAttr[p->dwApplyIndex] =* p;
			PyLog("ITEM_ATTR[{}]: {} {}", p->dwApplyIndex, p->szApply, p->dwProb);
		}
	}

	data += size * sizeof(TItemAttrTable);


	/*
     * ITEM RARE
     */
	if (decode_2bytes(data) != sizeof(TItemAttrTable))
	{
		SysLog("item rare table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;
	PyLog("BOOT: ITEM_RARE: {}", size);

	if (size)
	{
		TItemAttrTable* p = (TItemAttrTable *) data;

		for (int32_t i = 0; i < size; ++i, ++p)
		{
			if (p->dwApplyIndex >= ITEM::MAX_APPLY_NUM)
				continue;

			g_map_itemRare[p->dwApplyIndex] =* p;
			PyLog("ITEM_RARE[{}]: {} {}", p->dwApplyIndex, p->szApply, p->dwProb);
		}
	}

	data += size * sizeof(TItemAttrTable);


	/*
	 * BANWORDS
	 */

	if (decode_2bytes(data) != sizeof(TBanwordTable))
	{
		SysLog("ban uint16_t table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;

	CBanwordManager::GetInstance()->Initialize((TBanwordTable *) data, size);
	data += size * sizeof(TBanwordTable);

	{
		using namespace building;

		/*
		 * LANDS
		 */

		if (decode_2bytes(data) != sizeof(TLand))
		{
			SysLog("land table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;

		TLand * kLand = (TLand *) data;
		data += size * sizeof(TLand);

		for (uint16_t i = 0; i < size; ++i, ++kLand)
			CManager::GetInstance()->LoadLand(kLand);

		/*
		 * OBJECT PROTO
		 */

		if (decode_2bytes(data) != sizeof(TObjectProto))
		{
			SysLog("object proto table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;

		CManager::GetInstance()->LoadObjectProto((TObjectProto *) data, size);
		data += size * sizeof(TObjectProto);

		/*
		 * OBJECT 
		 */
		if (decode_2bytes(data) != sizeof(TObject))
		{
			SysLog("object table size error");
			thecore_shutdown();
			return;
		}
		data += 2;

		size = decode_2bytes(data);
		data += 2;

		TObject * kObj = (TObject *) data;
		data += size * sizeof(TObject);

		for (uint16_t i = 0; i < size; ++i, ++kObj)
			CManager::GetInstance()->LoadObject(kObj, true);
	}

	set_global_time(*(time_t*) data);
	data += sizeof(time_t);

	if (decode_2bytes(data) != sizeof(TItemIDRangeTable))
	{
		SysLog("ITEM ID RANGE size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;

	TItemIDRangeTable* range = (TItemIDRangeTable*) data;
	data += size * sizeof(TItemIDRangeTable);

	TItemIDRangeTable* rangespare = (TItemIDRangeTable*) data;
	data += size * sizeof(TItemIDRangeTable);

	int32_t ChunkSize = decode_2bytes(data);
	data += 2;
	int32_t HostSize = decode_2bytes(data);
	data += 2;
	PyLog("GM Value Count {} {}", HostSize, ChunkSize);
	for (int32_t n = 0; n < HostSize; ++n)
	{
		gm_new_host_inert(data);
		PyLog("GM HOST : IP[{}] ", data);		
		data += ChunkSize;
	}
	
	
	data += 2;
	int32_t adminsize = decode_2bytes(data);
	data += 2;

	for (int32_t n = 0; n < adminsize; ++n)
	{
		tAdminInfo& rAdminInfo = *(tAdminInfo*)data;

		gm_new_insert(rAdminInfo);
		
		data += sizeof(rAdminInfo);
	}
	
	//END_ADMIN_MANAGER
		
	//MONARCH
	data += 2;
	data += 2;

	TMonarchInfo& p = *(TMonarchInfo *) data;
	data += sizeof(TMonarchInfo);

	CMonarch::GetInstance()->SetMonarchInfo(&p);

	for (int32_t n = 1; n < 4; ++n)
	{
		if (p.name[n] &&* p.name[n])
			PyLog("[MONARCH] Empire {} Pid {} Money {} {}", n, p.pid[n], p.money[n], p.name[n]);
	}
	
	int32_t CandidacySize = decode_2bytes(data);
	data += 2;

	int32_t CandidacyCount = decode_2bytes(data);
	data += 2;

	if (test_server)
		PyLog("[MONARCH] Size {} Count {}", CandidacySize, CandidacyCount);

	data += CandidacySize * CandidacyCount;

	
	//END_MONARCH

	uint16_t endCheck=decode_2bytes(data);
	if (endCheck != 0xffff)
	{
		SysLog("boot packet end check error [x{}]!=0xffff", endCheck);
		thecore_shutdown();
		return;
	}
	else
		PyLog("boot packet end check ok [x{}]==0xffff", endCheck);
	data +=2; 

	if (!ITEM_MANAGER::GetInstance()->SetMaxItemID(*range))
	{
		SysLog("not enough item id contact your administrator!");
		thecore_shutdown();
		return;
	}

	if (!ITEM_MANAGER::GetInstance()->SetMaxSpareItemID(*rangespare))
	{
		SysLog("not enough item id for spare contact your administrator!");
		thecore_shutdown();
		return;
	}



	// LOCALE_SERVICE
	const int32_t FILE_NAME_LEN = 256;
	char szCommonDropItemFileName[FILE_NAME_LEN];
	char szETCDropItemFileName[FILE_NAME_LEN];
	char szMOBDropItemFileName[FILE_NAME_LEN];
	char szDropItemGroupFileName[FILE_NAME_LEN];
	char szSpecialItemGroupFileName[FILE_NAME_LEN];
	char szMapIndexFileName[FILE_NAME_LEN];
	char szItemVnumMaskTableFileName[FILE_NAME_LEN];
	char szDragonSoulTableFileName[FILE_NAME_LEN];

	snprintf(szCommonDropItemFileName, sizeof(szCommonDropItemFileName),
			"%s/common_drop_item.txt", LocaleService_GetBasePath().c_str());
	snprintf(szETCDropItemFileName, sizeof(szETCDropItemFileName),
			"%s/etc_drop_item.txt", LocaleService_GetBasePath().c_str());
	snprintf(szMOBDropItemFileName, sizeof(szMOBDropItemFileName),
			"%s/mob_drop_item.txt", LocaleService_GetBasePath().c_str());
	snprintf(szSpecialItemGroupFileName, sizeof(szSpecialItemGroupFileName),
			"%s/special_item_group.txt", LocaleService_GetBasePath().c_str());
	snprintf(szDropItemGroupFileName, sizeof(szDropItemGroupFileName),
			"%s/drop_item_group.txt", LocaleService_GetBasePath().c_str());
	snprintf(szMapIndexFileName, sizeof(szMapIndexFileName),
			"%s/index", LocaleService_GetMapPath().c_str());
	snprintf(szItemVnumMaskTableFileName, sizeof(szItemVnumMaskTableFileName),
			"%s/ori_to_new_table.txt", LocaleService_GetBasePath().c_str());
	snprintf(szDragonSoulTableFileName, sizeof(szDragonSoulTableFileName),
			"%s/dragon_soul_table.txt", LocaleService_GetBasePath().c_str());

	PyLog("Initializing Informations of Cube System");
	if (!Cube_InformationInitialize())
	{
		SysLog("cannot init cube infomation.");
		thecore_shutdown();
		return;
	}

	PyLog("LoadLocaleFile: CommonDropItem: {}", szCommonDropItemFileName);
	if (!ITEM_MANAGER::GetInstance()->ReadCommonDropItemFile(szCommonDropItemFileName))
	{
		SysLog("cannot load CommonDropItem: {}", szCommonDropItemFileName);
		thecore_shutdown();
		return;
	}

	PyLog("LoadLocaleFile: ETCDropItem: {}", szETCDropItemFileName);
	if (!ITEM_MANAGER::GetInstance()->ReadEtcDropItemFile(szETCDropItemFileName))
	{
		SysLog("cannot load ETCDropItem: {}", szETCDropItemFileName);
		thecore_shutdown();
		return;
	}

	PyLog("LoadLocaleFile: DropItemGroup: {}", szDropItemGroupFileName);
	if (!ITEM_MANAGER::GetInstance()->ReadDropItemGroup(szDropItemGroupFileName))
	{
		SysLog("cannot load DropItemGroup: {}", szDropItemGroupFileName);
		thecore_shutdown();
		return;
	}

	PyLog("LoadLocaleFile: SpecialItemGroup: {}", szSpecialItemGroupFileName);
	if (!ITEM_MANAGER::GetInstance()->ReadSpecialDropItemFile(szSpecialItemGroupFileName))
	{
		SysLog("cannot load SpecialItemGroup: {}", szSpecialItemGroupFileName);
		thecore_shutdown();
		return;
	}

	PyLog("LoadLocaleFile: ItemVnumMaskTable : {}", szItemVnumMaskTableFileName);
	if (!ITEM_MANAGER::GetInstance()->ReadItemVnumMaskTable(szItemVnumMaskTableFileName))
	{
		PyLog("Could not open MaskItemTable");
	}

	PyLog("LoadLocaleFile: MOBDropItemFile: {}", szMOBDropItemFileName);
	if (!ITEM_MANAGER::GetInstance()->ReadMonsterDropItemGroup(szMOBDropItemFileName))
	{
		SysLog("cannot load MOBDropItemFile: {}", szMOBDropItemFileName);
		thecore_shutdown();
		return;
	}

	PyLog("LoadLocaleFile: MapIndex: {}", szMapIndexFileName);
	if (!SECTREE_MANAGER::GetInstance()->Build(szMapIndexFileName, LocaleService_GetMapPath().c_str()))
	{
		SysLog("cannot load MapIndex: {}", szMapIndexFileName);
		thecore_shutdown();
		return;
	}

	PyLog("LoadLocaleFile: DragonSoulTable: {}", szDragonSoulTableFileName);
	if (!DSManager::GetInstance()->ReadDragonSoulTableFile(szDragonSoulTableFileName))
	{
		SysLog("cannot load DragonSoulTable: {}", szDragonSoulTableFileName);
	}

	// END_OF_LOCALE_SERVICE


	building::CManager::GetInstance()->FinalizeBoot();

	CMotionManager::GetInstance()->Build();

	signal_timer_enable(30);

	if (test_server)
	{
		CMobManager::GetInstance()->DumpRegenCount("mob_count");
	}

	// castle_boot
	castle_boot();
}

EVENTINFO(quest_login_event_info)
{
	uint32_t dwPID;

	quest_login_event_info() 
	: dwPID(0)
	{
	}
};

EVENTFUNC(quest_login_event)
{
	quest_login_event_info* info = dynamic_cast<quest_login_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("quest_login_event> <Factor> Null pointer");
		return 0;
	}

	uint32_t dwPID = info->dwPID;

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(dwPID);

	if (!ch)
		return 0;

	LPDESC d = ch->GetDesc();

	if (!d)
		return 0;

	if (d->IsPhase(PHASE_HANDSHAKE) ||
		d->IsPhase(PHASE_LOGIN) ||
		d->IsPhase(PHASE_SELECT) ||
		d->IsPhase(PHASE_DEAD) ||
		d->IsPhase(PHASE_LOADING))
	{
		return PASSES_PER_SEC(1);
	}
	else if (d->IsPhase(PHASE_CLOSE))
	{
		return 0;
	}
	else if (d->IsPhase(PHASE_GAME))
	{
		PyLog("QUEST_LOAD: Login pc {} by event", ch->GetPlayerID());
		quest::CQuestManager::GetInstance()->Login(ch->GetPlayerID());
		return 0;
	}
	else
	{
		SysLog("input_db.cpp:quest_login_event INVALID PHASE pid {}", ch->GetPlayerID());
		return 0;
	}
}

void CInputDB::QuestLoad(LPDESC d, const char* c_pData)
{
	if (!d)
		return;

	LPCHARACTER ch = d->GetCharacter();

	if (!ch)
		return;

	const uint32_t dwCount = decode_4bytes(c_pData);

	const TQuestTable* pQuestTable = reinterpret_cast<const TQuestTable*>(c_pData+4);

	if (NULL != pQuestTable)
	{
		if (dwCount != 0)
		{
			if (ch->GetPlayerID() != pQuestTable[0].dwPID)
			{
				SysLog("PID differs {} {}", ch->GetPlayerID(), pQuestTable[0].dwPID);
				return;
			}
		}

		PyLog("QUEST_LOAD: count {}", dwCount);

		quest::PC* pPC = quest::CQuestManager::GetInstance()->GetPCForce(ch->GetPlayerID());

		if (!pPC)
		{
			SysLog("null quest::PC with id {}", pQuestTable[0].dwPID);
			return;
		}

		if (pPC->IsLoaded())
			return;

		for (uint32_t i = 0; i < dwCount; ++i)
		{
			std::string st(pQuestTable[i].szName);

			st += ".";
			st += pQuestTable[i].szState;

			PyLog("            {} {}", st.c_str(), pQuestTable[i].lValue);
			pPC->SetFlag(st.c_str(), pQuestTable[i].lValue, false);
		}

		pPC->SetLoaded();
		pPC->Build();

		if (ch->GetDesc()->IsPhase(PHASE_GAME))
		{
			PyLog("QUEST_LOAD: Login pc {}", pQuestTable[0].dwPID);
			quest::CQuestManager::GetInstance()->Login(pQuestTable[0].dwPID);
		}
		else
		{
			quest_login_event_info* info = AllocEventInfo<quest_login_event_info>();
			info->dwPID = ch->GetPlayerID();

			event_create(quest_login_event, info, PASSES_PER_SEC(1));
		}
	}	
}

void CInputDB::SafeboxLoad(LPDESC d, const char* c_pData)
{
	if (!d)
		return;

	TSafeboxTable* p = (TSafeboxTable *) c_pData;

	if (d->GetAccountTable().id != p->dwID)
	{
		SysLog("SafeboxLoad: safebox has different id {} != {}", d->GetAccountTable().id, p->dwID);
		return;
	}

	if (!d->GetCharacter())
		return;

	uint8_t bSize = 1;

	LPCHARACTER ch = d->GetCharacter();

	//PREVENT_TRADE_WINDOW
	if (ch->GetShopOwner() || ch->GetExchange() || ch->GetMyShop() || ch->IsCubeOpen())
	{
		d->GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot open a Storeroom while another window is open."));
		d->GetCharacter()->CancelSafeboxLoad();
		return;
	}
	//END_PREVENT_TRADE_WINDOW

	// ADD_PREMIUM
	if (d->GetCharacter()->GetPremiumRemainSeconds(PREMIUM_SAFEBOX) > 0 ||
			d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_LARGE_SAFEBOX))
		bSize = 3;
	// END_OF_ADD_PREMIUM

	d->GetCharacter()->LoadSafebox(bSize * SAFEBOX_PAGE_SIZE, p->dwGold, p->wItemCount, (TPlayerItem *) (c_pData + sizeof(TSafeboxTable)));
}

void CInputDB::SafeboxChangeSize(LPDESC d, const char* c_pData)
{
	if (!d)
		return;

	uint8_t bSize = *(uint8_t*) c_pData;

	if (!d->GetCharacter())
		return;

	d->GetCharacter()->ChangeSafeboxSize(bSize);
}


void CInputDB::SafeboxWrongPassword(LPDESC d)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	TPacketGCSafeboxWrongPassword p;
	p.bHeader = HEADER_GC_SAFEBOX_WRONG_PASSWORD;
	d->Packet(&p, sizeof(p));

	d->GetCharacter()->CancelSafeboxLoad();
}

void CInputDB::SafeboxChangePasswordAnswer(LPDESC d, const char* c_pData)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	TSafeboxChangePasswordPacketAnswer* p = (TSafeboxChangePasswordPacketAnswer*) c_pData;
	if (p->flag)
	{
		d->GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] Storeroom password has been changed."));
	}
	else
	{
		d->GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] You have entered the wrong password."));
	}
}

void CInputDB::LoginAlready(LPDESC d, const char* c_pData)
{
	if (!d)
		return;

	// INTERNATIONAL_VERSION
	{ 
		TPacketDGLoginAlready* p = (TPacketDGLoginAlready *) c_pData;

		LPDESC d2 = DESC_MANAGER::GetInstance()->FindByLoginName(p->szLogin);

		if (d2)
			d2->DisconnectOfSameLogin();
		else
		{
			TPacketGGDisconnect pgg;

			pgg.bHeader = HEADER_GG_DISCONNECT;
			strlcpy(pgg.szLogin, p->szLogin, sizeof(pgg.szLogin));

			P2P_MANAGER::GetInstance()->Send(&pgg, sizeof(TPacketGGDisconnect));
		}
	}
	// END_OF_INTERNATIONAL_VERSION

	LoginFailure(d, "ALREADY");
}

void CInputDB::EmpireSelect(LPDESC d, const char* c_pData)
{
	if (!d)
		return;

	TAccountTable& rTable = d->GetAccountTable();
	rTable.bEmpire = *(uint8_t*) c_pData;

	TPacketGCEmpire pe;
	pe.bHeader = HEADER_GC_EMPIRE;
	pe.bEmpire = rTable.bEmpire;
	d->Packet(&pe, sizeof(pe));

	for (int32_t i = 0; i < PLAYER_PER_ACCOUNT; ++i) 
		if (rTable.players[i].dwID)
		{
			rTable.players[i].x = EMPIRE_START_X(rTable.bEmpire);
			rTable.players[i].y = EMPIRE_START_Y(rTable.bEmpire);
		}

	GetServerLocation(d->GetAccountTable(), rTable.bEmpire);

	d->SendLoginSuccessPacket();
}

void CInputDB::MapLocations(const char* c_pData)
{
	uint8_t bCount = *(uint8_t*) (c_pData++);

	PyLog("InputDB::MapLocations {}", bCount);

	TMapLocation* pLoc = (TMapLocation *) c_pData;

	while (bCount--)
	{
		for (int32_t i = 0; i < 32; ++i)
		{
			if (0 == pLoc->alMaps[i])
				break;

			CMapLocation::GetInstance()->Insert(pLoc->alMaps[i], pLoc->szHost, pLoc->wPort);
		}

		pLoc++;
	}
}

void CInputDB::P2P(const char* c_pData)
{
	extern LPFDWATCH main_fdw;

	TPacketDGP2P* p = (TPacketDGP2P *) c_pData;

	auto mgr = P2P_MANAGER::GetInstance();

	if (!DESC_MANAGER::GetInstance()->IsP2PDescExist(p->szHost, p->wPort))
	{
	    LPCLIENT_DESC pDesc = nullptr;
		PyLog("InputDB:P2P {}:{}", p->szHost, p->wPort);
	    pDesc = DESC_MANAGER::GetInstance()->CreateConnectionDesc(main_fdw, p->szHost, p->wPort, PHASE_P2P, false);
		mgr->RegisterConnector(pDesc);
		pDesc->SetP2P(p->szHost, p->wPort, p->bChannel);
	}
}

void CInputDB::GuildLoad(const char* c_pData)
{
	CGuildManager::GetInstance()->LoadGuild(*(uint32_t*) c_pData);
}

void CInputDB::GuildSkillUpdate(const char* c_pData)
{
	TPacketGuildSkillUpdate* p = (TPacketGuildSkillUpdate *) c_pData;

	CGuild * g = CGuildManager::GetInstance()->TouchGuild(p->guild_id);

	if (g)
	{
		g->UpdateSkill(p->skill_point, p->skill_levels);
		g->GuildPointChange(POINT_SP, p->amount, p->save?true:false);
	}
}

void CInputDB::GuildWar(const char* c_pData)
{
	TPacketGuildWar* p = (TPacketGuildWar*) c_pData;

	PyLog("InputDB::GuildWar {} {} state {}", p->dwGuildFrom, p->dwGuildTo, p->bWar);

	switch (p->bWar)
	{
		case GUILD_WAR_SEND_DECLARE:
		case GUILD_WAR_RECV_DECLARE:
			CGuildManager::GetInstance()->DeclareWar(p->dwGuildFrom, p->dwGuildTo, p->bType);
			break;

		case GUILD_WAR_REFUSE:
			CGuildManager::GetInstance()->RefuseWar(p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_WAIT_START:
			CGuildManager::GetInstance()->WaitStartWar(p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_CANCEL:
			CGuildManager::GetInstance()->CancelWar(p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_ON_WAR:
			CGuildManager::GetInstance()->StartWar(p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_END:
			CGuildManager::GetInstance()->EndWar(p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_OVER:
			CGuildManager::GetInstance()->WarOver(p->dwGuildFrom, p->dwGuildTo, p->bType);
			break;

		case GUILD_WAR_RESERVE:
			CGuildManager::GetInstance()->ReserveWar(p->dwGuildFrom, p->dwGuildTo, p->bType);
			break;

		default:
			SysLog("Unknown guild war state");
			break;
	}
}

void CInputDB::GuildWarScore(const char* c_pData)
{
	TPacketGuildWarScore* p = (TPacketGuildWarScore*) c_pData;
	CGuild * g = CGuildManager::GetInstance()->TouchGuild(p->dwGuildGainPoint);
	g->SetWarScoreAgainstTo(p->dwGuildOpponent, p->lScore);
}

void CInputDB::GuildSkillRecharge()
{
	CGuildManager::GetInstance()->SkillRecharge();
}

void CInputDB::GuildExpUpdate(const char* c_pData)
{
	TPacketGuildSkillUpdate* p = (TPacketGuildSkillUpdate *) c_pData;
	TraceLog("GuildExpUpdate {}", p->amount);

	CGuild * g = CGuildManager::GetInstance()->TouchGuild(p->guild_id);

	if (g)
		g->GuildPointChange(POINT_EXP, p->amount);
}

void CInputDB::GuildAddMember(const char* c_pData)
{
	TPacketDGGuildMember* p = (TPacketDGGuildMember *) c_pData;
	CGuild * g = CGuildManager::GetInstance()->TouchGuild(p->dwGuild);

	if (g)
		g->AddMember(p);
}

void CInputDB::GuildRemoveMember(const char* c_pData)
{
	TPacketGuild* p=(TPacketGuild*)c_pData;
	CGuild* g = CGuildManager::GetInstance()->TouchGuild(p->dwGuild);

	if (g)
		g->RemoveMember(p->dwInfo);
}

void CInputDB::GuildChangeGrade(const char* c_pData)
{
	TPacketGuild* p=(TPacketGuild*)c_pData;
	CGuild* g = CGuildManager::GetInstance()->TouchGuild(p->dwGuild);

	if (g)
		g->P2PChangeGrade((uint8_t)p->dwInfo);
}

void CInputDB::GuildChangeMemberData(const char* c_pData)
{
	PyLog("Recv GuildChangeMemberData");
	TPacketGuildChangeMemberData* p = (TPacketGuildChangeMemberData *) c_pData;
	CGuild * g = CGuildManager::GetInstance()->TouchGuild(p->guild_id);

	if (g)
		g->ChangeMemberData(p->pid, p->offer, p->level, p->grade);
}

void CInputDB::GuildDisband(const char* c_pData)
{
	TPacketGuild* p = (TPacketGuild*) c_pData;
	CGuildManager::GetInstance()->DisbandGuild(p->dwGuild);
}

void CInputDB::GuildLadder(const char* c_pData)
{
	TPacketGuildLadder* p = (TPacketGuildLadder*) c_pData;
	PyLog("Recv GuildLadder {} {} / w {} d {} l {}", p->dwGuild, p->lLadderPoint, p->lWin, p->lDraw, p->lLoss);
	CGuild * g = CGuildManager::GetInstance()->TouchGuild(p->dwGuild);

	g->SetLadderPoint(p->lLadderPoint);
	g->SetWarData(p->lWin, p->lDraw, p->lLoss);
}

void CInputDB::ItemLoad(LPDESC d, const char* c_pData)
{
	LPCHARACTER ch;

	if (!d || !(ch = d->GetCharacter()))
		return;

	if (ch->IsItemLoaded())
		return;

	uint32_t dwCount = decode_4bytes(c_pData);
	c_pData += sizeof(uint32_t);

	PyLog("ITEM_LOAD: COUNT {} {}", ch->GetName(), dwCount);

	std::vector<LPITEM> v;

	TPlayerItem* p = (TPlayerItem *) c_pData;

	for (uint32_t i = 0; i < dwCount; ++i, ++p)
	{
		LPITEM item = ITEM_MANAGER::GetInstance()->CreateItem(p->vnum, p->count, p->id);

		if (!item)
		{
			SysLog("cannot create item by vnum {} (name {} id {})", p->vnum, ch->GetName(), p->id);
			continue;
		}

		item->SetSkipSave(true);
		item->SetSockets(p->alSockets);
		item->SetAttributes(p->aAttr);

		if ((p->window == INVENTORY && ch->GetInventoryItem(p->pos)) ||
				(p->window == EQUIPMENT && ch->GetWear(p->pos)))
		{
			PyLog("ITEM_RESTORE: {} {}", ch->GetName(), item->GetName());
			v.push_back(item);
		}
		else
		{
			switch (p->window)
			{
				case INVENTORY:
				case DRAGON_SOUL_INVENTORY:
					item->AddToCharacter(ch, TItemPos(p->window, p->pos));
					break;

				case EQUIPMENT:
					if (item->CheckItemUseLevel(ch->GetLevel()))
					{
						if (item->EquipTo(ch, p->pos) == false)
						{
							v.push_back(item);
						}
					}
					else
					{
						v.push_back(item);
					}
					break;
			}
		}

		if (!item->OnAfterCreatedItem())
			SysLog("Failed to call ITEM::OnAfterCreatedItem (vnum: {}, id: {})", item->GetVnum(), item->GetID());

		item->SetSkipSave(false);
	}

	auto it = v.begin();

	while (it != v.end())
	{
		LPITEM item = *(it++);

		int32_t pos = ch->GetEmptyInventory(item->GetSize());

		if (pos < 0)
		{
			PIXEL_POSITION coord;
			coord.x = ch->GetX();
			coord.y = ch->GetY();

			item->AddToGround(ch->GetMapIndex(), coord);
			item->SetOwnership(ch, 180);
			item->StartDestroyEvent();
		}
		else
			item->AddToCharacter(ch, TItemPos(INVENTORY, pos));
	}

	ch->CheckMaximumPoints();
	ch->PointsPacket();

	ch->SetItemLoaded();
}

void CInputDB::AffectLoad(LPDESC d, const char* c_pData)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	LPCHARACTER ch = d->GetCharacter();

	uint32_t dwPID = decode_4bytes(c_pData);
	c_pData += sizeof(uint32_t);

	uint32_t dwCount = decode_4bytes(c_pData);
	c_pData += sizeof(uint32_t);

	if (ch->GetPlayerID() != dwPID)
		return;

	ch->LoadAffect(dwCount, (TPacketAffectElement *) c_pData);
	
}
	


void CInputDB::PartyCreate(const char* c_pData)
{
	TPacketPartyCreate* p = (TPacketPartyCreate*) c_pData;
	CPartyManager::GetInstance()->P2PCreateParty(p->dwLeaderPID);
}

void CInputDB::PartyDelete(const char* c_pData)
{
	TPacketPartyDelete* p = (TPacketPartyDelete*) c_pData;
	CPartyManager::GetInstance()->P2PDeleteParty(p->dwLeaderPID);
}

void CInputDB::PartyAdd(const char* c_pData)
{
	TPacketPartyAdd* p = (TPacketPartyAdd*) c_pData;
	CPartyManager::GetInstance()->P2PJoinParty(p->dwLeaderPID, p->dwPID, p->bState);
}

void CInputDB::PartyRemove(const char* c_pData)
{
	TPacketPartyRemove* p = (TPacketPartyRemove*) c_pData;
	CPartyManager::GetInstance()->P2PQuitParty(p->dwPID);
}

void CInputDB::PartyStateChange(const char* c_pData)
{
	TPacketPartyStateChange* p = (TPacketPartyStateChange *) c_pData;
	LPPARTY pParty = CPartyManager::GetInstance()->P2PCreateParty(p->dwLeaderPID);

	if (!pParty)
		return;

	pParty->SetRole(p->dwPID, p->bRole, p->bFlag);
}

void CInputDB::PartySetMemberLevel(const char* c_pData)
{
	TPacketPartySetMemberLevel* p = (TPacketPartySetMemberLevel*) c_pData;
	LPPARTY pParty = CPartyManager::GetInstance()->P2PCreateParty(p->dwLeaderPID);

	if (!pParty)
		return;

	pParty->P2PSetMemberLevel(p->dwPID, p->bLevel);
}

void CInputDB::Time(const char* c_pData)
{
	set_global_time(*(time_t*) c_pData);
}

void CInputDB::ReloadProto(const char* c_pData)
{
	uint16_t wSize;

	/*
	 * Skill
	 */
	wSize = decode_2bytes(c_pData);
	c_pData += sizeof(uint16_t);
	if (wSize) CSkillManager::GetInstance()->Initialize((TSkillTable *) c_pData, wSize);
	c_pData += sizeof(TSkillTable) * wSize;

	/*
	 * Banwords
	 */

	wSize = decode_2bytes(c_pData);
	c_pData += sizeof(uint16_t);
	CBanwordManager::GetInstance()->Initialize((TBanwordTable *) c_pData, wSize);
	c_pData += sizeof(TBanwordTable) * wSize;

	/*
	 * ITEM
	 */
	wSize = decode_2bytes(c_pData);
	c_pData += 2;
	PyLog("RELOAD: ITEM: {}", wSize);

	if (wSize)
	{
		ITEM_MANAGER::GetInstance()->Initialize((TItemTable *) c_pData, wSize);
		c_pData += wSize * sizeof(TItemTable);
	}

	/*
	 * MONSTER
	 */
	wSize = decode_2bytes(c_pData);
	c_pData += 2;
	PyLog("RELOAD: MOB: {}", wSize);

	if (wSize)
	{
		CMobManager::GetInstance()->Initialize((TMobTable *) c_pData, wSize);
		c_pData += wSize * sizeof(TMobTable);
	}

	CMotionManager::GetInstance()->Build();

	CHARACTER_MANAGER::GetInstance()->for_each_pc(std::mem_fn(&CHARACTER::ComputePoints));
}

void CInputDB::GuildSkillUsableChange(const char* c_pData)
{
	TPacketGuildSkillUsableChange* p = (TPacketGuildSkillUsableChange*) c_pData;

	CGuild* g = CGuildManager::GetInstance()->TouchGuild(p->dwGuild);

	g->SkillUsableChange(p->dwSkillVnum, p->bUsable?true:false);
}

void CInputDB::AuthLogin(LPDESC d, const char* c_pData)
{
	if (!d)
		return;

	uint8_t bResult = *(uint8_t*) c_pData;

	TPacketGCAuthSuccess ptoc;

	ptoc.bHeader = HEADER_GC_AUTH_SUCCESS;

	if (bResult)
	{
		ptoc.dwLoginKey = d->GetLoginKey();
	}
	else
	{
		ptoc.dwLoginKey = 0;
	}

	ptoc.bResult = bResult;

	d->Packet(&ptoc, sizeof(TPacketGCAuthSuccess));
	PyLog("AuthLogin result {} key {}", bResult, d->GetLoginKey());
}
void CInputDB::AuthLoginOpenID(LPDESC d, const char* c_pData)
{
	if (!d)
		return;

	uint8_t bResult = *(uint8_t*) c_pData;

	TPacketGCAuthSuccessOpenID ptoc;

	ptoc.bHeader = HEADER_GC_AUTH_SUCCESS_OPENID;

	if (bResult)
	{
		ptoc.dwLoginKey = d->GetLoginKey();
	}
	else
	{
		ptoc.dwLoginKey = 0;
	}

	strcpy(ptoc.login, d->GetLogin().c_str());

	ptoc.bResult = bResult;

	d->Packet(&ptoc, sizeof(TPacketGCAuthSuccessOpenID));
	PyLog("AuthLogin result {} key {}", bResult, d->GetLoginKey());
}

void CInputDB::ChangeEmpirePriv(const char* c_pData)
{
	TPacketDGChangeEmpirePriv* p = (TPacketDGChangeEmpirePriv*) c_pData;

	// ADD_EMPIRE_PRIV_TIME
	CPrivManager::GetInstance()->GiveEmpirePriv(p->empire, p->type, p->value, p->bLog, p->end_time_sec);
	// END_OF_ADD_EMPIRE_PRIV_TIME
}

/**
 * @version 05/06/08	Bang2ni - Add duration
 */
void CInputDB::ChangeGuildPriv(const char* c_pData)
{
	TPacketDGChangeGuildPriv* p = (TPacketDGChangeGuildPriv*) c_pData;

	// ADD_GUILD_PRIV_TIME
	CPrivManager::GetInstance()->GiveGuildPriv(p->guild_id, p->type, p->value, p->bLog, p->end_time_sec);
	// END_OF_ADD_GUILD_PRIV_TIME
}

void CInputDB::ChangeCharacterPriv(const char* c_pData)
{
	TPacketDGChangeCharacterPriv* p = (TPacketDGChangeCharacterPriv*) c_pData;
	CPrivManager::GetInstance()->GiveCharacterPriv(p->pid, p->type, p->value, p->bLog);
}

void CInputDB::MoneyLog(const char* c_pData)
{
	TPacketMoneyLog* p = (TPacketMoneyLog *) c_pData;

	if (p->type == 4) // QUEST_MONEY_LOG_SKIP
		return;

	if (g_bAuthServer)
		return;

	LogManager::GetInstance()->MoneyLog(p->type, p->vnum, p->gold);
}

void CInputDB::GuildMoneyChange(const char* c_pData)
{
	TPacketDGGuildMoneyChange* p = (TPacketDGGuildMoneyChange*) c_pData;

	CGuild* g = CGuildManager::GetInstance()->TouchGuild(p->dwGuild);
	if (g)
	{
		g->RecvMoneyChange(p->iTotalGold);
	}
}

void CInputDB::GuildWithdrawMoney(const char* c_pData)
{
	TPacketDGGuildMoneyWithdraw* p = (TPacketDGGuildMoneyWithdraw*) c_pData;

	CGuild* g = CGuildManager::GetInstance()->TouchGuild(p->dwGuild);
	if (g)
	{
		g->RecvWithdrawMoneyGive(p->iChangeGold);
	}
}

void CInputDB::SetEventFlag(const char* c_pData)
{
	TPacketSetEventFlag* p = (TPacketSetEventFlag*) c_pData;
	quest::CQuestManager::GetInstance()->SetEventFlag(p->szFlagName, p->lValue);
}

void CInputDB::CreateObject(const char* c_pData)
{
	using namespace building;
	CManager::GetInstance()->LoadObject((TObject *) c_pData);
}

void CInputDB::DeleteObject(const char* c_pData)
{
	using namespace building;
	CManager::GetInstance()->DeleteObject(*(uint32_t*) c_pData);
}

void CInputDB::UpdateLand(const char* c_pData)
{
	using namespace building;
	CManager::GetInstance()->UpdateLand((TLand *) c_pData);
}

void CInputDB::Notice(const char* c_pData)
{
	extern void SendNotice(const char* c_pszBuf);

	char szBuf[256+1];
	strlcpy(szBuf, c_pData, sizeof(szBuf));

	PyLog("InputDB:: Notice: {}", szBuf);

	//SendNotice(LC_TEXT(szBuf));
	SendNotice(szBuf);
}

void CInputDB::GuildWarReserveAdd(TGuildWarReserve* p)
{
	CGuildManager::GetInstance()->ReserveWarAdd(p);
}

void CInputDB::GuildWarReserveDelete(uint32_t dwID)
{
	CGuildManager::GetInstance()->ReserveWarDelete(dwID);
}

void CInputDB::GuildWarBet(TPacketGDGuildWarBet* p)
{
	CGuildManager::GetInstance()->ReserveWarBet(p);
}

void CInputDB::MarriageAdd(TPacketMarriageAdd* p)
{
	PyLog("MarriageAdd {} {} {} {} {}", p->dwPID1, p->dwPID2, (uint32_t)p->tMarryTime, p->szName1, p->szName2);
	marriage::CManager::GetInstance()->Add(p->dwPID1, p->dwPID2, p->tMarryTime, p->szName1, p->szName2);
}

void CInputDB::MarriageUpdate(TPacketMarriageUpdate* p)
{
	PyLog("MarriageUpdate {} {} {} {}", p->dwPID1, p->dwPID2, p->iLovePoint, p->byMarried);
	marriage::CManager::GetInstance()->Update(p->dwPID1, p->dwPID2, p->iLovePoint, p->byMarried);
}

void CInputDB::MarriageRemove(TPacketMarriageRemove* p)
{
	PyLog("MarriageRemove {} {}", p->dwPID1, p->dwPID2);
	marriage::CManager::GetInstance()->Remove(p->dwPID1, p->dwPID2);
}

void CInputDB::WeddingRequest(TPacketWeddingRequest* p)
{
	marriage::WeddingManager::GetInstance()->Request(p->dwPID1, p->dwPID2);
}

void CInputDB::WeddingReady(TPacketWeddingReady* p)
{
	PyLog("WeddingReady {} {} {}", p->dwPID1, p->dwPID2, p->dwMapIndex);
	marriage::CManager::GetInstance()->WeddingReady(p->dwPID1, p->dwPID2, p->dwMapIndex);
}

void CInputDB::WeddingStart(TPacketWeddingStart* p)
{
	PyLog("WeddingStart {} {}", p->dwPID1, p->dwPID2);
	marriage::CManager::GetInstance()->WeddingStart(p->dwPID1, p->dwPID2);
}

void CInputDB::WeddingEnd(TPacketWeddingEnd* p)
{
	PyLog("WeddingEnd {} {}", p->dwPID1, p->dwPID2);
	marriage::CManager::GetInstance()->WeddingEnd(p->dwPID1, p->dwPID2);
}

// MYSHOP_PRICE_LIST
void CInputDB::MyshopPricelistRes(LPDESC d, const TPacketMyshopPricelistHeader* p)
{
	LPCHARACTER ch;

	if (!d || !(ch = d->GetCharacter()))
		return;

	PyLog("RecvMyshopPricelistRes name[{}]", ch->GetName());
	ch->UseSilkBotaryReal(p);

}
// END_OF_MYSHOP_PRICE_LIST


//RELOAD_ADMIN
void CInputDB::ReloadAdmin(const char* c_pData)
{
	gm_new_clear();
	int32_t ChunkSize = decode_2bytes(c_pData);
	c_pData += 2;
	int32_t HostSize = decode_2bytes(c_pData);
	c_pData += 2;
	
	for (int32_t n = 0; n < HostSize; ++n)
	{
		gm_new_host_inert(c_pData);
		c_pData += ChunkSize;
	}
	
	
	c_pData += 2;
	int32_t size = 	decode_2bytes(c_pData);
	c_pData += 2;
	
	for (int32_t n = 0; n < size; ++n)
	{
		tAdminInfo& rAdminInfo = *(tAdminInfo*)c_pData;

		gm_new_insert(rAdminInfo);

		c_pData += sizeof (tAdminInfo);
	
		LPCHARACTER pChar = CHARACTER_MANAGER::GetInstance()->FindPC(rAdminInfo.m_szName);
		if (pChar)
		{
			pChar->SetGMLevel();
		}
	}
	
}
//END_RELOAD_ADMIN

////////////////////////////////////////////////// ////////////////////
// Analyze
// @version 05/06/10 Bang2ni - Added item price information list packet (HEADER_DG_MYSHOP_PRICELIST_RES) processing routine.
////////////////////////////////////////////////// ////////////////////
int32_t CInputDB::Analyze(LPDESC d, uint8_t bHeader, const char* c_pData)
{
	switch (bHeader)
	{
	case HEADER_DG_BOOT:
		Boot(c_pData);
		break;

	case HEADER_DG_LOGIN_SUCCESS:
		LoginSuccess(m_dwHandle, c_pData);
		break;

	case HEADER_DG_LOGIN_NOT_EXIST:
		LoginFailure(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), "NOID");
		break;

	case HEADER_DG_LOGIN_WRONG_PASSWD:
		LoginFailure(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), "WRONGPWD");
		break;

	case HEADER_DG_LOGIN_ALREADY:
		LoginAlready(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_PLAYER_LOAD_SUCCESS:
		PlayerLoad(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_PLAYER_CREATE_SUCCESS:
		PlayerCreateSuccess(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_PLAYER_CREATE_FAILED:
		PlayerCreateFailure(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), 0);
		break;

	case HEADER_DG_PLAYER_CREATE_ALREADY:
		PlayerCreateFailure(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), 1);
		break;

	case HEADER_DG_PLAYER_DELETE_SUCCESS:
		PlayerDeleteSuccess(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_PLAYER_LOAD_FAILED:
		break;

	case HEADER_DG_PLAYER_DELETE_FAILED:
		PlayerDeleteFail(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle));
		break;

	case HEADER_DG_ITEM_LOAD:
		ItemLoad(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_QUEST_LOAD:
		QuestLoad(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_AFFECT_LOAD:
		AffectLoad(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_SAFEBOX_LOAD:
		SafeboxLoad(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_SAFEBOX_CHANGE_SIZE:
		SafeboxChangeSize(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_SAFEBOX_WRONG_PASSWORD:
		SafeboxWrongPassword(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle));
		break;

	case HEADER_DG_SAFEBOX_CHANGE_PASSWORD_ANSWER:
		SafeboxChangePasswordAnswer(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_EMPIRE_SELECT:
		EmpireSelect(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_MAP_LOCATIONS:
		MapLocations(c_pData);
		break;

	case HEADER_DG_P2P:
		P2P(c_pData);
		break;

	case HEADER_DG_GUILD_SKILL_UPDATE:
		GuildSkillUpdate(c_pData);
		break;

	case HEADER_DG_GUILD_LOAD:
		GuildLoad(c_pData);
		break;

	case HEADER_DG_GUILD_SKILL_RECHARGE:
		GuildSkillRecharge();
		break;

	case HEADER_DG_GUILD_EXP_UPDATE:
		GuildExpUpdate(c_pData);
		break;

	case HEADER_DG_PARTY_CREATE:
		PartyCreate(c_pData);
		break;

	case HEADER_DG_PARTY_DELETE:
		PartyDelete(c_pData);
		break;

	case HEADER_DG_PARTY_ADD:
		PartyAdd(c_pData);
		break;

	case HEADER_DG_PARTY_REMOVE:
		PartyRemove(c_pData);
		break;

	case HEADER_DG_PARTY_STATE_CHANGE:
		PartyStateChange(c_pData);
		break;

	case HEADER_DG_PARTY_SET_MEMBER_LEVEL:
		PartySetMemberLevel(c_pData);    
		break;

	case HEADER_DG_TIME:
		Time(c_pData);
		break;

	case HEADER_DG_GUILD_ADD_MEMBER:
		GuildAddMember(c_pData);
		break;

	case HEADER_DG_GUILD_REMOVE_MEMBER:
		GuildRemoveMember(c_pData);
		break;

	case HEADER_DG_GUILD_CHANGE_GRADE:
		GuildChangeGrade(c_pData);
		break;

	case HEADER_DG_GUILD_CHANGE_MEMBER_DATA:
		GuildChangeMemberData(c_pData);
		break;

	case HEADER_DG_GUILD_DISBAND:
		GuildDisband(c_pData);
		break;

	case HEADER_DG_RELOAD_PROTO:
		ReloadProto(c_pData);
		break;

	case HEADER_DG_GUILD_WAR:
		GuildWar(c_pData);
		break;

	case HEADER_DG_GUILD_WAR_SCORE:
		GuildWarScore(c_pData);
		break;

	case HEADER_DG_GUILD_LADDER:
		GuildLadder(c_pData);
		break;

	case HEADER_DG_GUILD_SKILL_USABLE_CHANGE:
		GuildSkillUsableChange(c_pData);
		break;

	case HEADER_DG_CHANGE_NAME:
		ChangeName(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_AUTH_LOGIN:
		if (openid_server)
			AuthLoginOpenID(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		else
			AuthLogin(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_CHANGE_EMPIRE_PRIV:
		ChangeEmpirePriv(c_pData);
		break;

	case HEADER_DG_CHANGE_GUILD_PRIV:
		ChangeGuildPriv(c_pData);
		break;

	case HEADER_DG_CHANGE_CHARACTER_PRIV:
		ChangeCharacterPriv(c_pData);
		break;

	case HEADER_DG_MONEY_LOG:
		MoneyLog(c_pData);
		break;

	case HEADER_DG_GUILD_WITHDRAW_MONEY_GIVE:
		GuildWithdrawMoney(c_pData);
		break;

	case HEADER_DG_GUILD_MONEY_CHANGE:
		GuildMoneyChange(c_pData);
		break;

	case HEADER_DG_SET_EVENT_FLAG:
		SetEventFlag(c_pData);
		break;

	case HEADER_DG_CREATE_OBJECT:
		CreateObject(c_pData);
		break;

	case HEADER_DG_DELETE_OBJECT:
		DeleteObject(c_pData);
		break;

	case HEADER_DG_UPDATE_LAND:
		UpdateLand(c_pData);
		break;

	case HEADER_DG_NOTICE:
		Notice(c_pData);
		break;

	case HEADER_DG_GUILD_WAR_RESERVE_ADD:
		GuildWarReserveAdd((TGuildWarReserve *) c_pData);
		break;

	case HEADER_DG_GUILD_WAR_RESERVE_DEL:
		GuildWarReserveDelete(*(uint32_t*) c_pData);
		break;

	case HEADER_DG_GUILD_WAR_BET:
		GuildWarBet((TPacketGDGuildWarBet *) c_pData);
		break;

	case HEADER_DG_MARRIAGE_ADD:
		MarriageAdd((TPacketMarriageAdd*) c_pData);
		break;

	case HEADER_DG_MARRIAGE_UPDATE:
		MarriageUpdate((TPacketMarriageUpdate*) c_pData);
		break;

	case HEADER_DG_MARRIAGE_REMOVE:
		MarriageRemove((TPacketMarriageRemove*) c_pData);
		break;

	case HEADER_DG_WEDDING_REQUEST:
		WeddingRequest((TPacketWeddingRequest*) c_pData);
		break;

	case HEADER_DG_WEDDING_READY:
		WeddingReady((TPacketWeddingReady*) c_pData);
		break;

	case HEADER_DG_WEDDING_START:
		WeddingStart((TPacketWeddingStart*) c_pData);
		break;

	case HEADER_DG_WEDDING_END:
		WeddingEnd((TPacketWeddingEnd*) c_pData);
		break;

	case HEADER_DG_MYSHOP_PRICELIST_RES:
		MyshopPricelistRes(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), (TPacketMyshopPricelistHeader*) c_pData);
		break;
	case HEADER_DG_RELOAD_ADMIN:
		ReloadAdmin(c_pData);		
		break;
	
	case HEADER_DG_ADD_MONARCH_MONEY:
		AddMonarchMoney(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData); 
		break;

	case HEADER_DG_DEC_MONARCH_MONEY:
		DecMonarchMoney(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_TAKE_MONARCH_MONEY:
		TakeMonarchMoney(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;

	case HEADER_DG_CHANGE_MONARCH_LORD_ACK :
		ChangeMonarchLord((TPacketChangeMonarchLordACK*)c_pData);
		break;

	case HEADER_DG_UPDATE_MONARCH_INFO :
		UpdateMonarchInfo((TMonarchInfo*)c_pData);
		break;

	case HEADER_DG_ACK_CHANGE_GUILD_MASTER :
		this->GuildChangeMaster((TPacketChangeGuildMaster*) c_pData);
		break;	
	case HEADER_DG_ACK_SPARE_ITEM_ID_RANGE :
		ITEM_MANAGER::GetInstance()->SetMaxSpareItemID(*((TItemIDRangeTable*)c_pData));
		break;

	case HEADER_DG_UPDATE_HORSE_NAME :
	case HEADER_DG_ACK_HORSE_NAME :
		CHorseNameManager::GetInstance()->UpdateHorseName(
				((TPacketUpdateHorseName*)c_pData)->dwPlayerID, 
				((TPacketUpdateHorseName*)c_pData)->szHorseName);
		break;

	case HEADER_DG_NEED_LOGIN_LOG:
		DetailLog((TPacketNeedLoginLogInfo*) c_pData);
		break;
	case HEADER_DG_RESPOND_CHANNELSTATUS:
		RespondChannelStatus(DESC_MANAGER::GetInstance()->FindByHandle(m_dwHandle), c_pData);
		break;
	default:
		return (-1);
	}

	return 0;
}

bool CInputDB::Process(LPDESC d, const void* orig, int32_t bytes, int32_t& r_iBytesProceed)
{
	const char* 	c_pData = (const char* ) orig;
	uint8_t		bHeader, bLastHeader = 0;
	int32_t			iSize;
	int32_t			iLastPacketLen = 0;

	for (m_iBufferLeft = bytes; m_iBufferLeft > 0;)
	{
		if (m_iBufferLeft < 9)
			return true;

		bHeader		= *((uint8_t*) (c_pData));	// 1
		m_dwHandle	= *((uint32_t*) (c_pData + 1));	// 4
		iSize		= *((uint32_t*) (c_pData + 5));	// 4

		TraceLog("DBCLIENT: header {} handle {} size {} bytes {}", bHeader, m_dwHandle, iSize, bytes); 

		if (m_iBufferLeft - 9 < iSize)
			return true;

		const char* pRealData = (c_pData + 9);

		if (Analyze(d, bHeader, pRealData) < 0)
		{
			SysLog("in InputDB: UNKNOWN HEADER: {}, LAST HEADER: {}({}), REMAIN BYTES: {}, DESC: {}",
					bHeader, bLastHeader, iLastPacketLen, m_iBufferLeft, d->GetSocket());

		}

		c_pData		+= 9 + iSize;
		m_iBufferLeft	-= 9 + iSize;
		r_iBytesProceed	+= 9 + iSize;

		iLastPacketLen	= 9 + iSize;
		bLastHeader	= bHeader;
	}

	return true;
}

void CInputDB::AddMonarchMoney(LPDESC d, const char* data)
{
	int32_t Empire = *(int32_t*) data;
	data += sizeof(int32_t);

	int32_t Money = *(int32_t*) data;
	data += sizeof(int32_t);
	
	CMonarch::GetInstance()->AddMoney(Money, Empire);

	uint32_t pid = CMonarch::GetInstance()->GetMonarchPID(Empire);	

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pid);

	if (ch)
	{
		if (number(1, 100) > 95) 
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s still has %u Yang available."), EMPIRE_NAME(Empire), CMonarch::GetInstance()->GetMoney(Empire));
	}
}
	
void CInputDB::DecMonarchMoney(LPDESC d, const char* data)
{
	int32_t Empire = *(int32_t*) data;
	data += sizeof(int32_t);
	
	int32_t Money = *(int32_t*) data;
	data += sizeof(int32_t);

	CMonarch::GetInstance()->DecMoney(Money, Empire);
	
	uint32_t pid = CMonarch::GetInstance()->GetMonarchPID(Empire);	

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pid);

	if (ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s still has %d Yang available."), EMPIRE_NAME(Empire), CMonarch::GetInstance()->GetMoney(Empire));
	}
}

void CInputDB::TakeMonarchMoney(LPDESC d, const char* data)
{
	int32_t Empire = *(int32_t*) data;
	data += sizeof(int32_t);
	
	int32_t Money = *(int32_t*) data;
	data += sizeof(int32_t);

	if (!CMonarch::GetInstance()->DecMoney(Money, Empire))
	{
		if (!d)
			return;

		if (!d->GetCharacter())
			return;

		LPCHARACTER ch = d->GetCharacter();
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have enough Yang."));
	}
}

void CInputDB::ChangeMonarchLord(TPacketChangeMonarchLordACK* info)
{
	char notice[256];
	snprintf(notice, sizeof(notice), LC_TEXT("The emperor of %s has changed to %s."), EMPIRE_NAME(info->bEmpire), info->szName);
	SendNotice(notice);
}

void CInputDB::UpdateMonarchInfo(TMonarchInfo* info)
{
	CMonarch::GetInstance()->SetMonarchInfo(info);
	PyLog("MONARCH INFO UPDATED");
}

void CInputDB::GuildChangeMaster(TPacketChangeGuildMaster* p)
{
	CGuildManager::GetInstance()->ChangeMaster(p->dwGuildID);
}

void CInputDB::DetailLog(const TPacketNeedLoginLogInfo* info)
{
	LPCHARACTER pChar = CHARACTER_MANAGER::GetInstance()->FindByPID(info->dwPlayerID);

	if (NULL != pChar)
	{
		LogManager::GetInstance()->DetailLoginLog(true, pChar);
	}
}

void CInputDB::RespondChannelStatus(LPDESC desc, const char* pcData) 
{
	if (!desc) {
		return;
	}
	const int32_t nSize = decode_4bytes(pcData);
	pcData += sizeof(nSize);

	uint8_t bHeader = HEADER_GC_RESPOND_CHANNELSTATUS;
	desc->BufferedPacket(&bHeader, sizeof(uint8_t));
	desc->BufferedPacket(&nSize, sizeof(nSize));
	if (0 < nSize) {
		desc->BufferedPacket(pcData, sizeof(TChannelStatus)*nSize);
	}
	uint8_t bSuccess = 1;
	desc->Packet(&bSuccess, sizeof(bSuccess));
	desc->SetChannelStatusRequested(false);
}