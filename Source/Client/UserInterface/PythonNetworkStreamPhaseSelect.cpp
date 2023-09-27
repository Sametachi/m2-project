#include "StdAfx.h"
#include "PythonNetworkStream.h"

extern uint32_t g_adwEncryptKey[4];
extern uint32_t g_adwDecryptKey[4];

// Select Character ---------------------------------------------------------------------------
void CPythonNetworkStream::SetSelectPhase()
{
	if ("Select" != m_strPhase)
		m_phaseLeaveFunc.Run();

	TraceLog("## Network - Select Phase ##");

	m_strPhase = "Select";	

	//SetSecurityMode(true, (const char *) g_adwEncryptKey, (const char *) g_adwDecryptKey);

	m_dwChangingPhaseTime = ELTimer_GetMSec();
	m_phaseProcessFunc.Set(this, &CPythonNetworkStream::SelectPhase);
	m_phaseLeaveFunc.Set(this, &CPythonNetworkStream::__LeaveSelectPhase);

	if (__DirectEnterMode_IsSet())
	{
		PyCallClassMemberFunc(m_poHandler, "_BINARY_DecideLoadingPhase");
	}
	else
	{
		PyCallClassMemberFunc(m_poHandler, "_BINARY_CloseGamePhase");

		if (IsSelectedEmpire())
			PyCallClassMemberFunc(m_poHandler, "SetSelectCharacterPhase");
		else
			PyCallClassMemberFunc(m_poHandler, "SetSelectEmpirePhase");
	}
}

void CPythonNetworkStream::SelectPhase()
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

		case HEADER_GC_EMPIRE:
			if (__RecvEmpirePacket())
				return;
			break;

		case HEADER_GC_LOGIN_SUCCESS:
			if (__RecvLoginSuccessPacket())
				return;
			break;

		case HEADER_GC_CHARACTER_CREATE_SUCCESS:
			if (__RecvPlayerCreateSuccessPacket())
				return;
			break;

		case HEADER_GC_CHARACTER_CREATE_FAILURE:
			if (__RecvPlayerCreateFailurePacket())
				return;
			break;

		case HEADER_GC_CHARACTER_DELETE_WRONG_SOCIAL_ID:
			if (__RecvPlayerDestroyFailurePacket())
				return;
			break;

		case HEADER_GC_CHARACTER_DELETE_SUCCESS:
			if (__RecvPlayerDestroySuccessPacket())
				return;
			break;

		case HEADER_GC_CHANGE_NAME:
			if (__RecvChangeName())
				return;
			break;

		case HEADER_GC_HANDSHAKE:
			RecvHandshakePacket();
			return;
			break;

		case HEADER_GC_TIME_SYNC:
			RecvHandshakeOKPacket();
			return;
			break;

		case HEADER_GC_CHARACTER_POINT_CHANGE:
			TPacketGCPointChange PointChange;
			Recv(sizeof(TPacketGCPointChange), &PointChange);
			return;
			break;

		///////////////////////////////////////////////////////////////////////////////////////////
		case HEADER_GC_PING:
			if (RecvPingPacket())
				return;
			break;
	}

	RecvErrorPacket(header);
}

bool CPythonNetworkStream::SendSelectEmpirePacket(uint32_t dwEmpireID)
{
	TPacketCGEmpire kPacketEmpire;
	kPacketEmpire.bHeader=HEADER_CG_EMPIRE;
	kPacketEmpire.bEmpire=dwEmpireID;

	if (!Send(sizeof(kPacketEmpire), &kPacketEmpire))
	{
		TraceLog("SendSelectEmpirePacket - Error");
		return false;
	}

	SetEmpireID(dwEmpireID);
	return true;
}

bool CPythonNetworkStream::SendSelectCharacterPacket(uint8_t Index)
{
	TPacketCGCharacterSelect SelectCharacterPacket;

	SelectCharacterPacket.header = HEADER_CG_CHARACTER_SELECT;
	SelectCharacterPacket.index = Index;

	if (!Send(sizeof(TPacketCGCharacterSelect), &SelectCharacterPacket))
	{
		TraceLog("SendSelectCharacterPacket - Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendDestroyCharacterPacket(uint8_t index, const char * szPrivateCode)
{
	TPacketCGCharacterDelete DestroyCharacterPacket;

	DestroyCharacterPacket.header = HEADER_CG_CHARACTER_DELETE;
	DestroyCharacterPacket.index = index;
	strncpy(DestroyCharacterPacket.private_code, szPrivateCode, PRIVATE_CODE_LENGTH-1);

	if (!Send(sizeof(TPacketCGCharacterDelete), &DestroyCharacterPacket))
	{
		TraceLog("SendDestroyCharacterPacket");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendCreateCharacterPacket(uint8_t index, const char *name, uint8_t job, uint8_t shape, uint8_t byCON, uint8_t byINT, uint8_t bySTR, uint8_t byDEX)
{
	TPacketCGCharacterCreate createCharacterPacket;

	createCharacterPacket.header = HEADER_CG_CHARACTER_CREATE;
	createCharacterPacket.index = index;
	strncpy(createCharacterPacket.name, name, CHARACTER_NAME_MAX_LEN);
	createCharacterPacket.job = job;
	createCharacterPacket.shape = shape;
	createCharacterPacket.Con = byCON;
	createCharacterPacket.Int = byINT;
	createCharacterPacket.Str = bySTR;
	createCharacterPacket.Dex = byDEX;

	if (!Send(sizeof(TPacketCGCharacterCreate), &createCharacterPacket))
	{
		TraceLog("Failed to SendCreateCharacterPacket");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendChangeNamePacket(uint8_t index, const char *name)
{
	TPacketCGChangeName ChangeNamePacket;
	ChangeNamePacket.header = HEADER_CG_CHANGE_NAME;
	ChangeNamePacket.index = index;
	strncpy(ChangeNamePacket.name, name, CHARACTER_NAME_MAX_LEN);

	if (!Send(sizeof(TPacketCGChangeName), &ChangeNamePacket))
	{
		TraceLog("Failed to SendChangeNamePacket");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::__RecvPlayerCreateSuccessPacket()
{
	TPacketGCPlayerCreateSuccess kCreateSuccessPacket;

	if (!Recv(sizeof(kCreateSuccessPacket), &kCreateSuccessPacket))
		return false;

	if (kCreateSuccessPacket.bAccountCharacterIndex>= PLAYER_PER_ACCOUNT)
	{
		TraceLog("CPythonNetworkStream::RecvPlayerCreateSuccessPacket - OUT OF RANGE SLOT({}) > PLATER_PER_ACCOUNT({})",
			kCreateSuccessPacket.bAccountCharacterIndex, PLAYER_PER_ACCOUNT);
		return true;
	}

	m_akSimplePlayerInfo[kCreateSuccessPacket.bAccountCharacterIndex]=kCreateSuccessPacket.player;
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_CREATE], "OnCreateSuccess");
	return true;
}

bool CPythonNetworkStream::__RecvPlayerCreateFailurePacket()
{
	TPacketGCCreateFailure packet;

	if (!Recv(sizeof(TPacketGCCreateFailure), &packet))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_CREATE], "OnCreateFailure", packet.bType);
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnCreateFailure", packet.bType);
	return true;
}

bool CPythonNetworkStream::__RecvPlayerDestroySuccessPacket()
{
	TPacketGCDestroyCharacterSuccess packet;
	if (!Recv(sizeof(TPacketGCDestroyCharacterSuccess), &packet))
		return false;

	memset(&m_akSimplePlayerInfo[packet.account_index], 0, sizeof(m_akSimplePlayerInfo[packet.account_index]));
	m_adwGuildID[packet.account_index] = 0;
	m_astrGuildName[packet.account_index] = "";

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnDeleteSuccess", packet.account_index);
	return true;
}

bool CPythonNetworkStream::__RecvPlayerDestroyFailurePacket()
{
	TPacketGCBlank packet_blank;
	if (!Recv(sizeof(TPacketGCBlank), &packet_blank))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnDeleteFailure");
	return true;
}

bool CPythonNetworkStream::__RecvChangeName()
{
	TPacketGCChangeName ChangeNamePacket;
	if (!Recv(sizeof(TPacketGCChangeName), &ChangeNamePacket))
		return false;

	for (int32_t i = 0; i < PLAYER_PER_ACCOUNT; ++i)
	{
		if (ChangeNamePacket.pid == m_akSimplePlayerInfo[i].dwID)
		{
			m_akSimplePlayerInfo[i].bChangeName = FALSE;
			strncpy(m_akSimplePlayerInfo[i].szName, ChangeNamePacket.name, CHARACTER_NAME_MAX_LEN);

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnChangeName", i, ChangeNamePacket.name);
			return true;
		}
	}

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnCreateFailure", 100);
	return true;
}
