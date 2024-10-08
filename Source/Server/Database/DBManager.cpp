#include "stdafx.h"
#include "DBManager.h"
#include "ClientManager.h"

extern std::string g_stLocale;

CDBManager::CDBManager()
{
	Initialize();
}

CDBManager::~CDBManager()
{
	Destroy();
}

void CDBManager::Initialize()
{
	for (int32_t i = 0; i < SQL_MAX_NUM; ++i)
	{
		m_mainSQL[i] = nullptr;
		m_directSQL[i] = nullptr;
		m_asyncSQL[i] = nullptr;
	}
}

void CDBManager::Destroy()
{
	Clear();
}

void CDBManager::Clear()
{
	for (int32_t i = 0; i < SQL_MAX_NUM; ++i)
	{
		if (m_mainSQL[i])
		{
			delete m_mainSQL[i];
			m_mainSQL[i] = nullptr;
		}

		if (m_directSQL[i])
		{
			delete m_directSQL[i];
			m_directSQL[i] = nullptr;
		}

		if (m_asyncSQL[i])
		{
			delete m_asyncSQL[i];
			m_asyncSQL[i] = nullptr;
		}
	}

	Initialize();
}

void CDBManager::Quit()
{
	for (int32_t i = 0; i < SQL_MAX_NUM; ++i)
	{
		if (m_mainSQL[i])
			m_mainSQL[i]->Quit();

		if (m_asyncSQL[i])
			m_asyncSQL[i]->Quit();

		if (m_directSQL[i])
			m_directSQL[i]->Quit();
	}
}

SQLMsg * CDBManager::PopResult()
{
	SQLMsg* pSQLMsg;
	for (const auto pCAysncSQL2 : m_mainSQL)
	{
		if (pCAysncSQL2->PopResult(&pSQLMsg))
			return pSQLMsg;
	}

	return nullptr;
}

SQLMsg * CDBManager::PopResult(eSQL_SLOT slot)
{
	SQLMsg* p;

	if (m_mainSQL[slot] && m_mainSQL[slot]->PopResult(&p))
			return p;

	return nullptr;
}
int32_t CDBManager::Connect(int32_t iSlot, const char* db_address, const int32_t db_port, const char* db_name, const char* user, const char* pwd)
{
	if (!db_address || !db_name)
		return false;

	if (iSlot < 0 || iSlot >= SQL_MAX_NUM)
		return false;

	TraceLog("CREATING DIRECT_SQL");
	m_directSQL[iSlot] = new CAsyncSQL2;
	if (!m_directSQL[iSlot]->Setup(db_address, user, pwd, db_name, g_stLocale.c_str(), true, db_port))
	{
		Clear();
		return false;
	}


	TraceLog("CREATING MAIN_SQL");
	m_mainSQL[iSlot] = new CAsyncSQL2;
	if (!m_mainSQL[iSlot]->Setup(db_address, user, pwd, db_name, g_stLocale.c_str(), false, db_port))
	{
		Clear();
		return false;
	}

	TraceLog("CREATING ASYNC_SQL");
	m_asyncSQL[iSlot] = new CAsyncSQL2;
	if (!m_asyncSQL[iSlot]->Setup(db_address, user, pwd, db_name, g_stLocale.c_str(), false, db_port))
	{
		Clear();
		return false;
	}

	return true;
}

SQLMsg * CDBManager::DirectQuery(const char* c_pszQuery, int32_t iSlot)
{
	return m_directSQL[iSlot]->DirectQuery(c_pszQuery);
}

extern CPacketInfo g_query_info;

void CDBManager::ReturnQuery(const char* c_pszQuery, int32_t iType, IDENT dwIdent, void* udata, int32_t iSlot)
{
	assert(iSlot < SQL_MAX_NUM);
	CQueryInfo* p = new CQueryInfo;

	p->iType = iType;
	p->dwIdent = dwIdent;
	p->pvData = udata;

	m_mainSQL[iSlot]->ReturnQuery(c_pszQuery, p);

	++g_query_count[0];
}

void CDBManager::AsyncQuery(const char* c_pszQuery, int32_t iSlot)
{
	assert(iSlot < SQL_MAX_NUM);
	m_asyncSQL[iSlot]->AsyncQuery(c_pszQuery);
	++g_query_count[1];
}

uint32_t CDBManager::EscapeString(void*to, const void*from, uint32_t length, int32_t iSlot)
{
	assert(iSlot < SQL_MAX_NUM);
	return mysql_real_escape_string(m_directSQL[iSlot]->GetSQLHandle(), (char*) to, (const char*) from, length);
}

void CDBManager::SetLocale(const char* szLocale)
{
	const std::string stLocale(szLocale);
	TraceLog("SetLocale start");
	for (int32_t n = 0; n < SQL_MAX_NUM; ++n)
	{
		m_mainSQL[n]->SetLocale(stLocale);
		m_directSQL[n]->SetLocale(stLocale);
		m_asyncSQL[n]->SetLocale(stLocale);
	}
	TraceLog("End setlocale {}", szLocale);
}

void CDBManager::QueryLocaleSet()
{
	for (int32_t n = 0; n < SQL_MAX_NUM; ++n)
	{
		m_mainSQL[n]->QueryLocaleSet();
		m_directSQL[n]->QueryLocaleSet();
		m_asyncSQL[n]->QueryLocaleSet();
	}
}

