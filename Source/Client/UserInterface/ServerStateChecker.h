#pragma once
#include <EterLib/NetStream.h>

class CServerStateChecker : public Singleton<CServerStateChecker>
{
public:
	CServerStateChecker();
	virtual ~CServerStateChecker();

	void Create(pybind11::handle poWnd);
	void AddChannel(uint32_t uServerIndex, const char* c_szAddr, uint32_t uPort);
	void Request();
	void Update();
	void Initialize();
		
private:
	typedef struct SChannel
	{
		uint32_t uServerIndex;
		const char* c_szAddr;
		uint32_t uPort;
	} TChannel;

	pybind11::handle m_poWnd;
	std::list<TChannel> m_lstChannel;
	CNetworkStream m_kStream;
};