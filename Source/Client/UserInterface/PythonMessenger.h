#pragma once

class CPythonMessenger : public Singleton<CPythonMessenger>
{
	public:
		typedef std::set<std::string> TFriendNameMap;
		typedef std::map<std::string, uint8_t> TGuildMemberStateMap;

		enum EMessengerGroupIndex
		{
			MESSENGER_GRUOP_INDEX_FRIEND,
			MESSENGER_GRUOP_INDEX_GUILD,
		};

	public:
		CPythonMessenger();
		virtual ~CPythonMessenger();

		void Destroy();

		// Friend
		void RemoveFriend(const char * c_szKey);
		void OnFriendLogin(const char * c_szKey);
		void OnFriendLogout(const char * c_szKey);
		BOOL IsFriendByKey(const char * c_szKey);
		BOOL IsFriendByName(const char * c_szName);

		// Guild
		void AppendGuildMember(const char * c_szName);
		void RemoveGuildMember(const char * c_szName);
		void RemoveAllGuildMember();
		void LoginGuildMember(const char * c_szName);
		void LogoutGuildMember(const char * c_szName);
		void RefreshGuildMember();

		void SetMessengerHandler(pybind11::handle poHandler);

	protected:
		TFriendNameMap m_FriendNameMap;
		TGuildMemberStateMap m_GuildMemberStateMap;

	private:
		pybind11::handle m_poMessengerHandler;
};
