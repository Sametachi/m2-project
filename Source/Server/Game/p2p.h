#pragma once

#include <Core/Net/PacketsGG.hpp>
#include <boost/unordered_map.hpp>

#include "input.h"
#include <Common/stl.h>

typedef struct _CCI
{
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	uint32_t	dwPID;
	uint8_t	bEmpire;
	int32_t	lMapIndex;
	uint8_t	bChannel;

	LPDESC	pDesc;
} CCI;

class P2P_MANAGER : public Singleton<P2P_MANAGER>
{
	public:
		P2P_MANAGER();
		~P2P_MANAGER();

		void			RegisterAcceptor(LPDESC d);
		void			UnregisterAcceptor(LPDESC d);

		void			RegisterConnector(LPDESC d);
		void			UnregisterConnector(LPDESC d);

		void			EraseUserByDesc(LPDESC d);

		void			FlushOutput();

		void			Boot(LPDESC d);

		void			Send(const void* c_pvData, int32_t iSize, LPDESC except = nullptr);

		void			Login(LPDESC d, const TPacketGGLogin* p);
		void			Logout(const char* c_pszName);

		CCI *			Find(const char* c_pszName);
		CCI *			FindByPID(uint32_t pid);

		int32_t				GetCount();
		int32_t				GetEmpireUserCount(int32_t idx);
		int32_t				GetDescCount();
		void			GetP2PHostNames(std::string& hostNames);

	private:
		void			Logout(CCI* pCCI);

		CInputProcessor *	m_pInputProcessor;
		int32_t			m_iHandleCount;

		typedef boost::unordered_map<std::string, CCI *, stringhash> TCCIMap;
		typedef boost::unordered_map<uint32_t, CCI*> TPIDCCIMap;

		TR1_NS::unordered_set<LPDESC> m_set_pPeers;
		TCCIMap			m_map_pCCI;
		TPIDCCIMap		m_map_dwPID_pCCI;
		int32_t			m_aiEmpireUserCount[EMPIRE_MAX_NUM];
};

