#include "stdafx.h"
#include "MoneyLog.h"
#include "ClientManager.h"
#include "Peer.h"

CMoneyLog::CMoneyLog()
{
}

CMoneyLog::~CMoneyLog()
{
}

void CMoneyLog::Save()
{
	LPPEER pPeer = CClientManager::GetInstance()->GetAnyPeer();
	if (!pPeer)
		return;

	for (uint8_t bType = 0; bType < MONEY_LOG_TYPE_MAX_NUM; bType ++)
	{
		for (auto it = m_MoneyLogContainer[bType].begin(); it != m_MoneyLogContainer[bType].end(); ++it)
		{
			TPacketMoneyLog p;
			p.type = bType;
			p.vnum = it->first;
			p.gold = it->second;
			pPeer->EncodeHeader(HEADER_DG_MONEY_LOG, 0, sizeof(p));
			pPeer->Encode(&p, sizeof(p));
		}
		m_MoneyLogContainer[bType].clear();
	}
}

void CMoneyLog::AddLog(uint8_t bType, uint32_t dwVnum, int32_t iGold)
{
	m_MoneyLogContainer[bType][dwVnum] += iGold;
}
