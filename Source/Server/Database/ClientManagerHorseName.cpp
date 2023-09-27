#include "stdafx.h"
#include "ClientManager.h"

void CClientManager::UpdateHorseName(TPacketUpdateHorseName* data, LPPEER pPeer)
{
	char szQuery[512];

	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO horse_name VALUES(%u, '%s')", data->dwPlayerID, data->szHorseName);

	std::unique_ptr<SQLMsg> pmsg_insert(CDBManager::GetInstance()->DirectQuery(szQuery));

	ForwardPacket(HEADER_DG_UPDATE_HORSE_NAME, data, sizeof(TPacketUpdateHorseName), 0, pPeer);
}

void CClientManager::AckHorseName(uint32_t dwPID, LPPEER pPeer)
{
	char szQuery[512];

	snprintf(szQuery, sizeof(szQuery), "SELECT name FROM horse_name WHERE id = %u", dwPID);

	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	TPacketUpdateHorseName packet;
	packet.dwPlayerID = dwPID;

	if (pmsg->Get()->uiNumRows == 0)
	{
		memset(packet.szHorseName, 0, sizeof (packet.szHorseName));
	}
	else
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);
		strlcpy(packet.szHorseName, row[0], sizeof(packet.szHorseName));
	}

	pPeer->EncodeHeader(HEADER_DG_ACK_HORSE_NAME, 0, sizeof(TPacketUpdateHorseName));
	pPeer->Encode(&packet, sizeof(TPacketUpdateHorseName));
}

