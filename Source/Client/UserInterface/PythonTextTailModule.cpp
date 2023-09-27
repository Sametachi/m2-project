#include "StdAfx.h"
#include "PythonTextTail.h"
#include "PythonCharacterManager.h"

static void texttailClear()
{
	CPythonTextTail::GetInstance()->Clear();
}

static void texttailUpdateAllTextTail()
{
	CPythonTextTail::GetInstance()->UpdateAllTextTail();
}

static void texttailUpdateShowingTextTail()
{
	CPythonTextTail::GetInstance()->UpdateShowingTextTail();
}

static void texttailRender()
{
	CPythonTextTail::GetInstance()->Render();
}

static void texttailShowCharacterTextTail(uint32_t VirtualID)
{
	CPythonTextTail::GetInstance()->ShowCharacterTextTail(VirtualID);
}

static void texttailShowItemTextTail(uint32_t VirtualID)
{
	CPythonTextTail::GetInstance()->ShowItemTextTail(VirtualID);
}

static std::tuple<float,float,float> texttailGetPosition(uint32_t VirtualID)
{
	float x=0.0f;
	float y=0.0f;
	float z=0.0f;
	bool isData=CPythonTextTail::GetInstance()->GetTextTailPosition(VirtualID, &x, &y, &z);
	if (!isData)
	{
		auto rkChrMgr=CPythonCharacterManager::GetInstance();
		CInstanceBase* pkInstMain=rkChrMgr->GetMainInstancePtr();
		if (pkInstMain)
		{
			const D3DXVECTOR3 & c_rv3Position = pkInstMain->GetGraphicThingInstanceRef().GetPosition();
			CPythonGraphic::GetInstance()->ProjectPosition(c_rv3Position.x, c_rv3Position.y, c_rv3Position.z, &x, &y);	
		}
	}	

	return std::make_tuple( x, y, z);
}

static bool texttailIsChat(uint32_t VirtualID)
{
	return  CPythonTextTail::GetInstance()->IsChatTextTail(VirtualID);
}

static void texttailArrangeTextTail()
{
	CPythonTextTail::GetInstance()->ArrangeTextTail();
}

static void texttailHideAllTextTail()
{
	CPythonTextTail::GetInstance()->HideAllTextTail();
}

static void texttailShowAllTextTail()
{
	CPythonTextTail::GetInstance()->ShowAllTextTail();
}

static int32_t texttailPick(int32_t ix, int32_t iy)
{
	return CPythonTextTail::GetInstance()->Pick(ix, iy);
}

static void texttailSelectItemName(uint32_t iVirtualID)
{
	CPythonTextTail::GetInstance()->SelectItemName(iVirtualID);
}

static void texttailEnablePKTitle(BOOL iFlag)
{
	CPythonTextTail::GetInstance()->EnablePKTitle(iFlag);
}

static void texttailRegisterCharacterTextTail(uint32_t iGuildID, uint32_t iVirtualID)
{
	CPythonTextTail::GetInstance()->RegisterCharacterTextTail(iGuildID, iVirtualID, D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
}

static void texttailRegisterChatTail(uint32_t VirtualID, std::string szText)
{
	CPythonTextTail::GetInstance()->RegisterChatTail(VirtualID, szText.c_str());
}

static void texttailRegisterInfoTail(uint32_t VirtualID, std::string szText)
{
	CPythonTextTail::GetInstance()->RegisterInfoTail(VirtualID, szText.c_str());
}

static void texttailAttachTitle(uint32_t iVirtualID, std::string szName, float fr, float fg, float fb)
{
	CPythonTextTail::GetInstance()->AttachTitle(iVirtualID, szName.c_str(), D3DXCOLOR(fr, fg, fb, 1.0f));
}

static void texttailShowAllPCTextTail()
{
	CPythonTextTail::GetInstance()->ShowAllPCTextTail();
}

static void texttailShowAllNPCTextTail()
{
	CPythonTextTail::GetInstance()->ShowAllNPCTextTail();
}

static void texttailShowAllMonsterTextTail()
{
	CPythonTextTail::GetInstance()->ShowAllMonsterTextTail();
}

static void texttailShowAllItemTextTail()
{
	CPythonTextTail::GetInstance()->ShowAllItemTextTail();
}

PYBIND11_EMBEDDED_MODULE(textTail, m)
{
	m.def("Clear",	texttailClear);
	m.def("UpdateAllTextTail",	texttailUpdateAllTextTail);
	m.def("UpdateShowingTextTail",	texttailUpdateShowingTextTail);
	m.def("Render",	texttailRender);
	m.def("ShowCharacterTextTail",	texttailShowCharacterTextTail);
	m.def("ShowItemTextTail",	texttailShowItemTextTail);
	m.def("GetPosition",	texttailGetPosition);
	m.def("IsChat",	texttailIsChat);
	m.def("ArrangeTextTail",	texttailArrangeTextTail);
	m.def("Pick",	texttailPick);
	m.def("SelectItemName",	texttailSelectItemName);
	m.def("EnablePKTitle",	texttailEnablePKTitle);
	m.def("RegisterCharacterTextTail",	texttailRegisterCharacterTextTail);
	m.def("RegisterChatTail",	texttailRegisterChatTail);
	m.def("RegisterInfoTail",	texttailRegisterInfoTail);
	m.def("AttachTitle",	texttailAttachTitle);
	m.def("HideAllTextTail", texttailHideAllTextTail);
	m.def("ShowAllTextTail", texttailShowAllTextTail);

	m.def("ShowAllPCTextTail", texttailShowAllPCTextTail);
	m.def("ShowAllNPCTextTail", texttailShowAllNPCTextTail);
	m.def("ShowAllMonsterTextTail", texttailShowAllMonsterTextTail);
	m.def("ShowAllItemTextTail", texttailShowAllItemTextTail);
}
