#pragma once

#include <LibTheCore/include/AsyncSQL.h>
#include <functional>

enum
{
	QUERY_TYPE_RETURN = 1,
	QUERY_TYPE_FUNCTION = 2,
	QUERY_TYPE_AFTER_FUNCTION = 3,
};

enum
{
	QID_SAFEBOX_SIZE,
	QID_DB_STRING,
	QID_AUTH_LOGIN,
	QID_AUTH_LOGIN_OPENID,
	QID_LOTTO,
	QID_HIGHSCORE_REGISTER,
	QID_HIGHSCORE_SHOW,

	// BLOCK_CHAT
	QID_BLOCK_CHAT_LIST,
	// END_OF_BLOCK_CHAT

	// PROTECT_CHILD_FOR_NEWCIBN
	QID_PROTECT_CHILD,
	// END_PROTECT_CHILD_FOR_NEWCIBN

	QID_BRAZIL_CREATE_ID,
	QID_JAPAN_CREATE_ID,
};

typedef struct SUseTime
{
	uint32_t	dwLoginKey;
	char        szLogin[LOGIN_MAX_LEN+1];
	uint32_t      dwUseSec;
	char        szIP[MAX_HOST_LENGTH+1];
} TUseTime;

class CQueryInfo
{
	public:
		int32_t	iQueryType;
};

class CReturnQueryInfo : public CQueryInfo
{
	public:
		int32_t	iType;
		uint32_t	dwIdent;
		void			*	pvData;
};

class CFuncQueryInfo : public CQueryInfo
{
public:
	std::function<void(SQLMsg*)> f;
};

class CFuncAfterQueryInfo : public CQueryInfo
{
public:
	std::function<void()> f;
};

class CLoginData;


class DBManager : public Singleton<DBManager>
{
	public:
		DBManager();
		virtual ~DBManager();

		bool			IsConnected();

		bool			Connect(const char* host, const int32_t port, const char* user, const char* pwd, const char* db);
		void			Query(const char* c_pszFormat, ...);

		SQLMsg *		DirectQuery(const char* c_pszFormat, ...);
		void			ReturnQuery(int32_t iType, uint32_t dwIdent, void* pvData, const char* c_pszFormat, ...);

		void			FuncQuery(std::function<void(SQLMsg*)> f, const char* c_pszFormat, ...);
		void			FuncAfterQuery(std::function<void()> f, const char* c_pszFormat, ...);

		void			Process();
		void			AnalyzeReturnQuery(SQLMsg* pmsg);

		void			SendMoneyLog(uint8_t type, uint32_t vnum, int32_t gold);

		void			LoginPrepare(LPDESC d, uint32_t* pdwClientKey, int32_t* paiPremiumTimes = NULL);
		void			SendAuthLogin(LPDESC d);
		void			SendLoginPing(const char* c_pszLogin);

		void			InsertLoginData(CLoginData* pLD);
		void			DeleteLoginData(CLoginData* pLD);
		CLoginData *		GetLoginData(uint32_t dwKey);
		uint32_t			CountQuery()		{ return m_sql.CountQuery(); }
		uint32_t			CountQueryResult()	{ return m_sql.CountResult(); }
		void			ResetQueryResult()	{ m_sql.ResetQueryFinished(); }

		void			LoadDBString();
		const std::string &	GetDBString(const std::string& key);
		const std::vector<std::string> & GetGreetMessage();

		size_t EscapeString(char* dst, size_t dstSize, const char* src, size_t srcSize);

	private:
		SQLMsg *				PopResult();

		CAsyncSQL				m_sql;
		CAsyncSQL				m_sql_direct;
		bool					m_bIsConnect;

		std::map<std::string, std::string>	m_map_dbstring;
		std::vector<std::string>		m_vec_GreetMessage;
		std::map<uint32_t, CLoginData *>		m_map_pLoginData;
		std::vector<TUseTime>			m_vec_kUseTime;
};

////////////////////////////////////////////////////////////////
typedef struct SHighscoreRegisterQueryInfo
{
	char    szBoard[20+1]; 
	uint32_t   dwPID;
	int32_t     iValue;
	bool    bOrder;
} THighscoreRegisterQueryInfo;

// ACCOUNT_DB
class AccountDB : public Singleton<AccountDB>
{
	public:
		AccountDB();

		bool IsConnected();
		bool Connect(const char* host, const int32_t port, const char* user, const char* pwd, const char* db);
		bool ConnectAsync(const char* host, const int32_t port, const char* user, const char* pwd, const char* db, const char* locale);

		SQLMsg* DirectQuery(const char* query);		
		void ReturnQuery(int32_t iType, uint32_t dwIdent, void* pvData, const char* c_pszFormat, ...);
		void AsyncQuery(const char* query);

		void SetLocale(const std::string & stLocale);

		void Process();

	private:
		SQLMsg * PopResult();
		void AnalyzeReturnQuery(SQLMsg* pMsg);

		CAsyncSQL2	m_sql_direct;
		CAsyncSQL2	m_sql;
		bool		m_IsConnect;

};
//END_ACCOUNT_DB

