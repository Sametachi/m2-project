#include "StdAfx.h"
#include "PythonNetworkStream.h"
//#include "PythonNetworkDatagram.h"
#include "AccountConnector.h"
#include "PythonGuild.h"
#include "PythonPlayer.h"
#include "GuildMarkUploader.h"
#include "GuildMarkDownloader.h"
#include "AbstractResources.h"

static std::string gs_stServerInfo;
extern bool gs_bEmpireLanuageEnable;
std::list<std::string> g_kList_strCommand;

static uint32_t netGetBettingGuildWarValue(std::string szName)
{

	
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->EXPORT_GetBettingGuildWarValue(szName.c_str());
}

static void netEnableChatInsultFilter(bool nEnable)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->EnableChatInsultFilter(nEnable);

}

static void netSetServerInfo(std::string szFileName)
{

	gs_stServerInfo=szFileName;

}

static std::string netGetServerInfo()
{
	return  gs_stServerInfo.c_str();
}

static void netPreserveServerCommand(std::string szLine)
{

	g_kList_strCommand.push_back(szLine);


}

static std::string netGetPreservedServerCommand()
{
	if (g_kList_strCommand.empty())
		return  "";

	std::string strCommand = g_kList_strCommand.front();
	g_kList_strCommand.pop_front();

	return  strCommand;
}

static void netStartGame()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->StartGame();
	AbstractResources::GetInstance()->SetLoadingState(true);
}

static void netSetMarkServer(std::string szAddr, uint32_t port)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SetMarkServer(szAddr.c_str(), port);

}

static bool netIsChatInsultIn(std::string szMsg)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->IsChatInsultIn(szMsg.c_str());
}

static bool netIsInsultIn(std::string szMsg)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->IsInsultIn(szMsg.c_str());
}

static bool netLoadInsultList(std::string szFileName)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->LoadInsultList(szFileName.c_str());
}

static uint32_t netUploadMark(std::string szFileName)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->UploadMark(szFileName.c_str());
}

static uint32_t netUploadSymbol(std::string szFileName)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->UploadSymbol(szFileName.c_str());
}

static uint32_t netGetGuildID()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->GetGuildID();
}

static uint32_t netGetEmpireID()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->GetEmpireID();
}

static uint32_t netGetMainActorVID()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->GetMainActorVID();
}

static uint32_t netGetMainActorRace()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->GetMainActorRace();
}

static uint32_t netGetMainActorEmpire()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->GetMainActorEmpire();
}

static uint32_t netGetMainActorSkillGroup()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->GetMainActorSkillGroup();
}

static uint32_t netGetAccountCharacterSlotDataInteger(uint32_t nIndex, uint32_t nType)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return rkNetStream->GetAccountCharacterSlotDatau(nIndex, nType);
}

static std::string netGetAccountCharacterSlotDataString(uint32_t nIndex, uint32_t nType)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return rkNetStream->GetAccountCharacterSlotDataz(nIndex, nType);
}

static std::string netGetFieldMusicFileName()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->GetFieldMusicFileName();
}

static float netGetFieldMusicVolume()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->GetFieldMusicVolume();
}

static void netToggleGameDebugInfo()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->ToggleGameDebugInfo();

}

static void netSetLoginInfo(std::string szName, std::string szPwd)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	auto rkAccountConnector = CAccountConnector::GetInstance();
	rkNetStream->SetLoginInfo(szName.c_str(), szPwd.c_str());
	rkAccountConnector->SetLoginInfo(szName.c_str(), szPwd.c_str());

}

static void netSetPhaseWindow(int ePhaseWnd, pybind11::handle poPhaseWnd)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SetPhaseWindow(ePhaseWnd, poPhaseWnd);

}

static void netClearPhaseWindow(int ePhaseWnd, pybind11::handle poPhaseWnd)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->ClearPhaseWindow(ePhaseWnd, poPhaseWnd);

}

static void netSetServerCommandParserWindow(pybind11::handle poPhaseWnd)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SetServerCommandParserWindow(poPhaseWnd);

}

static void netSetAccountConnectorHandler(pybind11::handle poPhaseWnd)
{

	auto rkAccountConnector = CAccountConnector::GetInstance();
	rkAccountConnector->SetHandler(poPhaseWnd);

}

static void netSetHandler(pybind11::handle poHandler)
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SetHandler(poHandler);

}

static void netSetTCPRecvBufferSize(int32_t bufSize)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SetRecvBufferSize(bufSize);

}

static void netSetTCPSendBufferSize(int32_t bufSize)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SetSendBufferSize(bufSize);

}

static void netSetUDPRecvBufferSize(int32_t bufSize)
{

	//CPythonNetworkDatagram::GetInstance()->SetRecvBufferSize(bufSize);

}

static void netDirectEnter(uint32_t nChrSlot)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->ConnectGameServer(nChrSlot);

}

static void netLogOutGame()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->LogOutGame();

}

static void netExitGame()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->ExitGame();

}

static void netExitApplication()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->ExitApplication();

}

static void netConnectTCP(std::string szAddr, uint32_t port)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->ConnectLoginServer(szAddr.c_str(), port);

}

static void netConnectUDP(std::string c_szIP, uint32_t iPort)
{

	//CPythonNetworkDatagram::GetInstance()->SetConnection(c_szIP, iPort);

}

static void netConnectToAccountServer(std::string addr, int32_t port, std::string account_addr, int32_t account_port)
{

	auto rkAccountConnector = CAccountConnector::GetInstance();
	rkAccountConnector->Connect(addr.c_str(), port, account_addr.c_str(), account_port);

}

static void netSendLoginPacket(std::string szName, std::string szPwd)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendLoginPacket(szName.c_str(), szPwd.c_str());

}

static void netSendSelectEmpirePacket(uint32_t iEmpireIndex)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendSelectEmpirePacket(iEmpireIndex);

}

static void netSendSelectCharacterPacket(uint8_t Index)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendSelectCharacterPacket(Index);

}

static void netSendChangeNamePacket(uint8_t iIndex, std::string szName)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendChangeNamePacket(iIndex, szName.c_str());

}

static void netSendCreateCharacterPacket(uint8_t index, std::string name, uint8_t job, uint8_t shape, uint8_t stat1, uint8_t stat2, uint8_t stat3, uint8_t stat4)
{
	if (index < 0 && index > 3)
		throw std::runtime_error("Index error");

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendCreateCharacterPacket(index, name.c_str(), job, shape, stat1, stat2, stat3, stat4);

}

static void netSendDestroyCharacterPacket(uint8_t index, std::string szPrivateCode)
{

	if (index < 0 && index > 3)
		throw std::runtime_error("Index error");
	
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendDestroyCharacterPacket(index, szPrivateCode.c_str());

}

static void netSendEnterGamePacket()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendEnterGame();

}

static void netSendItemUsePacket(uint16_t cell)
{
	TItemPos Cell;
	Cell.cell = cell;

	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemUsePacket(Cell);
}

static void netSendItemUsePacket2(uint8_t type, uint16_t cell)
{
	TItemPos Cell(type, cell);

	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemUsePacket(Cell);
}

static void  netSendItemUseToItemPacket(uint16_t src, uint16_t trg)
{
	TItemPos SourceCell;
	TItemPos TargetCell;

	SourceCell.cell = src;
	TargetCell.cell = trg;

	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemUseToItemPacket(SourceCell, TargetCell);
}

static void  netSendItemUseToItemPacket2(uint8_t srcw, uint16_t src, uint8_t trgw, uint16_t trg)
{
	TItemPos SourceCell(srcw, src);
	TItemPos TargetCell(trgw, trg);

	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemUseToItemPacket(SourceCell, TargetCell);
}

static void netSendItemDropPacket(uint16_t cell)
{
	TItemPos Cell;
	Cell.cell = cell;
	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemDropPacket(Cell, 0);
}

static void netSendItemDropPacket2(uint8_t wt, uint16_t cell)
{
	TItemPos Cell(wt, cell);
	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemDropPacket(Cell, 0);
}

static void netSendItemDropPacketNew(uint8_t wt, uint16_t cell, uint32_t count)
{
	TItemPos Cell(wt, cell);

	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemDropPacketNew(Cell, 0, count);
}

static void netSendItemDropPacketNew2(uint16_t cell, uint32_t count)
{
	TItemPos Cell;
	Cell.cell = cell;

	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemDropPacketNew(Cell, 0, count);
}

static void netSendElkDropPacket(uint32_t iElk)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemDropPacket(TItemPos(RESERVED_WINDOW, 0), iElk);

}

static void netSendGoldDropPacketNew(uint32_t iElk)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemDropPacketNew(TItemPos (RESERVED_WINDOW, 0), iElk, 0);

}

static void netSendItemMovePacket(uint8_t srct, uint16_t srcc, uint8_t dstt, uint16_t dstc, uint8_t num)
{
	TItemPos Cell(srct, srcc);
	TItemPos ChangeCell(dstt, dstc);

	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemMovePacket(Cell, ChangeCell, num);
}

static void netSendItemMovePacket2(uint16_t srcc, uint16_t dstc, uint8_t num)
{
	TItemPos Cell;
	TItemPos ChangeCell;
	Cell.cell = srcc;
	ChangeCell.cell = dstc;

	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemMovePacket(Cell, ChangeCell, num);
}

static void netSendItemPickUpPacket(uint32_t vid)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendItemPickUpPacket(vid);

}

static void netSendGiveItemPacket(uint32_t iTargetVID, uint8_t wt, uint16_t ws, int32_t iItemCount)
{
	TItemPos Cell(wt, ws);
	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendGiveItemPacket(iTargetVID, Cell, iItemCount);
}

static void netSendGiveItemPacket2(uint32_t iTargetVID, uint16_t ws, int32_t iItemCount)
{
	TItemPos Cell;
	Cell.cell = ws;
	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendGiveItemPacket(iTargetVID, Cell, iItemCount);
}

static void netSetOfflinePhase()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SetOffLinePhase();

}

static void netDisconnect()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SetOffLinePhase();
	rkNetStream->Disconnect();


}

static bool netIsConnect()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	return  rkNetStream->IsOnline();
}

static void netSendChatPacket(std::string szLine, uint8_t iType)
{
	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendChatPacket(szLine.c_str(), iType);
}

static void netSendChatPacket2(std::string szLine)
{
	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendChatPacket(szLine.c_str(), CHAT_TYPE_TALKING);
}

static void netSendEmoticon(uint32_t eEmoticon)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendEmoticon(eEmoticon);

}

static void netSendWhisperPacket(std::string szName, std::string szLine)
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendWhisperPacket(szName.c_str(), szLine.c_str());

}

static void netSendCharacterPositionPacket(uint8_t iPosition)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendCharacterPositionPacket(iPosition);

}

static void netSendShopEndPacket()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendShopEndPacket();

}

static void netSendShopBuyPacket(uint8_t iCount)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendShopBuyPacket(iCount);

}

static void netSendShopSellPacket(uint8_t iSlotNumber)
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendShopSellPacket(iSlotNumber);

}

static void netSendShopSellPacketNew(uint8_t iSlotNumber, uint8_t iCount)
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendShopSellPacketNew(iSlotNumber, iCount);

}

static void netSendExchangeStartPacket(uint32_t vid)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendExchangeStartPacket(vid);

}

static void netSendExchangeItemAddPacket(uint8_t bWindowType, uint16_t wSlotIndex, uint8_t iDisplaySlotIndex)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendExchangeItemAddPacket(TItemPos(bWindowType, wSlotIndex), iDisplaySlotIndex);

}

static void netSendExchangeItemDelPacket(uint8_t pos)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendExchangeItemDelPacket(pos);

}

static void netSendExchangeElkAddPacket(uint32_t iElk)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendExchangeElkAddPacket(iElk);

}

static void netSendExchangeAcceptPacket()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendExchangeAcceptPacket();

}

static void netSendExchangeExitPacket()
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendExchangeExitPacket();

}

static void netSendOnClickPacket(uint32_t index)
{
	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->SendOnClickPacket(index);
}

static void netRegisterEmoticonString(std::string pcEmoticonString)
{

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->RegisterEmoticonString(pcEmoticonString.c_str());

}

static void netSendMessengerAddByVIDPacket(uint32_t vid)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendMessengerAddByVIDPacket(vid);
	

}

static void netSendMessengerAddByNamePacket(std::string szName)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendMessengerAddByNamePacket(szName.c_str());
	

}

static void netSendMessengerRemovePacket(std::string szKey, std::string szName)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendMessengerRemovePacket(szKey.c_str(), szName.c_str());
	

}

static void netSendPartyInvitePacket(uint32_t vid)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendPartyInvitePacket(vid);


}

static void netSendPartyInviteAnswerPacket(uint32_t vid, uint8_t answer)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendPartyInviteAnswerPacket(vid, answer);


}

static void netSendPartyExitPacket()
{
	auto rPlayer= CPythonPlayer::GetInstance();

	auto rns=CPythonNetworkStream::GetInstance();

	uint32_t dwVID = rPlayer->GetMainCharacterIndex();
	uint32_t dwPID;
	if (rPlayer->PartyMemberVIDToPID(dwVID, &dwPID))
		rns->SendPartyRemovePacket(dwPID);


}

static void netSendPartyRemovePacket(uint32_t vid)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendPartyRemovePacket(vid);


}

static void netSendPartySetStatePacket(uint32_t iVID, uint8_t iState, uint8_t iFlag)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendPartySetStatePacket(iVID, iState, iFlag);


}

static void netSendPartyUseSkillPacket(uint8_t iSkillIndex, uint32_t iVID)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendPartyUseSkillPacket(iSkillIndex, iVID);


}

static void netSendPartyParameterPacket(uint8_t iMode)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendPartyParameterPacket(iMode);


}

static void netSendSafeboxSaveMoneyPacket(uint32_t iMoney)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendSafeBoxMoneyPacket(SAFEBOX_MONEY_STATE_SAVE, iMoney);


}

static void netSendSafeboxWithdrawMoneyPacket(uint32_t iMoney)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendSafeBoxMoneyPacket(SAFEBOX_MONEY_STATE_WITHDRAW, iMoney);


}

static void netSendSafeboxCheckinPacket(uint16_t cell, uint8_t iSafeBoxPos)
{
	TItemPos InventoryPos;
	InventoryPos.window_type = INVENTORY;
	InventoryPos.cell = cell;
	auto rns = CPythonNetworkStream::GetInstance();
	rns->SendSafeBoxCheckinPacket(InventoryPos, iSafeBoxPos);
}

static void netSendSafeboxCheckinPacket2(uint8_t wt, uint16_t cell, uint8_t iSafeBoxPos)
{
	TItemPos InventoryPos(wt, cell);
	auto rns = CPythonNetworkStream::GetInstance();
	rns->SendSafeBoxCheckinPacket(InventoryPos, iSafeBoxPos);
}

static void netSendSafeboxCheckoutPacket(uint16_t cell, uint8_t iSafeBoxPos)
{
	TItemPos InventoryPos;
	InventoryPos.cell = cell;
	
	auto rns = CPythonNetworkStream::GetInstance();
	rns->SendSafeBoxCheckoutPacket(iSafeBoxPos, InventoryPos);
}

static void netSendSafeboxCheckoutPacket2(uint8_t wt, uint16_t cell, uint8_t iSafeBoxPos)
{
	TItemPos InventoryPos(wt, cell);

	auto rns = CPythonNetworkStream::GetInstance();
	rns->SendSafeBoxCheckoutPacket(iSafeBoxPos, InventoryPos);
}

static void netSendSafeboxItemMovePacket(uint8_t iSourcePos, uint8_t iTargetPos, uint8_t iCount)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendSafeBoxItemMovePacket(iSourcePos, iTargetPos, iCount);


}

static void netSendAnswerMakeGuildPacket(std::string szName)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendAnswerMakeGuildPacket(szName.c_str());


}

static void netSendQuestInputStringPacket(std::string szString)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendQuestInputStringPacket(szString.c_str());


}

static void netSendQuestConfirmPacket(uint8_t iAnswer, uint32_t iPID)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendQuestConfirmPacket(iAnswer, iPID);


}

static void netSendGuildAddMemberPacket(uint32_t iVID)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildAddMemberPacket(iVID);


}

static void netSendGuildRemoveMemberPacket(std::string szKey)
{

	CPythonGuild::TGuildMemberData * pGuildMemberData;
	if (!CPythonGuild::GetInstance()->GetMemberDataPtrByName(szKey.c_str(), &pGuildMemberData))
	{
		TraceLog("netSendGuildRemoveMemberPacket(szKey={}) - Can't Find Guild Member\n", szKey);
	}

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildRemoveMemberPacket(pGuildMemberData->dwPID);

}

static void netSendGuildChangeGradeNamePacket(uint8_t iGradeNumber, std::string szGradeName)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildChangeGradeNamePacket(iGradeNumber, szGradeName.c_str());


}

static void netSendGuildChangeGradeAuthorityPacket(uint8_t iGradeNumber, uint8_t iAuthority)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildChangeGradeAuthorityPacket(iGradeNumber, iAuthority);


}

static void netSendGuildOfferPacket(uint32_t iExperience)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildOfferPacket(iExperience);


}

static void netSendGuildPostCommentPacket(std::string szComment)
{
	auto rns = CPythonNetworkStream::GetInstance();
	rns->SendGuildPostCommentPacket(szComment.c_str());
}

static void netSendGuildDeleteCommentPacket(uint32_t iIndex)
{
	auto rns = CPythonNetworkStream::GetInstance();
	rns->SendGuildDeleteCommentPacket(iIndex);
}

static void netSendGuildRefreshCommentsPacket(uint32_t iHightestIndex)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildRefreshCommentsPacket(iHightestIndex);


}

static void netSendGuildChangeMemberGradePacket(uint32_t iPID, uint8_t iGradeNumber)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildChangeMemberGradePacket(iPID, iGradeNumber);


}

static void netSendGuildUseSkillPacket(uint32_t iSkillID, uint32_t iTargetVID)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildUseSkillPacket(iSkillID, iTargetVID);


}

static void netSendGuildChangeMemberGeneralPacket(uint32_t iPID, uint8_t iFlag)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildChangeMemberGeneralPacket(iPID, iFlag);


}

static void netSendGuildInviteAnswerPacket(uint32_t iGuildID, uint8_t iAnswer)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildInviteAnswerPacket(iGuildID, iAnswer);


}

static void netSendGuildChargeGSPPacket(uint32_t iGSP)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildChargeGSPPacket(iGSP);


}

static void netSendGuildDepositMoneyPacket(uint32_t iGSP)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildDepositMoneyPacket(iGSP);


}

static void netSendGuildWithdrawMoneyPacket(uint32_t iGSP)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendGuildWithdrawMoneyPacket(iGSP);


}

static void netSendRequestRefineInfoPacket(uint32_t iSlotIndex)
{

//	auto rns=CPythonNetworkStream::GetInstance();
//	rns->SendRequestRefineInfoPacket(iSlotIndex);
	assert(!"netSendRequestRefineInfoPacket");


}

static void netSendRefinePacket(uint8_t iSlotIndex, uint8_t iType)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendRefinePacket(iSlotIndex, iType);


}

static void netSendSelectItemPacket(uint32_t iItemPos)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->SendSelectItemPacket(iItemPos);


}

static void netSetEmpireLanguageMode(bool iMode)
{

	//auto rns=CPythonNetworkStream::GetInstance();
	gs_bEmpireLanuageEnable = iMode;


}

static void netSetSkillGroupFake(int32_t iIndex)
{

	auto rns=CPythonNetworkStream::GetInstance();
	rns->__TEST_SetSkillGroupFake(iIndex);


}

static void netSendGuildSymbol(std::string szIP, int32_t iPort, std::string szFileName, uint32_t iGuildID)
{

	CNetworkAddress kAddress;
	kAddress.Set(szIP.c_str(), iPort);

	UINT uiError;

	auto rkGuildMarkUploader=CGuildMarkUploader::GetInstance();
	if (!rkGuildMarkUploader->ConnectToSendSymbol(kAddress, 0, 0, iGuildID, szFileName.c_str(), &uiError))
	{
		assert(!"Failed connecting to send symbol");
	}


}

static void netDisconnectUploader()
{
	auto rkGuildMarkUploader=CGuildMarkUploader::GetInstance();
	rkGuildMarkUploader->Disconnect();

}

static void netRecvGuildSymbol(std::string szIP, int32_t iPort, uint32_t iGuildID)
{

	CNetworkAddress kAddress;
	kAddress.Set(szIP.c_str(), iPort);

	std::vector<uint32_t> kVec_dwGuildID;
	kVec_dwGuildID.clear();
	kVec_dwGuildID.push_back(iGuildID);

	auto rkGuildMarkDownloader=CGuildMarkDownloader::GetInstance();
	if (!rkGuildMarkDownloader->ConnectToRecvSymbol(kAddress, 0, 0, kVec_dwGuildID))
	{
		assert(!"Failed connecting to recv symbol");
	}


}


PYBIND11_EMBEDDED_MODULE(net, m)
{
	m.def("GetBettingGuildWarValue",	netGetBettingGuildWarValue);
	m.def("EnableChatInsultFilter",	netEnableChatInsultFilter);
	m.def("SetServerInfo",	netSetServerInfo);
	m.def("GetServerInfo",	netGetServerInfo);
	m.def("PreserveServerCommand",	netPreserveServerCommand);
	m.def("GetPreservedServerCommand",	netGetPreservedServerCommand);
	m.def("StartGame",	netStartGame);
	m.def("SetMarkServer",	netSetMarkServer);
	m.def("IsChatInsultIn",	netIsChatInsultIn);
	m.def("IsInsultIn",	netIsInsultIn);
	m.def("LoadInsultList",	netLoadInsultList);
	m.def("UploadMark",	netUploadMark);
	m.def("UploadSymbol",	netUploadSymbol);
	m.def("GetGuildID",	netGetGuildID);
	m.def("GetEmpireID",	netGetEmpireID);
	m.def("GetMainActorVID",	netGetMainActorVID);
	m.def("GetMainActorRace",	netGetMainActorRace);
	m.def("GetMainActorEmpire",	netGetMainActorEmpire);
	m.def("GetMainActorSkillGroup",	netGetMainActorSkillGroup);
	m.def("GetAccountCharacterSlotDataInteger",	netGetAccountCharacterSlotDataInteger);
	m.def("GetAccountCharacterSlotDataString",	netGetAccountCharacterSlotDataString);
	m.def("GetFieldMusicFileName",	netGetFieldMusicFileName);
	m.def("GetFieldMusicVolume",	netGetFieldMusicVolume);
	m.def("ToggleGameDebugInfo",	netToggleGameDebugInfo);
	m.def("SetLoginInfo",	netSetLoginInfo);
	m.def("SetPhaseWindow",	netSetPhaseWindow);
	m.def("ClearPhaseWindow",	netClearPhaseWindow);
	m.def("SetServerCommandParserWindow",	netSetServerCommandParserWindow);
	m.def("SetAccountConnectorHandler",	netSetAccountConnectorHandler);
	m.def("SetHandler",	netSetHandler);
	m.def("SetTCPRecvBufferSize",	netSetTCPRecvBufferSize);
	m.def("SetTCPSendBufferSize",	netSetTCPSendBufferSize);
	m.def("SetUDPRecvBufferSize",	netSetUDPRecvBufferSize);
	m.def("DirectEnter",	netDirectEnter);
	m.def("LogOutGame",	netLogOutGame);
	m.def("ExitGame",	netExitGame);
	m.def("ExitApplication",	netExitApplication);
	m.def("ConnectTCP",	netConnectTCP);
	m.def("ConnectUDP",	netConnectUDP);
	m.def("ConnectToAccountServer",	netConnectToAccountServer);
	m.def("SendLoginPacket",	netSendLoginPacket);
	m.def("SendSelectEmpirePacket",	netSendSelectEmpirePacket);
	m.def("SendSelectCharacterPacket",	netSendSelectCharacterPacket);
	m.def("SendChangeNamePacket",	netSendChangeNamePacket);
	m.def("SendCreateCharacterPacket",	netSendCreateCharacterPacket);
	m.def("SendDestroyCharacterPacket",	netSendDestroyCharacterPacket);
	m.def("SendEnterGamePacket",	netSendEnterGamePacket);
	m.def("SendItemUsePacket",	netSendItemUsePacket);
	m.def("SendItemUsePacket", netSendItemUsePacket2);
	m.def("SendItemUseToItemPacket",	netSendItemUseToItemPacket);
	m.def("SendItemUseToItemPacket", netSendItemUseToItemPacket2);
	m.def("SendItemDropPacket",	netSendItemDropPacket);
	m.def("SendItemDropPacket", netSendItemDropPacket2);
	m.def("SendItemDropPacketNew",	netSendItemDropPacketNew);
	m.def("SendItemDropPacketNew", netSendItemDropPacketNew2);
	m.def("SendElkDropPacket",	netSendElkDropPacket);
	m.def("SendGoldDropPacketNew",	netSendGoldDropPacketNew);
	m.def("SendItemMovePacket",	netSendItemMovePacket);
	m.def("SendItemMovePacket", netSendItemMovePacket2);
	m.def("SendItemPickUpPacket",	netSendItemPickUpPacket);
	m.def("SendGiveItemPacket",	netSendGiveItemPacket);
	m.def("SendGiveItemPacket", netSendGiveItemPacket2);
	m.def("SetOfflinePhase",	netSetOfflinePhase);
	m.def("Disconnect",	netDisconnect);
	m.def("IsConnect",	netIsConnect);
	m.def("SendChatPacket",	netSendChatPacket);
	m.def("SendChatPacket", netSendChatPacket2);
	m.def("SendEmoticon",	netSendEmoticon);
	m.def("SendWhisperPacket",	netSendWhisperPacket);
	m.def("SendCharacterPositionPacket",	netSendCharacterPositionPacket);
	m.def("SendShopEndPacket",	netSendShopEndPacket);
	m.def("SendShopBuyPacket",	netSendShopBuyPacket);
	m.def("SendShopSellPacket",	netSendShopSellPacket);
	m.def("SendShopSellPacketNew",	netSendShopSellPacketNew);
	m.def("SendExchangeStartPacket",	netSendExchangeStartPacket);
	m.def("SendExchangeItemAddPacket",	netSendExchangeItemAddPacket);
	m.def("SendExchangeItemDelPacket",	netSendExchangeItemDelPacket);
	m.def("SendExchangeElkAddPacket",	netSendExchangeElkAddPacket);
	m.def("SendExchangeAcceptPacket",	netSendExchangeAcceptPacket);
	m.def("SendExchangeExitPacket",	netSendExchangeExitPacket);
	m.def("SendOnClickPacket",	netSendOnClickPacket);
	m.def("RegisterEmoticonString",	netRegisterEmoticonString);
	m.def("SendMessengerAddByVIDPacket",	netSendMessengerAddByVIDPacket);
	m.def("SendMessengerAddByNamePacket",	netSendMessengerAddByNamePacket);
	m.def("SendMessengerRemovePacket",	netSendMessengerRemovePacket);
	m.def("SendPartyInvitePacket",	netSendPartyInvitePacket);
	m.def("SendPartyInviteAnswerPacket",	netSendPartyInviteAnswerPacket);
	m.def("SendPartyExitPacket",	netSendPartyExitPacket);
	m.def("SendPartyRemovePacket",	netSendPartyRemovePacket);
	m.def("SendPartySetStatePacket",	netSendPartySetStatePacket);
	m.def("SendPartyUseSkillPacket",	netSendPartyUseSkillPacket);
	m.def("SendPartyParameterPacket",	netSendPartyParameterPacket);
	m.def("SendSafeboxSaveMoneyPacket",	netSendSafeboxSaveMoneyPacket);
	m.def("SendSafeboxWithdrawMoneyPacket",	netSendSafeboxWithdrawMoneyPacket);
	m.def("SendSafeboxCheckinPacket",	netSendSafeboxCheckinPacket);
	m.def("SendSafeboxCheckinPacket", netSendSafeboxCheckinPacket2);
	m.def("SendSafeboxCheckoutPacket",	netSendSafeboxCheckoutPacket);
	m.def("SendSafeboxCheckoutPacket", netSendSafeboxCheckoutPacket2);
	m.def("SendSafeboxItemMovePacket",	netSendSafeboxItemMovePacket);
	m.def("SendAnswerMakeGuildPacket",	netSendAnswerMakeGuildPacket);
	m.def("SendQuestInputStringPacket",	netSendQuestInputStringPacket);
	m.def("SendQuestConfirmPacket",	netSendQuestConfirmPacket);
	m.def("SendGuildAddMemberPacket",	netSendGuildAddMemberPacket);
	m.def("SendGuildRemoveMemberPacket",	netSendGuildRemoveMemberPacket);
	m.def("SendGuildChangeGradeNamePacket",	netSendGuildChangeGradeNamePacket);
	m.def("SendGuildChangeGradeAuthorityPacket",	netSendGuildChangeGradeAuthorityPacket);
	m.def("SendGuildOfferPacket",	netSendGuildOfferPacket);
	m.def("SendGuildPostCommentPacket",	netSendGuildPostCommentPacket);
	m.def("SendGuildDeleteCommentPacket",	netSendGuildDeleteCommentPacket);
	m.def("SendGuildRefreshCommentsPacket",	netSendGuildRefreshCommentsPacket);
	m.def("SendGuildChangeMemberGradePacket",	netSendGuildChangeMemberGradePacket);
	m.def("SendGuildUseSkillPacket",	netSendGuildUseSkillPacket);
	m.def("SendGuildChangeMemberGeneralPacket",	netSendGuildChangeMemberGeneralPacket);
	m.def("SendGuildInviteAnswerPacket",	netSendGuildInviteAnswerPacket);
	m.def("SendGuildChargeGSPPacket",	netSendGuildChargeGSPPacket);
	m.def("SendGuildDepositMoneyPacket",	netSendGuildDepositMoneyPacket);
	m.def("SendGuildWithdrawMoneyPacket",	netSendGuildWithdrawMoneyPacket);
	m.def("SendRequestRefineInfoPacket",	netSendRequestRefineInfoPacket);
	m.def("SendRefinePacket",	netSendRefinePacket);
	m.def("SendSelectItemPacket",	netSendSelectItemPacket);
	m.def("SetEmpireLanguageMode",	netSetEmpireLanguageMode);
	m.def("SetSkillGroupFake",	netSetSkillGroupFake);
	m.def("SendGuildSymbol",	netSendGuildSymbol);
	m.def("DisconnectUploader",	netDisconnectUploader);
	m.def("RecvGuildSymbol",	netRecvGuildSymbol);

	m.attr("ERROR_NONE") = int32_t(CPythonNetworkStream::ERROR_NONE);
	m.attr("ERROR_CONNECT_MARK_SERVER") = int32_t(CPythonNetworkStream::ERROR_CONNECT_MARK_SERVER);
	m.attr("ERROR_LOAD_MARK") = int32_t(CPythonNetworkStream::ERROR_LOAD_MARK);
	m.attr("ERROR_MARK_WIDTH") = int32_t(CPythonNetworkStream::ERROR_MARK_WIDTH);
	m.attr("ERROR_MARK_HEIGHT") = int32_t(CPythonNetworkStream::ERROR_MARK_HEIGHT);
	m.attr("ERROR_MARK_UPLOAD_NEED_RECONNECT") = int32_t(CPythonNetworkStream::ERROR_MARK_UPLOAD_NEED_RECONNECT);
	m.attr("ERROR_MARK_CHECK_NEED_RECONNECT") = int32_t(CPythonNetworkStream::ERROR_MARK_CHECK_NEED_RECONNECT);
	m.attr("PHASE_WINDOW_LOGIN") = int32_t(CPythonNetworkStream::PHASE_WINDOW_LOGIN);
	m.attr("PHASE_WINDOW_SELECT") = int32_t(CPythonNetworkStream::PHASE_WINDOW_SELECT);
	m.attr("PHASE_WINDOW_CREATE") = int32_t(CPythonNetworkStream::PHASE_WINDOW_CREATE);
	m.attr("PHASE_WINDOW_LOAD") = int32_t(CPythonNetworkStream::PHASE_WINDOW_LOAD);
	m.attr("PHASE_WINDOW_GAME") = int32_t(CPythonNetworkStream::PHASE_WINDOW_GAME);
	m.attr("PHASE_WINDOW_EMPIRE") = int32_t(CPythonNetworkStream::PHASE_WINDOW_EMPIRE);
	m.attr("PHASE_WINDOW_LOGO") = int32_t(CPythonNetworkStream::PHASE_WINDOW_LOGO);
	m.attr("ACCOUNT_CHARACTER_SLOT_ID") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_ID);
	m.attr("ACCOUNT_CHARACTER_SLOT_NAME") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_NAME);
	m.attr("ACCOUNT_CHARACTER_SLOT_RACE") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_RACE);
	m.attr("ACCOUNT_CHARACTER_SLOT_LEVEL") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_LEVEL);
	m.attr("ACCOUNT_CHARACTER_SLOT_STR") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_STR);
	m.attr("ACCOUNT_CHARACTER_SLOT_DEX") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_DEX);
	m.attr("ACCOUNT_CHARACTER_SLOT_INT") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_INT);
	m.attr("ACCOUNT_CHARACTER_SLOT_HTH") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_HTH);
	m.attr("ACCOUNT_CHARACTER_SLOT_PLAYTIME") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_PLAYTIME);
	m.attr("ACCOUNT_CHARACTER_SLOT_FORM") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_FORM);
	m.attr("ACCOUNT_CHARACTER_SLOT_ADDR") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_ADDR);
	m.attr("ACCOUNT_CHARACTER_SLOT_PORT") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_PORT);
	m.attr("ACCOUNT_CHARACTER_SLOT_GUILD_ID") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_GUILD_ID);
	m.attr("ACCOUNT_CHARACTER_SLOT_GUILD_NAME") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_GUILD_NAME);
	m.attr("ACCOUNT_CHARACTER_SLOT_CHANGE_NAME_FLAG") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_CHANGE_NAME_FLAG);
	m.attr("ACCOUNT_CHARACTER_SLOT_HAIR") = int32_t(CPythonNetworkStream::ACCOUNT_CHARACTER_SLOT_HAIR);
	m.attr("SERVER_COMMAND_LOG_OUT") = int32_t(CPythonNetworkStream::SERVER_COMMAND_LOG_OUT);
	m.attr("SERVER_COMMAND_RETURN_TO_SELECT_CHARACTER") = int32_t(CPythonNetworkStream::SERVER_COMMAND_RETURN_TO_SELECT_CHARACTER);
	m.attr("SERVER_COMMAND_QUIT") = int32_t(CPythonNetworkStream::SERVER_COMMAND_QUIT);
	m.attr("EMPIRE_A") =	1;
	m.attr("EMPIRE_B") =	2;
	m.attr("EMPIRE_C") =	3;
	m.attr("DS_SUB_HEADER_REFINE_FAIL") = int32_t(DS_SUB_HEADER_REFINE_FAIL);
	m.attr("DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE") = int32_t(DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE);
	m.attr("DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL") = int32_t(DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL);
	m.attr("DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY") = int32_t(DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY);
	m.attr("DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL") = int32_t(DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL);
	m.attr("DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL") = int32_t(DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL);
	m.attr("DS_SUB_HEADER_REFINE_SUCCEED") = int32_t(DS_SUB_HEADER_REFINE_SUCCEED);
}
