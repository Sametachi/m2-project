#pragma once

#include "../eterBase/tea.h"
#include "NetAddress.h"

class CNetworkStream
{
	public:
		CNetworkStream();
		virtual ~CNetworkStream();

		void SetRecvBufferSize(int32_t recvBufSize);
		void SetSendBufferSize(int32_t sendBufSize);

		void SetSecurityMode(bool isSecurityMode, const char* c_szTeaKey);
		void SetSecurityMode(bool isSecurityMode, const char* c_szTeaEncryptKey, const char* c_szTeaDecryptKey);
		bool IsSecurityMode();

		int32_t	GetRecvBufferSize();

		void Clear();
		void ClearRecvBuffer();

		void Process();

		bool Connect(const CNetworkAddress& c_rkNetAddr, int32_t limitSec = 3);
		bool Connect(const char* c_szAddr, int32_t port, int32_t limitSec = 3);
		bool Connect(uint32_t dwAddr, int32_t port, int32_t limitSec = 3);
		void Disconnect();

		bool Peek(int32_t len);
		bool Peek(int32_t len, char* pDestBuf);
		bool Recv(int32_t len);
		bool Recv(int32_t len, char* pDestBuf);
		bool Send(int32_t len, const char* pSrcBuf);

		bool Peek(int32_t len, void* pDestBuf);
		bool Recv(int32_t len, void* pDestBuf);

		bool Send(int32_t len, const void* pSrcBuf);
		bool SendFlush(int32_t len, const void* pSrcBuf);

		bool IsOnline();

	protected:
		virtual void OnConnectSuccess();
		virtual void OnConnectFailure();
		virtual void OnRemoteDisconnect();
		virtual void OnDisconnect();
		virtual bool OnProcess();

		bool __SendInternalBuffer();
		bool __RecvInternalBuffer();

		void __PopSendBuffer();

		int32_t __GetSendBufferSize();


	private:
		time_t	m_connectLimitTime;

		char*	m_recvTEABuf;
		int32_t		m_recvTEABufInputPos;
		int32_t		m_recvTEABufSize;

		char*	m_recvBuf;
		int32_t		m_recvBufSize;
		int32_t		m_recvBufInputPos;
		int32_t		m_recvBufOutputPos;

		char*	m_sendBuf;
		int32_t		m_sendBufSize;
		int32_t		m_sendBufInputPos;
		int32_t		m_sendBufOutputPos;

		char*	m_sendTEABuf;
		int32_t		m_sendTEABufSize;
		int32_t		m_sendTEABufInputPos;

		bool	m_isOnline;

		// Obsolete encryption stuff here
		bool	m_isSecurityMode;
		char	m_szEncryptKey[TEA_KEY_LENGTH];
		char	m_szDecryptKey[TEA_KEY_LENGTH];
		SOCKET	m_sock;

		CNetworkAddress m_addr;
};
