#include "stdafx.h"
#include <Core/Net/PacketsCG.hpp>
#include <sstream>
#include <Common/length.h>

#include "db.h"

#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "p2p.h"
#include "log.h"
#include "login_data.h"
#include "locale_service.h"
#include "spam.h"

extern std::string g_stBlockDate;
extern int32_t openid_server;

DBManager::DBManager() : m_bIsConnect(false)
{
}

DBManager::~DBManager()
{
}

bool DBManager::Connect(const char* host, const int32_t port, const char* user, const char* pwd, const char* db)
{
	if (m_sql.Setup(host, user, pwd, db, g_stLocale.c_str(), false, port))
		m_bIsConnect = true;

	if (!m_sql_direct.Setup(host, user, pwd, db, g_stLocale.c_str(), true, port))
		SysLog("cannot open direct sql connection to host {}", host);

	if (m_bIsConnect && !g_bAuthServer)
	{
		LoadDBString();
	}

	return m_bIsConnect;
}

void DBManager::Query(const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

	m_sql.AsyncQuery(szQuery);
}

SQLMsg * DBManager::DirectQuery(const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

	return m_sql_direct.DirectQuery(szQuery);
}

bool DBManager::IsConnected()
{
	return m_bIsConnect;
}

void DBManager::ReturnQuery(int32_t iType, uint32_t dwIdent, void* pvData, const char* c_pszFormat, ...)
{
	//PyLog("ReturnQuery {}", c_pszQuery);
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

	CReturnQueryInfo* p = M2_NEW CReturnQueryInfo;

	p->iQueryType = QUERY_TYPE_RETURN;
	p->iType = iType;
	p->dwIdent = dwIdent;
	p->pvData = pvData;

	m_sql.ReturnQuery(szQuery, p);
}

void DBManager::FuncQuery(std::function<void(SQLMsg*)> f, const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, 4096, c_pszFormat, args);
	va_end(args);

	auto p = M2_NEW CFuncQueryInfo;

	p->iQueryType = QUERY_TYPE_FUNCTION;
	p->f = f;

	m_sql.ReturnQuery(szQuery, p);
}

void DBManager::FuncAfterQuery(std::function<void()> f, const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, 4096, c_pszFormat, args);
	va_end(args);

	auto  p = M2_NEW CFuncAfterQueryInfo;

	p->iQueryType = QUERY_TYPE_AFTER_FUNCTION;
	p->f = f;

	m_sql.ReturnQuery(szQuery, p);
}

SQLMsg * DBManager::PopResult()
{
	SQLMsg* p;

	if (m_sql.PopResult(&p))
		return p;

	return NULL;
}

void DBManager::Process()
{
	SQLMsg* pMsg = nullptr;

	while ((pMsg = PopResult()))
	{
		if(NULL != pMsg->pvUserData)
		{
			switch(reinterpret_cast<CQueryInfo*>(pMsg->pvUserData)->iQueryType)
			{
				case QUERY_TYPE_RETURN:
					AnalyzeReturnQuery(pMsg);
					break;

				case QUERY_TYPE_FUNCTION:
					{
						CFuncQueryInfo* qi = reinterpret_cast<CFuncQueryInfo*>(pMsg->pvUserData);
						qi->f(pMsg);
						M2_DELETE(qi);
					}
					break;

				case QUERY_TYPE_AFTER_FUNCTION:
					{
						CFuncAfterQueryInfo* qi = reinterpret_cast<CFuncAfterQueryInfo*>(pMsg->pvUserData);
						qi->f();
						M2_DELETE(qi);
					}
					break;
			}
		}

		delete pMsg;
	}
}

CLoginData * DBManager::GetLoginData(uint32_t dwKey)
{
	std::map<uint32_t, CLoginData *>::iterator it = m_map_pLoginData.find(dwKey);

	if (it == m_map_pLoginData.end())
		return NULL;

	return it->second;
}

void DBManager::InsertLoginData(CLoginData* pLD)
{
	m_map_pLoginData.insert(std::make_pair(pLD->GetKey(), pLD));
}

void DBManager::DeleteLoginData(CLoginData* pLD)
{
	std::map<uint32_t, CLoginData *>::iterator it = m_map_pLoginData.find(pLD->GetKey());

	if (it == m_map_pLoginData.end())
		return;

	PyLog("Deleting login data for account {}", pLD->GetLogin());

	M2_DELETE(it->second);
	m_map_pLoginData.erase(it);
}

void DBManager::SendLoginPing(const char* c_pszLogin)
{
	TPacketGGLoginPing ptog;

	ptog.bHeader = HEADER_GG_LOGIN_PING;
	strlcpy(ptog.szLogin, c_pszLogin, sizeof(ptog.szLogin));

	if (!g_pAuthMasterDesc)  // If I am master, broadcast to others
	{
		P2P_MANAGER::GetInstance()->Send(&ptog, sizeof(TPacketGGLoginPing));
	}
	else // If I am slave send login ping to master
	{
		g_pAuthMasterDesc->Packet(&ptog, sizeof(TPacketGGLoginPing));
	}
}

void DBManager::LoginPrepare(LPDESC d, uint32_t* pdwClientKey, int32_t* paiPremiumTimes)
{
	const TAccountTable& r = d->GetAccountTable();

	CLoginData* pLD = M2_NEW CLoginData;

	pLD->SetKey(d->GetLoginKey());
	pLD->SetLogin(r.login);
	pLD->SetIP(d->GetHostName());
	pLD->SetClientKey(pdwClientKey);

	if (paiPremiumTimes)
		pLD->SetPremium(paiPremiumTimes);

	InsertLoginData(pLD);
	SendAuthLogin(d);
}

void DBManager::SendAuthLogin(LPDESC d)
{
	const TAccountTable& r = d->GetAccountTable();

	CLoginData* pLD = GetLoginData(d->GetLoginKey());

	if (!pLD)
		return;

	TPacketGDAuthLogin ptod;
	ptod.dwID = r.id;
	
	trim_and_lower(r.login, ptod.szLogin, sizeof(ptod.szLogin));
	strlcpy(ptod.szSocialID, r.social_id, sizeof(ptod.szSocialID));
	ptod.dwLoginKey = d->GetLoginKey();

	memcpy(ptod.iPremiumTimes, pLD->GetPremiumPtr(), sizeof(ptod.iPremiumTimes));
	memcpy(&ptod.adwClientKey, pLD->GetClientKey(), sizeof(uint32_t) * 4);

	db_clientdesc->DBPacket(HEADER_GD_AUTH_LOGIN, d->GetHandle(), &ptod, sizeof(TPacketGDAuthLogin));
	PyLog("SendAuthLogin {} key {}", ptod.szLogin, ptod.dwID);

	SendLoginPing(r.login);
}

void DBManager::AnalyzeReturnQuery(SQLMsg* pMsg)
{
	CReturnQueryInfo * qi = (CReturnQueryInfo *) pMsg->pvUserData;

	switch (qi->iType)
	{
		case QID_AUTH_LOGIN:
			{
				TPacketCGLogin3* pinfo = (TPacketCGLogin3 *) qi->pvData;
				LPDESC d = DESC_MANAGER::GetInstance()->FindByLoginKey(qi->dwIdent);

				if (!d)
				{
					M2_DELETE(pinfo);
					break;
				}
				// Change Location - By SeMinZ
				d->SetLogin(pinfo->login);

				PyLog("QID_AUTH_LOGIN: START {}", qi->dwIdent);

				if (pMsg->Get()->uiNumRows == 0)
				{
					PyLog("   NOID");
					LoginFailure(d, "NOID");
					M2_DELETE(pinfo);
				}
				else
				{
					MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
					int32_t col = 0;

					// PASSWORD('%s'), password, securitycode, social_id, id, status
					char szEncrytPassword[45 + 1];
					char szPassword[45 + 1];
					char szSocialID[SOCIAL_ID_MAX_LEN + 1];
					char szStatus[ACCOUNT_STATUS_MAX_LEN + 1];
					uint32_t dwID = 0;

					if (!row[col]) 
					{ 
						SysLog("error column {}", col);
						M2_DELETE(pinfo);
					   	break; 
					}
					
					strlcpy(szEncrytPassword, row[col++], sizeof(szEncrytPassword));

					if (!row[col]) 
					{
					   	SysLog("error column {}", col);
						M2_DELETE(pinfo);
					   	break;
				   	}
				
					strlcpy(szPassword, row[col++], sizeof(szPassword));

					col++;

					if (!row[col])
				   	{ 
						SysLog("error column {}", col); 
						M2_DELETE(pinfo);
						break;
				   	}

					strlcpy(szSocialID, row[col++], sizeof(szSocialID));

					if (!row[col])
				   	{
					   	SysLog("error column {}", col);
						M2_DELETE(pinfo);
					   	break;
				   	}
				
					str_to_number(dwID, row[col++]);
					
					if (!row[col]) 
					{
					   	SysLog("error column {}", col); 
						M2_DELETE(pinfo);
						break;
				   	}

					strlcpy(szStatus, row[col++], sizeof(szStatus));

					uint8_t bNotAvail = 0;
					str_to_number(bNotAvail, row[col++]);

					int32_t aiPremiumTimes[PREMIUM_MAX_NUM];
					memset(&aiPremiumTimes, 0, sizeof(aiPremiumTimes));

					char szCreateDate[256] = "00000000";

					str_to_number(aiPremiumTimes[PREMIUM_EXP], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_ITEM], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_SAFEBOX], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_AUTOLOOT], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_FISH_MIND], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_MARRIAGE_FAST], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_GOLD], row[col++]);

					int32_t retValue = 0;
					str_to_number(retValue, row[col]);

					time_t create_time = retValue;
					struct tm * tm1;
					tm1 = localtime(&create_time);
					strftime(szCreateDate, 255, "%Y%m%d", tm1);

					PyLog("Create_Time {} {}", retValue, szCreateDate);
					PyLog("Block Time {} ", strncmp(szCreateDate, g_stBlockDate.c_str(), 8));

					int32_t nPasswordDiff = strcmp(szEncrytPassword, szPassword);

					if (nPasswordDiff)
					{
						LoginFailure(d, "WRONGPWD");
						PyLog("   WRONGPWD");
						M2_DELETE(pinfo);
					}
					else if (bNotAvail)
					{
						LoginFailure(d, "NOTAVAIL");
						PyLog("   NOTAVAIL");
						M2_DELETE(pinfo);
					}
					else if (DESC_MANAGER::GetInstance()->FindByLoginName(pinfo->login))
					{
						LoginFailure(d, "ALREADY");
						PyLog("   ALREADY");
						M2_DELETE(pinfo);
					}
					else if (strcmp(szStatus, "OK"))
					{
						LoginFailure(d, szStatus);
						PyLog("   STATUS: {}", szStatus);
						M2_DELETE(pinfo);
					}
					else
					{
						//stBlockData >= 0 == Date is future than BlockDate
						if (strncmp(szCreateDate, g_stBlockDate.c_str(), 8) >= 0)
						{
							LoginFailure(d, "BLKLOGIN");
							PyLog("   BLKLOGIN");
							M2_DELETE(pinfo);
							break;
						}

						char szQuery[1024];
						snprintf(szQuery, sizeof(szQuery), "UPDATE account SET last_play=NOW() WHERE id=%u", dwID);
						std::unique_ptr<SQLMsg> msg(DBManager::GetInstance()->DirectQuery(szQuery));

						TAccountTable& r = d->GetAccountTable();

						r.id = dwID;
						trim_and_lower(pinfo->login, r.login, sizeof(r.login));
						strlcpy(r.passwd, pinfo->passwd, sizeof(r.passwd));
						strlcpy(r.social_id, szSocialID, sizeof(r.social_id));
						DESC_MANAGER::GetInstance()->ConnectAccount(r.login, d);

						LoginPrepare(d, pinfo->adwClientKey, aiPremiumTimes);

						PyLog("QID_AUTH_LOGIN: SUCCESS {}", pinfo->login);
						M2_DELETE(pinfo);
					}
				}
			}
			break;
		case QID_AUTH_LOGIN_OPENID:
			{
				TPacketCGLogin3* pinfo = (TPacketCGLogin3 *) qi->pvData;
				LPDESC d = DESC_MANAGER::GetInstance()->FindByLoginKey(qi->dwIdent);

				if (!d)
				{
					M2_DELETE(pinfo);
					break;
				}
				//change location - By SeMinZ
				d->SetLogin(pinfo->login);

				PyLog("QID_AUTH_LOGIN_OPENID: START {}", qi->dwIdent);

				if (pMsg->Get()->uiNumRows == 0)
				{
					PyLog("   NOID");
					LoginFailure(d, "NOID");
					M2_DELETE(pinfo);
				}
				else
				{
					MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
					int32_t col = 0;

					// PASSWORD('%s'), password, securitycode, social_id, id, status
					char szEncrytPassword[45 + 1];
					char szPassword[45 + 1];
					char szSocialID[SOCIAL_ID_MAX_LEN + 1];
					char szStatus[ACCOUNT_STATUS_MAX_LEN + 1];
					uint32_t dwID = 0;

					if (!row[col]) 
					{ 
						SysLog("error column {}", col);
						M2_DELETE(pinfo);
					   	break; 
					}
					
					strlcpy(szEncrytPassword, row[col++], sizeof(szEncrytPassword));

					if (!row[col]) 
					{
					   	SysLog("error column {}", col);
						M2_DELETE(pinfo);
					   	break;
				   	}
				
					strlcpy(szPassword, row[col++], sizeof(szPassword));

					if (!row[col])
				   	{ 
						SysLog("error column {}", col); 
						M2_DELETE(pinfo);
						break;
				   	}

					col++;

					strlcpy(szSocialID, row[col++], sizeof(szSocialID));

					if (!row[col])
				   	{
					   	SysLog("error column {}", col);
						M2_DELETE(pinfo);
					   	break;
				   	}
				
					str_to_number(dwID, row[col++]);
					
					if (!row[col]) 
					{
					   	SysLog("error column {}", col); 
						M2_DELETE(pinfo);
						break;
				   	}

					strlcpy(szStatus, row[col++], sizeof(szStatus));

					uint8_t bNotAvail = 0;
					str_to_number(bNotAvail, row[col++]);

					int32_t aiPremiumTimes[PREMIUM_MAX_NUM];
					memset(&aiPremiumTimes, 0, sizeof(aiPremiumTimes));

					char szCreateDate[256] = "00000000";

					str_to_number(aiPremiumTimes[PREMIUM_EXP], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_ITEM], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_SAFEBOX], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_AUTOLOOT], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_FISH_MIND], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_MARRIAGE_FAST], row[col++]);
					str_to_number(aiPremiumTimes[PREMIUM_GOLD], row[col++]);

					int32_t retValue = 0;
					str_to_number(retValue, row[col]);

					time_t create_time = retValue;
					struct tm * tm1;
					tm1 = localtime(&create_time);
					strftime(szCreateDate, 255, "%Y%m%d", tm1);

					PyLog("Create_Time {} {}", retValue, szCreateDate);
					PyLog("Block Time {} ", strncmp(szCreateDate, g_stBlockDate.c_str(), 8));

					int32_t nPasswordDiff = strcmp(szEncrytPassword, szPassword);

					//OpenID : OpenID In case of , password check is not performed.
					if (openid_server)
					{
						nPasswordDiff = 0;
					}

					if (nPasswordDiff)
					{
						LoginFailure(d, "WRONGPWD");
						PyLog("   WRONGPWD");
						M2_DELETE(pinfo);
					}
					else if (bNotAvail)
					{
						LoginFailure(d, "NOTAVAIL");
						PyLog("   NOTAVAIL");
						M2_DELETE(pinfo);
					}
					else if (DESC_MANAGER::GetInstance()->FindByLoginName(pinfo->login))
					{
						LoginFailure(d, "ALREADY");
						PyLog("   ALREADY");
						M2_DELETE(pinfo);
					}
					else if (strcmp(szStatus, "OK"))
					{
						LoginFailure(d, szStatus);
						PyLog("   STATUS: {}", szStatus);
						M2_DELETE(pinfo);
					}
					else
					{
						//stBlockData >= 0 == Date is future than BlockDate
						if (strncmp(szCreateDate, g_stBlockDate.c_str(), 8) >= 0)
						{
							LoginFailure(d, "BLKLOGIN");
							PyLog("   BLKLOGIN");
							M2_DELETE(pinfo);
							break;
						}

						char szQuery[1024];
						snprintf(szQuery, sizeof(szQuery), "UPDATE account SET last_play=NOW() WHERE id=%u", dwID);
						std::unique_ptr<SQLMsg> msg(DBManager::GetInstance()->DirectQuery(szQuery));

						TAccountTable& r = d->GetAccountTable();

						r.id = dwID;
						trim_and_lower(pinfo->login, r.login, sizeof(r.login));
						strlcpy(r.passwd, pinfo->passwd, sizeof(r.passwd));
						strlcpy(r.social_id, szSocialID, sizeof(r.social_id));
						DESC_MANAGER::GetInstance()->ConnectAccount(r.login, d);

						LoginPrepare(d, pinfo->adwClientKey, aiPremiumTimes);

						PyLog("QID_AUTH_LOGIN_OPENID: SUCCESS {}", pinfo->login);
						M2_DELETE(pinfo);
					}
				}
			}
			break;

		case QID_SAFEBOX_SIZE:
			{
				LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(qi->dwIdent);

				if (ch)
				{
					if (pMsg->Get()->uiNumRows > 0)
					{
						MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
						int32_t	size = 0;
						str_to_number(size, row[0]);
						ch->SetSafeboxSize(SAFEBOX_PAGE_SIZE * size);
					}
				}
			}
			break;

		case QID_DB_STRING:
			{
				m_map_dbstring.clear();
				m_vec_GreetMessage.clear();

				for (uint i = 0; i < pMsg->Get()->uiNumRows; ++i)
				{
					MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
					//ch->SetSafeboxSize(SAFEBOX_PAGE_SIZE * atoi(row[0]));
					if (row[0] && row[1])
					{
						m_map_dbstring.insert(make_pair(std::string(row[0]), std::string(row[1])));
						PyLog("DBSTR '{}' '{}'", row[0], row[1]);
					}
				}
				if (m_map_dbstring.find("GREET") != m_map_dbstring.end())
				{
					std::istringstream is(m_map_dbstring["GREET"]);
					while (!is.eof())
					{
						std::string str;
						getline(is, str);
						m_vec_GreetMessage.push_back(str);
					}
				}
			}
			break;

		case QID_LOTTO:
			{
				LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(qi->dwIdent);
				uint32_t* pdw = (uint32_t*) qi->pvData;

				if (ch)
				{
					if (pMsg->Get()->uiAffectedRows == 0 || pMsg->Get()->uiAffectedRows == (uint32_t)-1)
					{
						PyLog("GIVE LOTTO FAIL TO pid {}", ch->GetPlayerID());
					}
					else
					{
						LPITEM pItem = ch->AutoGiveItem(pdw[0], pdw[1]);

						if (pItem)
						{
							PyLog("GIVE LOTTO SUCCESS TO {} (pid {})", ch->GetName(), qi->dwIdent);
							//ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s received"), pItem->GetName());

							pItem->SetSocket(0, pMsg->Get()->uiInsertID);
							pItem->SetSocket(1, pdw[2]);
						}
						else
							PyLog("GIVE LOTTO FAIL2 TO pid {}", ch->GetPlayerID());
					}
				}

				M2_DELETE_ARRAY(pdw);
			}
			break;

		case QID_HIGHSCORE_REGISTER:
			{
				THighscoreRegisterQueryInfo * info = (THighscoreRegisterQueryInfo *) qi->pvData;
				bool bQuery = true;

				if (pMsg->Get()->uiNumRows)
				{
					MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);

					if (row && row[0])
					{
						int32_t iCur = 0;
						str_to_number(iCur, row[0]);

						if ((info->bOrder && iCur >= info->iValue) ||
								(!info->bOrder && iCur <= info->iValue))
							bQuery = false;
					}
				}

				if (bQuery)
					Query("REPLACE INTO highscore%s VALUES('%s', %u, %d)",
							get_table_postfix(), info->szBoard, info->dwPID, info->iValue);

				M2_DELETE(info);
			}
			break;

		case QID_HIGHSCORE_SHOW:
			{
			}
			break;

			// BLOCK_CHAT
		case QID_BLOCK_CHAT_LIST:
			{
				LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(qi->dwIdent);
				
				if (ch == nullptr)
					break;
				if (pMsg->Get()->uiNumRows)
				{
					MYSQL_ROW row;
					while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, "%s %s sec", row[0], row[1]);
					}
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "No one currently blocked.");
				}
			}
			break;
			// END_OF_BLOCK_CHAT

		case QID_BRAZIL_CREATE_ID :
			{
				TPacketCGLogin3* pinfo = (TPacketCGLogin3 *) qi->pvData ;

				if(pMsg->Get()->uiAffectedRows == 0 || pMsg->Get()->uiAffectedRows == (uint32_t)-1)
				{
					LPDESC d = DESC_MANAGER::GetInstance()->FindByLoginKey(qi->dwIdent) ;
					PyLog("[AUTH_BRAZIL]   NOID") ;
					PyLog("[AUTH_BRAZIL] : Failed to create a new account {}", pinfo->login) ;
					LoginFailure(d, "NOID") ;
					M2_DELETE(pinfo);
				}
				else
				{
					PyLog("[AUTH_BRAZIL] : Succeed to create a new account {}", pinfo->login) ;

					ReturnQuery(QID_AUTH_LOGIN, qi->dwIdent, pinfo,
							"SELECT PASSWORD('%s'),password,securitycode,social_id,id,status,availDt - NOW() > 0,"
							"UNIX_TIMESTAMP(silver_expire),"
							"UNIX_TIMESTAMP(gold_expire),"
							"UNIX_TIMESTAMP(safebox_expire),"
							"UNIX_TIMESTAMP(autoloot_expire),"
							"UNIX_TIMESTAMP(fish_mind_expire),"
							"UNIX_TIMESTAMP(marriage_fast_expire),"
							"UNIX_TIMESTAMP(money_drop_rate_expire),"
							"UNIX_TIMESTAMP(create_time)"
							" FROM account WHERE login='%s'",
							pinfo->passwd, pinfo->login) ;
				}
			}
			break;
		case QID_JAPAN_CREATE_ID :
			{
				TPacketCGLogin3* pinfo = (TPacketCGLogin3 *) qi->pvData ;

				if(pMsg->Get()->uiAffectedRows == 0 || pMsg->Get()->uiAffectedRows == (uint32_t)-1)
				{
					LPDESC d = DESC_MANAGER::GetInstance()->FindByLoginKey(qi->dwIdent) ;
					PyLog("[AUTH_JAPAN]   NOID") ;
					PyLog("[AUTH_JAPAN] : Failed to create a new account {}", pinfo->login) ;
					LoginFailure(d, "NOID") ;
					M2_DELETE(pinfo);
				}
				else
				{
					PyLog("[AUTH_JAPAN] : Succeed to create a new account {}", pinfo->login) ;

					ReturnQuery(QID_AUTH_LOGIN_OPENID, qi->dwIdent, pinfo,
							"SELECT PASSWORD('%s'),password,securitycode,social_id,id,status,availDt - NOW() > 0,"
							"UNIX_TIMESTAMP(silver_expire),"
							"UNIX_TIMESTAMP(gold_expire),"
							"UNIX_TIMESTAMP(safebox_expire),"
							"UNIX_TIMESTAMP(autoloot_expire),"
							"UNIX_TIMESTAMP(fish_mind_expire),"
							"UNIX_TIMESTAMP(marriage_fast_expire),"
							"UNIX_TIMESTAMP(money_drop_rate_expire),"
							"UNIX_TIMESTAMP(create_time)"
							" FROM account WHERE login='%s'",
							pinfo->passwd, pinfo->login) ;
				}
			}
			break;

		default:
			SysLog("FATAL ERROR!!! Unhandled return query id {}", qi->iType);
			break;
	}

	M2_DELETE(qi);
}

void DBManager::LoadDBString()
{
	ReturnQuery(QID_DB_STRING, 0, NULL, "SELECT name, text FROM string%s", get_table_postfix());
}

const std::string& DBManager::GetDBString(const std::string& key)
{
	static std::string null_str = "";
	auto it = m_map_dbstring.find(key);
	if (it == m_map_dbstring.end())
		return null_str;
	return it->second;
}

const std::vector<std::string>& DBManager::GetGreetMessage()
{
	return m_vec_GreetMessage;
}

void DBManager::SendMoneyLog(uint8_t type, uint32_t vnum, int32_t gold)
{
	if (!gold)
		return;
	TPacketMoneyLog p;
	p.type = type;
	p.vnum = vnum;
	p.gold = gold;
	db_clientdesc->DBPacket(HEADER_GD_MONEY_LOG, 0, &p, sizeof(p));
}

size_t DBManager::EscapeString(char* dst, size_t dstSize, const char* src, size_t srcSize)
{
	return m_sql_direct.EscapeString(dst, dstSize, src, srcSize);
}

//
// Common SQL 
//
AccountDB::AccountDB() :
	m_IsConnect(false)
{
}

bool AccountDB::IsConnected()
{
	return m_IsConnect;
}

bool AccountDB::Connect(const char* host, const int32_t port, const char* user, const char* pwd, const char* db)
{
	m_IsConnect = m_sql_direct.Setup(host, user, pwd, db, "", true, port);

	if (!m_IsConnect)
	{
		SysLog("cannot open direct sql connection to host: {} user: {} db: {}\n", host, user, db);
		return false;
	}

	return m_IsConnect;
}

bool AccountDB::ConnectAsync(const char* host, const int32_t port, const char* user, const char* pwd, const char* db, const char* locale)
{
	m_sql.Setup(host, user, pwd, db, locale, false, port);
	return true;
}

void AccountDB::SetLocale(const std::string & stLocale)
{
	m_sql_direct.SetLocale(stLocale);
	m_sql_direct.QueryLocaleSet();
}

SQLMsg* AccountDB::DirectQuery(const char* query)
{
	return m_sql_direct.DirectQuery(query);
}

void AccountDB::AsyncQuery(const char* query)
{
	m_sql.AsyncQuery(query);
}

void AccountDB::ReturnQuery(int32_t iType, uint32_t dwIdent, void* pvData, const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

	CReturnQueryInfo* p = M2_NEW CReturnQueryInfo;

	p->iQueryType = QUERY_TYPE_RETURN;
	p->iType = iType;
	p->dwIdent = dwIdent;
	p->pvData = pvData;

	m_sql.ReturnQuery(szQuery, p);
}

SQLMsg * AccountDB::PopResult()
{
	SQLMsg* p;

	if (m_sql.PopResult(&p))
		return p;

	return NULL;
}

void AccountDB::Process()
{
	SQLMsg* pMsg = nullptr;

	while ((pMsg = PopResult()))
	{
		CQueryInfo* qi = (CQueryInfo *) pMsg->pvUserData;

		switch (qi->iQueryType)
		{
			case QUERY_TYPE_RETURN:
				AnalyzeReturnQuery(pMsg);
				break;
		}
	}

	delete pMsg;
}

extern uint32_t g_uiSpamReloadCycle;

enum EAccountQID
{
	QID_SPAM_DB,
};

// 10 reload every minute
static LPEVENT s_pReloadSpamEvent = nullptr;

EVENTINFO(reload_spam_event_info)
{
	// used to send command
	uint32_t empty;
};

EVENTFUNC(reload_spam_event)
{
	AccountDB::GetInstance()->ReturnQuery(QID_SPAM_DB, 0, NULL, "SELECT word, score FROM spam_db WHERE type='SPAM'");
	return PASSES_PER_SEC(g_uiSpamReloadCycle);
}

void LoadSpamDB()
{
	AccountDB::GetInstance()->ReturnQuery(QID_SPAM_DB, 0, NULL, "SELECT word, score FROM spam_db WHERE type='SPAM'");

	if (!s_pReloadSpamEvent)
	{
		reload_spam_event_info* info = AllocEventInfo<reload_spam_event_info>();
		s_pReloadSpamEvent = event_create(reload_spam_event, info, PASSES_PER_SEC(g_uiSpamReloadCycle));
	}
}

void CancelReloadSpamEvent() {
	s_pReloadSpamEvent = nullptr;
}

void AccountDB::AnalyzeReturnQuery(SQLMsg* pMsg)
{
	CReturnQueryInfo * qi = (CReturnQueryInfo *) pMsg->pvUserData;

	switch (qi->iType)
	{
		case QID_SPAM_DB:
			{
				if (pMsg->Get()->uiNumRows > 0)
				{
					MYSQL_ROW row;

					SpamManager::GetInstance()->Clear();

					while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
						SpamManager::GetInstance()->Insert(row[0], atoi(row[1]));
				}
			}
			break;
	}

	M2_DELETE(qi);
}
