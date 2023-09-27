#include "StdAfx.h"
#include "PythonMessenger.h"

void CPythonMessenger::RemoveFriend(const char* c_szKey)
{
	m_FriendNameMap.erase(c_szKey);
}

void CPythonMessenger::OnFriendLogin(const char* c_szKey/*, const char * c_szName*/)
{
	m_FriendNameMap.insert(c_szKey);

	if (m_poMessengerHandler)
		PyCallClassMemberFunc(m_poMessengerHandler, "OnLogin", MESSENGER_GRUOP_INDEX_FRIEND, c_szKey);
}

void CPythonMessenger::OnFriendLogout(const char* c_szKey)
{
	m_FriendNameMap.insert(c_szKey);

	if (m_poMessengerHandler)
		PyCallClassMemberFunc(m_poMessengerHandler, "OnLogout", MESSENGER_GRUOP_INDEX_FRIEND, c_szKey);
}

BOOL CPythonMessenger::IsFriendByKey(const char* c_szKey)
{
	return m_FriendNameMap.end() != m_FriendNameMap.find(c_szKey);
}

BOOL CPythonMessenger::IsFriendByName(const char* c_szName)
{
	return IsFriendByKey(c_szName);
}

void CPythonMessenger::AppendGuildMember(const char* c_szName)
{
	if (m_GuildMemberStateMap.end() != m_GuildMemberStateMap.find(c_szName))
		return;

	LogoutGuildMember(c_szName);
}

void CPythonMessenger::RemoveGuildMember(const char* c_szName)
{
	m_GuildMemberStateMap.erase(c_szName);

	if (m_poMessengerHandler)
		PyCallClassMemberFunc(m_poMessengerHandler, "OnRemoveList", MESSENGER_GRUOP_INDEX_GUILD, c_szName);
}

void CPythonMessenger::RemoveAllGuildMember()
{
	m_GuildMemberStateMap.clear();

	if (m_poMessengerHandler)
		PyCallClassMemberFunc(m_poMessengerHandler, "OnRemoveAllList", MESSENGER_GRUOP_INDEX_GUILD);
}

void CPythonMessenger::LoginGuildMember(const char* c_szName)
{
	m_GuildMemberStateMap[c_szName] = 1;
	if (m_poMessengerHandler)
		PyCallClassMemberFunc(m_poMessengerHandler, "OnLogin", MESSENGER_GRUOP_INDEX_GUILD, c_szName);
}

void CPythonMessenger::LogoutGuildMember(const char* c_szName)
{
	m_GuildMemberStateMap[c_szName] = 0;
	if (m_poMessengerHandler)
		PyCallClassMemberFunc(m_poMessengerHandler, "OnLogout", MESSENGER_GRUOP_INDEX_GUILD, c_szName);
}

void CPythonMessenger::RefreshGuildMember()
{
	for (TGuildMemberStateMap::iterator itor = m_GuildMemberStateMap.begin(); itor != m_GuildMemberStateMap.end(); ++itor)
	{
		if (itor->second)
			PyCallClassMemberFunc(m_poMessengerHandler, "OnLogin", MESSENGER_GRUOP_INDEX_GUILD, (itor->first));
		else
			PyCallClassMemberFunc(m_poMessengerHandler, "OnLogout", MESSENGER_GRUOP_INDEX_GUILD, (itor->first));
	}
}

void CPythonMessenger::Destroy()
{
	m_FriendNameMap.clear();
	m_GuildMemberStateMap.clear();
}

void CPythonMessenger::SetMessengerHandler(pybind11::handle poHandler)
{
	m_poMessengerHandler = poHandler;
}

CPythonMessenger::CPythonMessenger()
{
}

CPythonMessenger::~CPythonMessenger()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////



static void messengerRemoveFriend(std::string szKey)
{

	CPythonMessenger::GetInstance()->RemoveFriend(szKey.c_str());

}

static int messengerIsFriendByName(std::string szName)
{

	return  CPythonMessenger::GetInstance()->IsFriendByName(szName.c_str());
}

static void messengerDestroy()
{
	CPythonMessenger::GetInstance()->Destroy();

}

static void messengerRefreshGuildMember()
{
	CPythonMessenger::GetInstance()->RefreshGuildMember();

}

static void messengerSetMessengerHandler(pybind11::handle poHandle)
{
	CPythonMessenger::GetInstance()->SetMessengerHandler(poHandle);

}



PYBIND11_EMBEDDED_MODULE(messenger, m)
{
	m.def("RemoveFriend",	messengerRemoveFriend);
	m.def("IsFriendByName",	messengerIsFriendByName);
	m.def("Destroy",	messengerDestroy);
	m.def("RefreshGuildMember",	messengerRefreshGuildMember);
	m.def("SetMessengerHandler",	messengerSetMessengerHandler);

}
