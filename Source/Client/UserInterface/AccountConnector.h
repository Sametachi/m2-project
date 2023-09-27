#pragma once

#include "../../Libraries/EterLib/NetStream.h"
#include "../../Libraries/EterLib/FuncObject.h"

class CAccountConnector : public CNetworkStream, public Singleton<CAccountConnector>
{
	public:
		enum
		{
			STATE_OFFLINE,
			STATE_HANDSHAKE,
			STATE_AUTH,
		};

	public:
		CAccountConnector();
		virtual ~CAccountConnector();

		void SetHandler(pybind11::handle poHandler);
		void SetLoginInfo(const char * c_szName, const char * c_szPwd);
		void ClearLoginInfo( void );

		bool Connect(const char * c_szAddr, int32_t iPort, const char * c_szAccountAddr, int32_t iAccountPort);
		void Disconnect();
		void Process();

	protected:
		void OnConnectFailure();
		void OnConnectSuccess();
		void OnRemoteDisconnect();
		void OnDisconnect();

	protected:
		void __Inialize();
		bool __StateProcess();

		void __OfflineState_Set();
		void __HandshakeState_Set();
		void __AuthState_Set();

		bool __HandshakeState_Process();
		bool __AuthState_Process();

		bool __AuthState_RecvEmpty();
		bool __AuthState_RecvPhase();
		bool __AuthState_RecvHandshake();
		bool __AuthState_RecvPing();
		bool __AuthState_SendPong();
		bool __AuthState_RecvAuthSuccess();
		bool __AuthState_RecvAuthFailure();

		bool __AnalyzePacket(UINT uHeader, UINT uPacketSize, bool (CAccountConnector::*pfnDispatchPacket)());
		bool __AnalyzeVarSizePacket(UINT uHeader, bool (CAccountConnector::*pfnDispatchPacket)(int32_t));
		void __BuildClientKey();

	protected:
		UINT m_eState;
		std::string m_strID;
		std::string m_strPassword;

		std::string m_strAddr;
		int32_t m_iPort;
		BOOL m_isWaitKey;

		pybind11::handle m_poHandler;

		// CHINA_CRYPT_KEY
		void __BuildClientKey_20050304Myevan();
		// END_OF_CHINA_CRYPT_KEY
};
