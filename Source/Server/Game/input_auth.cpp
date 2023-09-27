#include "stdafx.h" 
#include <Core/Net/PacketsCG.hpp>
#include "constants.h"
#include "config.h"
#include "input.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "protocol.h"
#include "locale_service.h"
#include "db.h"

extern time_t get_global_time();
extern int32_t openid_server;

bool FN_IS_VALID_LOGIN_STRING(const char* str)
{
	const char*	tmp;

	if (!str || !*str)
		return false;

	if (strlen(str) < 2)
		return false;

	for (tmp = str; *tmp; ++tmp)
	{
		if (isdigit(*tmp) || isalpha(*tmp))
			continue;

		return false;
	}

	return true;
}

bool Login_IsInChannelService(const char* c_login)
{
	if (c_login[0] == '[')
		return true;
	return false;
}

CInputAuth::CInputAuth()
{
}

void CInputAuth::Login(LPDESC d, const char* c_pData)
{
	TPacketCGLogin3* pinfo = (TPacketCGLogin3 *) c_pData;

	if (!g_bAuthServer)
	{
		SysLog("CInputAuth class is not for game server. IP {} might be a hacker.", 
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return;
	}

	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	char passwd[PASSWD_MAX_LEN + 1];
	strlcpy(passwd, pinfo->passwd, sizeof(passwd));

	PyLog("InputAuth::Login : {} ({})",
			login, strlen(login));

	if (!FN_IS_VALID_LOGIN_STRING(login))
	{
		PyLog("InputAuth::Login : IS_NOT_VALID_LOGIN_STRING({})",
				login);
		LoginFailure(d, "NOID");
		return;
	}

	if (g_bNoMoreClient)
	{
		TPacketGCLoginFailure failurePacket;

		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));

		d->Packet(&failurePacket, sizeof(failurePacket));
		return;
	}

	if (DESC_MANAGER::GetInstance()->FindByLoginName(login))
	{
		LoginFailure(d, "ALREADY");
		return;
	}

	uint32_t dwKey = DESC_MANAGER::GetInstance()->CreateLoginKey(d);

	PyLog("InputAuth::Login : key {} login {}", dwKey, login);

	TPacketCGLogin3* p = M2_NEW TPacketCGLogin3;
	memcpy(p, pinfo, sizeof(TPacketCGLogin3));

	char szPasswd[PASSWD_MAX_LEN * 2 + 1];
	DBManager::GetInstance()->EscapeString(szPasswd, sizeof(szPasswd), passwd, strlen(passwd));

	char szLogin[LOGIN_MAX_LEN * 2 + 1];
	DBManager::GetInstance()->EscapeString(szLogin, sizeof(szLogin), login, strlen(login));

	if (Login_IsInChannelService(szLogin))
	{
		PyLog("ChannelServiceLogin [{}]", szLogin);

		DBManager::GetInstance()->ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
				"SELECT '%s',password,securitycode,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",

				szPasswd, szLogin);
	}
	else
	{
		DBManager::GetInstance()->ReturnQuery(QID_AUTH_LOGIN, dwKey, p, 
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
				szPasswd, szLogin);
	}
}

void CInputAuth::LoginOpenID(LPDESC d, const char* c_pData)
{
	TPacketCGLogin5 *tempInfo1 = (TPacketCGLogin5 *)c_pData;

	char* authKey = tempInfo1->authKey;
	char returnID[LOGIN_MAX_LEN + 1] = {0};

	int32_t test_url_get_protocol = auth_OpenID(authKey, inet_ntoa(d->GetAddr().sin_addr), returnID);

	if (0!=test_url_get_protocol)
	{
		LoginFailure(d, "OpenID Fail");
		return;
	}

	TPacketCGLogin3 tempInfo2;
	strncpy(tempInfo2.login, returnID, LOGIN_MAX_LEN);
	strncpy(tempInfo2.passwd, "0000", PASSWD_MAX_LEN);
	for(int32_t i=0;i<4;i++)
		tempInfo2.adwClientKey[i] = tempInfo1->adwClientKey[i];
	TPacketCGLogin3* pinfo = &tempInfo2;

	if (!g_bAuthServer)
	{
		SysLog("CInputAuth class is not for game server. IP {} might be a hacker.", 
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return;
	}

	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	char passwd[PASSWD_MAX_LEN + 1];
	strlcpy(passwd, pinfo->passwd, sizeof(passwd));

	PyLog("InputAuth::Login : {} ({})",
			login, strlen(login));

	if (!FN_IS_VALID_LOGIN_STRING(login))
	{
		PyLog("InputAuth::Login : IS_NOT_VALID_LOGIN_STRING({})",
				login);
		LoginFailure(d, "NOID");
		return;
	}

	if (g_bNoMoreClient)
	{
		TPacketGCLoginFailure failurePacket;

		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));

		d->Packet(&failurePacket, sizeof(failurePacket));
		return;
	}

	if (DESC_MANAGER::GetInstance()->FindByLoginName(login))
	{
		LoginFailure(d, "ALREADY");
		return;
	}

	uint32_t dwKey = DESC_MANAGER::GetInstance()->CreateLoginKey(d);

	PyLog("InputAuth::Login : key {} login {}", dwKey, login);

	TPacketCGLogin3* p = M2_NEW TPacketCGLogin3;
	memcpy(p, pinfo, sizeof(TPacketCGLogin3));

	char szPasswd[PASSWD_MAX_LEN * 2 + 1];
	DBManager::GetInstance()->EscapeString(szPasswd, sizeof(szPasswd), passwd, strlen(passwd));

	char szLogin[LOGIN_MAX_LEN * 2 + 1];
	DBManager::GetInstance()->EscapeString(szLogin, sizeof(szLogin), login, strlen(login));

	if (Login_IsInChannelService(szLogin))
	{
		PyLog("ChannelServiceLogin [{}]", szLogin);

		DBManager::GetInstance()->ReturnQuery(QID_AUTH_LOGIN_OPENID, dwKey, p,
				"SELECT '%s',password,securitycode,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",

				szPasswd, szLogin);
	}
	else
	{
		DBManager::GetInstance()->ReturnQuery(QID_AUTH_LOGIN_OPENID, dwKey, p, 
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
				szPasswd, szLogin);
	}
}

extern void socket_timeout(socket_t s, long sec, long usec);

int32_t CInputAuth::auth_OpenID(const char* authKey, const char* ipAddr, char* rID)
{
	extern char openid_host[256];
	extern char openid_uri[256];

    int32_t		port	= 80;

    socket_t fd = socket_connect(openid_host, port);
    if (fd < 0)
    {
		SysLog("[auth_OpenID] : could not connect to OpenID server({})", openid_host);
		return 1;
    }

    socket_block(fd);
    socket_timeout(fd, 3, 0);

    {
		char request[512];
		int32_t len = snprintf(request, sizeof(request),
						"GET %s?auth_key=%s&ip=%s HTTP/1.1\r\n"
						"Host: %s\r\n"
						"Connection: Close\r\n\r\n",
						openid_uri, authKey, ipAddr, openid_host);

		if (socket_write(fd, request, len) < 0)
		{
			SysLog("[auth_OpenID] : could not send auth-request ({})", authKey);
			socket_close(fd);
			return 2;
		}
    }

    char reply[1024] = {0};
	int32_t len = socket_read(fd, reply, sizeof(reply));
	socket_close(fd);

	if (len <= 0)
	{
	    SysLog("[auth_OpenID] : could not recv auth-reply ({})", authKey);
	    return 3;
	}

	char buffer[1024];
	strcpy(buffer, reply);

	const char* delim = "\r\n";
	char* last = 0;
	char* v = strtok(buffer, delim);
	char* result = 0;

	while (v)
	{
		result = v;
		v = strtok(NULL, delim);
	}


	char* id = strtok(result, "%");
	char* success = strtok(NULL, "%");

	if (!*id || !*success)
	{
	    SysLog("[auth_OpenID] : OpenID AuthServer Reply Error ({})", reply);
		return 4;
	}

	if (0 != strcmp("OK", success))
	{
		int32_t returnNumber = 0;
		str_to_number(returnNumber, id);
		switch (returnNumber)
		{
		case 1:
			SysLog("[auth_OpenID] : AuthKey incorrect");
			break;
		case 2:
			SysLog("[auth_OpenID] : ip incorrect");
			break;
		case 3:
			SysLog("[auth_OpenID] : used AuthKey");
			break;
		case 4:
			SysLog("[auth_OpenID] : AuthKey not delivered");
			break;
		case 5:
			SysLog("[auth_OpenID] : ip not delivered");
			break;
		case 6:
			SysLog("[auth_OpenID] : AuthKey time over");
			break;
		default:
			break;

		return 5;
		}
	}

	strcpy(rID, id);

	return 0;
}


int32_t CInputAuth::Analyze(LPDESC d, uint8_t bHeader, const char* c_pData)
{

	if (!g_bAuthServer)
	{
		SysLog("CInputAuth class is not for game server. IP {} might be a hacker.", 
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return 0;
	}

	int32_t iExtraLen = 0;

	if (test_server)
		PyLog(" InputAuth Analyze Header[{}] ", bHeader);

	switch (bHeader)
	{
		case HEADER_CG_PONG:
			Pong(d);
			break;

		case HEADER_CG_LOGIN3:
			Login(d, c_pData);
			break;

		case HEADER_CG_LOGIN5_OPENID:
			if (openid_server)
				LoginOpenID(d, c_pData);
			else
				SysLog("HEADER_CG_LOGIN5_OPENID : wrong client access");
			break;

		case HEADER_CG_HANDSHAKE:
			break;

		default:
			SysLog("This phase does not handle this header {} (0x{})(phase: AUTH)", bHeader, bHeader);
			break;
	}

	return iExtraLen;
}

