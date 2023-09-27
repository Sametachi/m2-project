#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "AccountConnector.h"

// Login ---------------------------------------------------------------------------
void CPythonNetworkStream::LoginPhase()
{
	TPacketHeader header;
	if (!CheckPacket(&header))
		return;

	switch (header)
	{
		case HEADER_GC_PHASE:
			if (RecvPhasePacket())
				return;
			break;

		case HEADER_GC_LOGIN_SUCCESS:
			if (__RecvLoginSuccessPacket())
				return;
			break;

		case HEADER_GC_LOGIN_FAILURE:
			if (__RecvLoginFailurePacket())
				return;		
			break;

		case HEADER_GC_EMPIRE:
			if (__RecvEmpirePacket())
				return;
			break;

		case HEADER_GC_LOGIN_KEY:
			if (__RecvLoginKeyPacket())
				return;
			break;

		case HEADER_GC_PING:
			if (RecvPingPacket())
				return;
			break;

		default:
			if (RecvDefaultPacket(header))
				return;
			break;
	}

	RecvErrorPacket(header);
}

void CPythonNetworkStream::SetLoginPhase()
{
#if 0
	const char* key = LocaleService_GetSecurityKey();
	SetSecurityMode(true, key);
#endif

	if ("Login" != m_strPhase)
		m_phaseLeaveFunc.Run();

	TraceLog("## Network - Login Phase ##");

	m_strPhase = "Login";	

	m_phaseProcessFunc.Set(this, &CPythonNetworkStream::LoginPhase);
	m_phaseLeaveFunc.Set(this, &CPythonNetworkStream::__LeaveLoginPhase);

	m_dwChangingPhaseTime = ELTimer_GetMSec();

	if (__DirectEnterMode_IsSet())
	{
		if (0 != m_dwLoginKey)
			SendLoginPacketNew(m_stID.c_str(), m_stPassword.c_str());
		else
			SendLoginPacket(m_stID.c_str(), m_stPassword.c_str());

		// ��й�ȣ�� �޸𸮿� ��� ���� �ִ� ������ �־, ��� ��� ������ ������ ����
		ClearLoginInfo();
		auto rkAccountConnector = CAccountConnector::GetInstance();
		rkAccountConnector->ClearLoginInfo();
	}
	else
	{
		if (0 != m_dwLoginKey)
			SendLoginPacketNew(m_stID.c_str(), m_stPassword.c_str());
		else
			SendLoginPacket(m_stID.c_str(), m_stPassword.c_str());

		// ��й�ȣ�� �޸𸮿� ��� ���� �ִ� ������ �־, ��� ��� ������ ������ ����
		ClearLoginInfo();
		auto rkAccountConnector = CAccountConnector::GetInstance();
		rkAccountConnector->ClearLoginInfo();

		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnLoginStart");

		__ClearSelectCharacterData();
	}
}

bool CPythonNetworkStream::__RecvEmpirePacket()
{
	TPacketGCEmpire kPacketEmpire;
	if (!Recv(sizeof(kPacketEmpire), &kPacketEmpire))
		return false;

	m_dwEmpireID=kPacketEmpire.bEmpire;
	return true;
}

bool CPythonNetworkStream::__RecvLoginSuccessPacket()
{
	TPacketGCLoginSuccess kPacketLoginSuccess;

	if (!Recv(sizeof(kPacketLoginSuccess), &kPacketLoginSuccess))
		return false;	
	
	for (int32_t i = 0; i< PLAYER_PER_ACCOUNT; ++i)
	{
		m_akSimplePlayerInfo[i]=kPacketLoginSuccess.players[i];
		m_adwGuildID[i]=kPacketLoginSuccess.guild_id[i];
		m_astrGuildName[i]=kPacketLoginSuccess.guild_name[i];
	}

	m_kMarkAuth.m_dwHandle=kPacketLoginSuccess.handle;
	m_kMarkAuth.m_dwRandomKey=kPacketLoginSuccess.random_key;	

	if (__DirectEnterMode_IsSet())
	{
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "Refresh");		
	}

	return true;
}


void CPythonNetworkStream::OnConnectFailure()
{
	if (__DirectEnterMode_IsSet())
	{
		ClosePhase();
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnConnectFailure");	
	}
}


bool CPythonNetworkStream::__RecvLoginFailurePacket()
{
	TPacketGCLoginFailure packet_failure;
	if (!Recv(sizeof(TPacketGCLoginFailure), &packet_failure))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnLoginFailure", packet_failure.szStatus);
	ConsoleLog(" RecvLoginFailurePacket : [{}]\n", packet_failure.szStatus);
	return true;
}

bool CPythonNetworkStream::SendLoginPacket(const char* c_szName, const char* c_szPassword)
{
	TPacketCGLogin LoginPacket;
	LoginPacket.header = HEADER_CG_LOGIN;

	strncpy(LoginPacket.login, c_szName, sizeof(LoginPacket.login)-1);
	strncpy(LoginPacket.passwd, c_szPassword, sizeof(LoginPacket.passwd)-1);

	LoginPacket.login[ID_MAX_NUM]='\0';
	LoginPacket.passwd[PASS_MAX_NUM]='\0';

	if (!Send(sizeof(LoginPacket), &LoginPacket))
	{
		TraceLog("SendLogin Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendLoginPacketNew(const char * c_szName, const char * c_szPassword)
{
	TPacketCGLogin2 LoginPacket;
	LoginPacket.header = HEADER_CG_LOGIN2;
	LoginPacket.dwLoginKey = m_dwLoginKey;

	strncpy(LoginPacket.login, c_szName, sizeof(LoginPacket.login)-1);
	LoginPacket.login[ID_MAX_NUM]='\0';

	extern uint32_t g_adwEncryptKey[4];
	extern uint32_t g_adwDecryptKey[4];
	for (uint32_t i = 0; i < 4; ++i)
		LoginPacket.adwClientKey[i] = g_adwEncryptKey[i];

	if (!Send(sizeof(LoginPacket), &LoginPacket))
	{
		TraceLog("SendLogin Error");
		return false;
	}

	__SendInternalBuffer();

	//SetSecurityMode(true, (const char *) g_adwEncryptKey, (const char *) g_adwDecryptKey);
	return true;
}

bool CPythonNetworkStream::__RecvLoginKeyPacket()
{
	TPacketGCLoginKey kLoginKeyPacket;
	if (!Recv(sizeof(TPacketGCLoginKey), &kLoginKeyPacket))
		return false;

	m_dwLoginKey = kLoginKeyPacket.dwLoginKey;
	m_isWaitLoginKey = FALSE;

	return true;
}
