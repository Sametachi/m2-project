#include "StdAfx.h"
#include "PythonChat.h"
#include "PythonItem.h"
#include "../../Libraries/gameLib/ItemManager.h"

static void chatSetChatColor(uint32_t iType, uint32_t r, uint32_t g, uint32_t b)
{

	CPythonChat::GetInstance()->SetChatColor(iType, r, g, b);

}

static void chatClear()
{
	CPythonChat::GetInstance()->Destroy();

}

static void chatClose()
{
	CPythonChat::GetInstance()->Close();

}

static int chatCreateChatSet(uint32_t iID)
{

	return  CPythonChat::GetInstance()->CreateChatSet(iID);
}

static void chatUpdate(uint32_t iID)
{

	CPythonChat::GetInstance()->Update(iID);

}

static void chatRender(uint32_t iID)
{

	CPythonChat::GetInstance()->Render(iID);

}

static void chatSetBoardState(uint32_t iID, int32_t iState)
{

	CPythonChat::GetInstance()->SetBoardState(iID, iState);


}

static void chatSetPosition(uint32_t iID, int32_t ix, int32_t iy)
{

	CPythonChat::GetInstance()->SetPosition(iID, ix, iy);


}

static void chatSetHeight(uint32_t iID, int32_t iHeight)
{

	CPythonChat::GetInstance()->SetHeight(iID, iHeight);


}

static void chatSetStep(uint32_t iID, int32_t iStep)
{

	CPythonChat::GetInstance()->SetStep(iID, iStep);


}

static void chatToggleChatMode(uint32_t iID, int32_t iType)
{

	CPythonChat::GetInstance()->ToggleChatMode(iID, iType);


}

static void chatEnableChatMode(uint32_t iID, int32_t iType)
{

	CPythonChat::GetInstance()->EnableChatMode(iID, iType);


}

static void chatDisableChatMode(uint32_t iID, int32_t iType)
{

	CPythonChat::GetInstance()->DisableChatMode(iID, iType);


}

static void chatSetEndPos(uint32_t iID, float fPos)
{

	CPythonChat::GetInstance()->SetEndPos(iID, fPos);


}

static int32_t chatGetLineCount(uint32_t iID)
{

	return  CPythonChat::GetInstance()->GetLineCount(iID);
}

static int32_t chatGetVisibleLineCount(uint32_t iID)
{

	return  CPythonChat::GetInstance()->GetVisibleLineCount(iID);
}

static int32_t chatGetLineStep(uint32_t iID)
{

	return  CPythonChat::GetInstance()->GetLineStep(iID);
}

static void chatAppendChat(int32_t iType, std::string szChat)
{

	CPythonChat::GetInstance()->AppendChat(iType, szChat.c_str());


}

static void chatAppendChatWithDelay(int32_t iType, std::string szChat, int32_t iDelay)
{

	CPythonChat::GetInstance()->AppendChatWithDelay(iType, szChat.c_str(), iDelay);


}

static void chatArrangeShowingChat(uint32_t iID)
{

	CPythonChat::GetInstance()->ArrangeShowingChat(iID);


}

static void chatIgnoreCharacter(std::string szName)
{

	CPythonChat::GetInstance()->IgnoreCharacter(szName.c_str());


}

static void chatIsIgnoreCharacter(std::string szName)
{

	CPythonChat::GetInstance()->IsIgnoreCharacter(szName.c_str());


}

static void chatCreateWhisper(std::string szName)
{

	CPythonChat::GetInstance()->CreateWhisper(szName.c_str());


}

static void chatAppendWhisper(int32_t iType, std::string szName, std::string szChat)
{

	CPythonChat::GetInstance()->AppendWhisper(iType, szName.c_str(), szChat.c_str());

}

static void chatRenderWhisper(std::string szName, float fx, float fy)
{

	CWhisper * pWhisper;
	if (CPythonChat::GetInstance()->GetWhisper(szName.c_str(), &pWhisper))
	{
		pWhisper->Render(fx, fy);
	}


}

static void chatSetWhisperBoxSize(std::string szName, float fWidth, float fHeight)
{

	CWhisper * pWhisper;
	if (CPythonChat::GetInstance()->GetWhisper(szName.c_str(), &pWhisper))
	{
		pWhisper->SetBoxSize(fWidth, fHeight);
	}


}

static void chatSetWhisperPosition(std::string szName, float fPosition)
{

	CWhisper * pWhisper;
	if (CPythonChat::GetInstance()->GetWhisper(szName.c_str(), &pWhisper))
	{
		pWhisper->SetPosition(fPosition);
	}


}

static void chatClearWhisper(std::string szName)
{

	CPythonChat::GetInstance()->ClearWhisper(szName.c_str());


}

static void chatInitWhisper(pybind11::handle poInterface)
{

	CPythonChat::GetInstance()->InitWhisper(poInterface);

}

static std::string chatGetLinkFromHyperlink(std::string stHyperlink)
{
	std::vector<std::string> results;

	split_string(stHyperlink, ":", results, false);

	// item:vnum:flag:socket0:socket1:socket2
	if (0 == results[0].compare("item"))
	{
		if (results.size() < 6)
			return  "";

		CItemData * pItemData = NULL;

		if (CItemManager::GetInstance()->GetItemDataPointer(htoi(results[1].c_str()), &pItemData))
		{
			char buf[1024] = { 0 };
			char itemlink[256];
			int32_t len;
			bool isAttr = false;

			len = snprintf(itemlink, sizeof(itemlink), "item:%x:%x:%x:%x:%x", 
					htoi(results[1].c_str()),
					htoi(results[2].c_str()),
					htoi(results[3].c_str()),
					htoi(results[4].c_str()),
					htoi(results[5].c_str()));

			if (results.size() >= 8)
			{
				for (int32_t i = 6; i < results.size(); i += 2)
				{
					len += snprintf(itemlink + len, sizeof(itemlink) - len, ":%x:%d", 
							htoi(results[i].c_str()),
							atoi(results[i+1].c_str()));
					isAttr = true;
				}
			}

			if (isAttr)
				snprintf(buf, sizeof(buf), "|cffffc700|H%s|h |h|r", itemlink);
			else
				snprintf(buf, sizeof(buf), "|cfff1e6c0|H%s|h |h|r", itemlink);

			return  buf;
		}
	}

	return  "";
}

PYBIND11_EMBEDDED_MODULE(chat, m)
{
	m.def("SetChatColor",	chatSetChatColor);
	m.def("Clear",	chatClear);
	m.def("Close",	chatClose);
	m.def("CreateChatSet",	chatCreateChatSet);
	m.def("Update",	chatUpdate);
	m.def("Render",	chatRender);
	m.def("SetBoardState",	chatSetBoardState);
	m.def("SetPosition",	chatSetPosition);
	m.def("SetHeight",	chatSetHeight);
	m.def("SetStep",	chatSetStep);
	m.def("ToggleChatMode",	chatToggleChatMode);
	m.def("EnableChatMode",	chatEnableChatMode);
	m.def("DisableChatMode",	chatDisableChatMode);
	m.def("SetEndPos",	chatSetEndPos);
	m.def("GetLineCount",	chatGetLineCount);
	m.def("GetVisibleLineCount",	chatGetVisibleLineCount);
	m.def("GetLineStep",	chatGetLineStep);
	m.def("AppendChat",	chatAppendChat);
	m.def("AppendChatWithDelay",	chatAppendChatWithDelay);
	m.def("ArrangeShowingChat",	chatArrangeShowingChat);
	m.def("IgnoreCharacter",	chatIgnoreCharacter);
	m.def("IsIgnoreCharacter",	chatIsIgnoreCharacter);
	m.def("CreateWhisper",	chatCreateWhisper);
	m.def("AppendWhisper",	chatAppendWhisper);
	m.def("RenderWhisper",	chatRenderWhisper);
	m.def("SetWhisperBoxSize",	chatSetWhisperBoxSize);
	m.def("SetWhisperPosition",	chatSetWhisperPosition);
	m.def("ClearWhisper",	chatClearWhisper);
	m.def("InitWhisper",	chatInitWhisper);
	m.def("GetLinkFromHyperlink",	chatGetLinkFromHyperlink);

	m.attr("CHAT_TYPE_TALKING") = int32_t(CHAT_TYPE_TALKING);
	m.attr("CHAT_TYPE_INFO") = int32_t(CHAT_TYPE_INFO);
	m.attr("CHAT_TYPE_NOTICE") = int32_t(CHAT_TYPE_NOTICE);
	m.attr("CHAT_TYPE_PARTY") = int32_t(CHAT_TYPE_PARTY);
	m.attr("CHAT_TYPE_GUILD") = int32_t(CHAT_TYPE_GUILD);
	m.attr("CHAT_TYPE_COMMAND") = int32_t(CHAT_TYPE_COMMAND);
	m.attr("CHAT_TYPE_SHOUT") = int32_t(CHAT_TYPE_SHOUT);
	m.attr("CHAT_TYPE_WHISPER") = int32_t(CHAT_TYPE_WHISPER);
	m.attr("WHISPER_TYPE_CHAT") =	int32_t(CPythonChat::WHISPER_TYPE_CHAT);
	m.attr("WHISPER_TYPE_SYSTEM") = int32_t(CPythonChat::WHISPER_TYPE_SYSTEM);
	m.attr("WHISPER_TYPE_GM") = int32_t(CPythonChat::WHISPER_TYPE_GM);
	m.attr("BOARD_STATE_VIEW") = int32_t(CPythonChat::BOARD_STATE_VIEW);
	m.attr("BOARD_STATE_EDIT") = int32_t(CPythonChat::BOARD_STATE_EDIT);
	m.attr("BOARD_STATE_LOG") = int32_t(CPythonChat::BOARD_STATE_LOG);
	m.attr("CHAT_SET_CHAT_WINDOW") =	0;
	m.attr("CHAT_SET_LOG_WINDOW") =	1;
}
