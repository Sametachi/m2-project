#pragma once

#include <LibTheCore/include/AsyncSQL.h>

#define SQL_SAFE_LENGTH(size)	(size * 2 + 1)
#define QUERY_SAFE_LENGTH(size)	(1024 + SQL_SAFE_LENGTH(size))
static uint32_t g_query_count[2];

class CQueryInfo
{
public:
	int32_t	iType;
	uint32_t	dwIdent;
	void*	pvData;
};

enum eSQL_SLOT
{
    SQL_PLAYER,
    SQL_ACCOUNT,
	SQL_COMMON,
	SQL_HOTBACKUP,
    SQL_MAX_NUM,
};

class CDBManager : public Singleton<CDBManager>
{
protected:
	void			Initialize();
	void			Destroy();

public:
	CDBManager();
	virtual ~CDBManager();

	void			Clear();
	void			Quit();

	int32_t			Connect(int32_t iSlot, const char* host, int32_t port, const char* dbname, const char* user, const char* pass);

	void			ReturnQuery(const char* c_pszQuery, int32_t iType, uint32_t dwIdent, void* pvData, int32_t iSlot = SQL_PLAYER);
	void			AsyncQuery(const char* c_pszQuery, int32_t iSlot = SQL_PLAYER);
	SQLMsg *		DirectQuery(const char* c_pszQuery, int32_t iSlot = SQL_PLAYER);

	SQLMsg *		PopResult();
	SQLMsg * 		PopResult(eSQL_SLOT slot);

	uint32_t		EscapeString(void* to, const void* from, uint32_t length, int32_t iSlot = SQL_PLAYER);

	uint32_t			CountReturnQuery(int32_t i) { return m_mainSQL[i] ? m_mainSQL[i]->CountQuery() : 0; }
	uint32_t			CountReturnResult(int32_t i) { return m_mainSQL[i] ? m_mainSQL[i]->CountResult() : 0; }
	uint32_t			CountReturnQueryFinished(int32_t i) { return m_mainSQL[i] ? m_mainSQL[i]->CountQueryFinished() : 0; }
	uint32_t			CountReturnCopiedQuery(int32_t i) { return m_mainSQL[i] ? m_mainSQL[i]->GetCopiedQueryCount() : 0; }

	uint32_t			CountAsyncQuery(int32_t i) { return m_asyncSQL[i] ? m_asyncSQL[i]->CountQuery() : 0; }
	uint32_t			CountAsyncResult(int32_t i) { return m_asyncSQL[i] ? m_asyncSQL[i]->CountResult() : 0; }
	uint32_t			CountAsyncQueryFinished(int32_t i) { return m_asyncSQL[i] ? m_asyncSQL[i]->CountQueryFinished() : 0; }
	uint32_t			CountAsyncCopiedQuery(int32_t i) { return m_asyncSQL[i] ? m_asyncSQL[i]->GetCopiedQueryCount() : 0; }

	void			ResetCounter()
	{
	    for (int32_t i = 0; i < SQL_MAX_NUM; ++i)
		{
			if (m_mainSQL[i])
			{
				m_mainSQL[i]->ResetQueryFinished();
				m_mainSQL[i]->ResetCopiedQueryCount();
			}

			if (m_asyncSQL[i])
			{
				m_asyncSQL[i]->ResetQueryFinished();
				m_asyncSQL[i]->ResetCopiedQueryCount();
			}
		}
	}

private:
	CAsyncSQL2 *		m_mainSQL[SQL_MAX_NUM];
	CAsyncSQL2 *	 	m_directSQL[SQL_MAX_NUM];
	CAsyncSQL2 *		m_asyncSQL[SQL_MAX_NUM];

	int32_t			m_quit;

public:
	void SetLocale(const char* szLocale);
	void QueryLocaleSet();
};
