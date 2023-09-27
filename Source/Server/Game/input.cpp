#include "stdafx.h"
#include <Core/Net/PacketsCG.hpp>
#include <sstream>

#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "buffer_manager.h"
#include "config.h"
#include "p2p.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "login_sim.h"
#include "fishing.h"
#include "priv_manager.h"
#include "castle.h"

extern time_t get_global_time();

bool IsEmptyAdminPage()
{
	return g_stAdminPageIP.empty();
}

bool IsAdminPage(const char* ip)
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
	{
		if (g_stAdminPageIP[n] == ip)
			return 1; 
	}	
	return 0;
}

void ClearAdminPages()
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
		g_stAdminPageIP[n].clear();

	g_stAdminPageIP.clear();
}

CInputProcessor::CInputProcessor() : m_pPacketInfo(nullptr), m_iBufferLeft(0)
{
	if (!m_pPacketInfo)
		BindPacketInfo(&m_packetInfoCG);
}

void CInputProcessor::BindPacketInfo(CPacketInfo* pPacketInfo)
{
	m_pPacketInfo = pPacketInfo;
}

bool CInputProcessor::Process(LPDESC lpDesc, const void* c_pvOrig, int32_t iBytes, int32_t& r_iBytesProceed)
{
	const char* c_pData = (const char* ) c_pvOrig;

	uint8_t	bLastHeader = 0;
	int32_t		iLastPacketLen = 0;
	int32_t		iPacketLen;

	if (!m_pPacketInfo)
	{
		SysLog("No packet info has been binded to");
		return true;
	}

	for (m_iBufferLeft = iBytes; m_iBufferLeft > 0;)
	{
		uint8_t bHeader = (uint8_t) *(c_pData);
		const char* c_pszName = "";

		if (bHeader == 0)
			iPacketLen = 1;
		else if (!m_pPacketInfo->Get(bHeader, &iPacketLen, &c_pszName))
		{
			SysLog("UNKNOWN HEADER: {}, LAST HEADER: {}({}), REMAIN BYTES: {}, fd: {}",
					bHeader, bLastHeader, iLastPacketLen, m_iBufferLeft, lpDesc->GetSocket());
			lpDesc->SetPhase(PHASE_CLOSE);
			return true;
		}

		if (m_iBufferLeft < iPacketLen)
			return true;

		if (bHeader)
		{
			if (test_server && bHeader != HEADER_CG_MOVE)
				PyLog("Packet Analyze [Header {}][bufferLeft {}] ", bHeader, m_iBufferLeft);

			m_pPacketInfo->Start();

			int32_t iExtraPacketSize = Analyze(lpDesc, bHeader, c_pData);

			if (iExtraPacketSize < 0)
				return true;

			iPacketLen += iExtraPacketSize;
			lpDesc->Log("%s %d", c_pszName, iPacketLen);
			m_pPacketInfo->End();
		}

		if (bHeader == HEADER_CG_PONG)
			PyLog("PONG! {}", *(uint8_t*) (c_pData + iPacketLen - sizeof(uint8_t)));

		c_pData	+= iPacketLen;
		m_iBufferLeft -= iPacketLen;
		r_iBytesProceed += iPacketLen;

		iLastPacketLen = iPacketLen;
		bLastHeader	= bHeader;

		if (GetType() != lpDesc->GetInputProcessor()->GetType())
			return false;
	}

	return true;
}

void CInputProcessor::Pong(LPDESC d)
{
	d->SetPong(true);
}

void CInputProcessor::Handshake(LPDESC d, const char* c_pData)
{
	TPacketCGHandshake* p = (TPacketCGHandshake *) c_pData;

	if (d->GetHandshake() != p->dwHandshake)
	{
		SysLog("Invalid Handshake on {}", d->GetSocket());
		d->SetPhase(PHASE_CLOSE);
	}
	else
	{
		if (d->IsPhase(PHASE_HANDSHAKE))
		{
			if (d->HandshakeProcess(p->dwTime, p->lDelta, false))
			{
				if (g_bAuthServer)
					d->SetPhase(PHASE_AUTH);
				else
					d->SetPhase(PHASE_LOGIN);
			}
		}
		else
			d->HandshakeProcess(p->dwTime, p->lDelta, true);
	}
}

void LoginFailure(LPDESC d, const char* c_pszStatus)
{
	if (!d)
		return;

	TPacketGCLoginFailure failurePacket;

	failurePacket.header = HEADER_GC_LOGIN_FAILURE;
	strlcpy(failurePacket.szStatus, c_pszStatus, sizeof(failurePacket.szStatus));

	d->Packet(&failurePacket, sizeof(failurePacket));
}

CInputHandshake::CInputHandshake()
{
	CPacketInfoCG* pPacketInfo = M2_NEW CPacketInfoCG;

	m_pMainPacketInfo = m_pPacketInfo;
	BindPacketInfo(pPacketInfo);
}

CInputHandshake::~CInputHandshake()
{
	if(NULL != m_pPacketInfo)
	{
		M2_DELETE(m_pPacketInfo);
		m_pPacketInfo = nullptr;
	}
}


std::map<uint32_t, CLoginSim *> g_sim;
std::map<uint32_t, CLoginSim *> g_simByPID;
std::vector<TPlayerTable> g_vec_save;

ACMD(do_block_chat);

int32_t CInputHandshake::Analyze(LPDESC d, uint8_t bHeader, const char* c_pData)
{
	if (bHeader == 10)
		return 0;

	if (bHeader == HEADER_CG_TEXT)
	{
		++c_pData;
		const char* c_pSep;

		if (!(c_pSep = strchr(c_pData, '\n')))
			return -1;

		if (*(c_pSep - 1) == '\r')
			--c_pSep;

		std::string stResult;
		std::string stBuf;
		stBuf.assign(c_pData, 0, c_pSep - c_pData);

		PyLog("SOCKET_CMD: FROM({}) CMD({})", d->GetHostName(), stBuf.c_str());

		if (!stBuf.compare("IS_SERVER_UP"))
		{
			if (g_bNoMoreClient)
				stResult = "NO";
			else
				stResult = "YES";
		}
		else if (stBuf == g_stAdminPagePassword)
		{
			if (!IsEmptyAdminPage())
			{
				if (!IsAdminPage(inet_ntoa(d->GetAddr().sin_addr)))
				{
					char szTmp[64];
					snprintf(szTmp, sizeof(szTmp), "WEBADMIN : Wrong Connector : %s", inet_ntoa(d->GetAddr().sin_addr));
					stResult += szTmp;
				}
				else
				{
					d->SetAdminMode();
					stResult = "UNKNOWN";
				}
			}
			else
			{
				d->SetAdminMode();
				stResult = "UNKNOWN";
			}
		}
		else if (!stBuf.compare("USER_COUNT"))
		{
			char szTmp[64];

			if (!IsEmptyAdminPage())
			{
				if (!IsAdminPage(inet_ntoa(d->GetAddr().sin_addr)))
				{
					snprintf(szTmp, sizeof(szTmp), "WEBADMIN : Wrong Connector : %s", inet_ntoa(d->GetAddr().sin_addr));
				}
				else
				{
					int32_t iTotal;
					int32_t* paiEmpireUserCount;
					int32_t iLocal;
					DESC_MANAGER::GetInstance()->GetUserCount(iTotal, &paiEmpireUserCount, iLocal);
					snprintf(szTmp, sizeof(szTmp), "%d %d %d %d %d", iTotal, paiEmpireUserCount[1], paiEmpireUserCount[2], paiEmpireUserCount[3], iLocal);
				}
			}
			else
			{
				int32_t iTotal;
				int32_t* paiEmpireUserCount;
				int32_t iLocal;
				DESC_MANAGER::GetInstance()->GetUserCount(iTotal, &paiEmpireUserCount, iLocal);
				snprintf(szTmp, sizeof(szTmp), "%d %d %d %d %d", iTotal, paiEmpireUserCount[1], paiEmpireUserCount[2], paiEmpireUserCount[3], iLocal);
			}
			stResult += szTmp;
		}
		else if (!stBuf.compare("CHECK_P2P_CONNECTIONS"))
		{
			std::ostringstream oss(std::ostringstream::out);
			
			oss << "P2P CONNECTION NUMBER : " << P2P_MANAGER::GetInstance()->GetDescCount() << "\n";
			std::string hostNames;
			P2P_MANAGER::GetInstance()->GetP2PHostNames(hostNames);
			oss << hostNames;
			stResult = oss.str();
			TPacketGGCheckAwakeness packet;
			packet.bHeader = HEADER_GG_CHECK_AWAKENESS;

			P2P_MANAGER::GetInstance()->Send(&packet, sizeof(packet));
		}
		else if (!stBuf.compare("PACKET_INFO"))
		{
			m_pMainPacketInfo->Log("packet_info.txt");
			stResult = "OK";
		}
		else
		{
			stResult = "UNKNOWN";
			
			if (d->IsAdminMode())
			{
				if (!stBuf.compare(0, 7, "NOTICE "))
				{
					std::string msg = stBuf.substr(7, 50);
					LogManager::GetInstance()->CharLog(0, 0, 0, 1, "NOTICE", msg.c_str(), d->GetHostName());
					BroadcastNotice(msg.c_str());
				}
				else if (!stBuf.compare("SHUTDOWN"))
				{
					LogManager::GetInstance()->CharLog(0, 0, 0, 2, "SHUTDOWN", "", d->GetHostName());
					TPacketGGShutdown p;
					p.bHeader = HEADER_GG_SHUTDOWN;
					P2P_MANAGER::GetInstance()->Send(&p, sizeof(TPacketGGShutdown));
					SysLog("Accept shutdown command from {}.", d->GetHostName());
					Shutdown(10);
				}
				else if (!stBuf.compare("SHUTDOWN_ONLY"))
				{
					LogManager::GetInstance()->CharLog(0, 0, 0, 2, "SHUTDOWN", "", d->GetHostName());
					SysLog("Accept shutdown only command from {}.", d->GetHostName());
					Shutdown(10);
				}
				else if (!stBuf.compare(0, 3, "DC "))
				{
					std::string msg = stBuf.substr(3, LOGIN_MAX_LEN);

					TPacketGGDisconnect pgg;

					pgg.bHeader = HEADER_GG_DISCONNECT;
					strlcpy(pgg.szLogin, msg.c_str(), sizeof(pgg.szLogin));

					P2P_MANAGER::GetInstance()->Send(&pgg, sizeof(TPacketGGDisconnect));

					{
						TPacketDC p;
						strlcpy(p.login, msg.c_str(), sizeof(p.login));
						db_clientdesc->DBPacket(HEADER_GD_DC, 0, &p, sizeof(p));
					}
				}
				else if (!stBuf.compare(0, 10, "RELOAD_CRC"))
				{
					LoadValidCRCList();

					uint8_t bHeader = HEADER_GG_RELOAD_CRC_LIST;
					P2P_MANAGER::GetInstance()->Send(&bHeader, sizeof(uint8_t));
					stResult = "OK";
				}
				else if (!stBuf.compare(0, 6, "RELOAD"))
				{
					if (stBuf.size() == 6)
					{
						LoadStateUserCount();
						db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);
						DBManager::GetInstance()->LoadDBString();
					}
					else
					{
						char c = stBuf[7];

						switch (LOWER(c))
						{
							case 'u':
								LoadStateUserCount();
								break;

							case 'p':
								db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);
								break;

							case 's':
								DBManager::GetInstance()->LoadDBString();
								break;

							case 'q':
								quest::CQuestManager::GetInstance()->Reload();
								break;

							case 'f':
								fishing::Initialize();
								break;

							case 'a':
								db_clientdesc->DBPacket(HEADER_GD_RELOAD_ADMIN, 0, NULL, 0);
								PyLog("Reloading admin infomation.");
								break;
						}
					}
				}
				else if (!stBuf.compare(0, 6, "EVENT "))
				{
					std::istringstream is(stBuf);
					std::string strEvent, strFlagName;
					int32_t lValue;
					is >> strEvent >> strFlagName >> lValue;

					if (!is.fail())
					{
						PyLog("EXTERNAL EVENT FLAG name {} value {}", strFlagName.c_str(), lValue);
						quest::CQuestManager::GetInstance()->RequestSetEventFlag(strFlagName, lValue);
						stResult = "EVENT FLAG CHANGE ";
						stResult += strFlagName;
					}
					else
					{
						stResult = "EVENT FLAG FAIL";
					}
				}
				else if (!stBuf.compare(0, 11, "BLOCK_CHAT "))
				{
					std::istringstream is(stBuf);
					std::string strBlockChat, strCharName;
					int32_t lDuration;
					is >> strBlockChat >> strCharName >> lDuration;

					if (!is.fail())
					{
						PyLog("EXTERNAL BLOCK_CHAT name {} duration {}", strCharName.c_str(), lDuration);

						do_block_chat(NULL, const_cast<char*>(stBuf.c_str() + 11), 0, 0);

						stResult = "BLOCK_CHAT ";
						stResult += strCharName;
					}
					else
					{
						stResult = "BLOCK_CHAT FAIL";
					}
				}
				else if (!stBuf.compare(0, 12, "PRIV_EMPIRE "))
				{
					int32_t	empire, type, value, duration;
					std::istringstream is(stBuf);
					std::string strPrivEmpire;
					is >> strPrivEmpire >> empire >> type >> value >> duration;

					value = MINMAX(0, value, 1000);
					stResult = "PRIV_EMPIRE FAIL";

					if (!is.fail())
					{
						if (empire < 0 || 3 < empire);
						else if (type < 1 || 4 < type);
						else if (value < 0);
						else if (duration < 0);
						else
						{
							stResult = "PRIV_EMPIRE SUCCEED";

							duration = duration * (60 * 60);

							PyLog("_give_empire_privileage(empire={}, type={}, value={}, duration={}) by web", 
									empire, type, value, duration);
							CPrivManager::GetInstance()->RequestGiveEmpirePriv(empire, type, value, duration);
						}
					}
				}
			}
		}

		TraceLog("TEXT {} RESULT {}", stBuf.c_str(), stResult.c_str());
		stResult += "\n";
		d->Packet(stResult.c_str(), stResult.length());
		return (c_pSep - c_pData) + 1;
	}
	else if (bHeader == HEADER_CG_MARK_LOGIN)
	{
		if (!guild_mark_server)
		{
			SysLog("Guild Mark login requested but i'm not a mark server!");
			d->SetPhase(PHASE_CLOSE);
			return 0;
		}

		PyLog("MARK_SERVER: Login");
		d->SetPhase(PHASE_LOGIN);
		return 0;
	}
	else if (bHeader == HEADER_CG_STATE_CHECKER)
	{
		if (d->isChannelStatusRequested()) {
			return 0;
		}
		d->SetChannelStatusRequested(true);
		db_clientdesc->DBPacket(HEADER_GD_REQUEST_CHANNELSTATUS, d->GetHandle(), NULL, 0);
	}
	else if (bHeader == HEADER_CG_PONG)
		Pong(d);
	else if (bHeader == HEADER_CG_HANDSHAKE)
		Handshake(d, c_pData);
	else
		SysLog("Handshake phase does not handle packet {} (fd {})", bHeader, d->GetSocket());

	return 0;
}


