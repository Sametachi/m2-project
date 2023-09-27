#include "StdAfx.h"
#include "PythonApplication.h"
#include "PythonEventManager.h"
#include "PythonNetworkStream.h"
#include "PythonNonPlayer.h"
#include "PythonMiniMap.h"

#include <Gamelib/ItemManager.h>
#include <EterLib/GrpFontManager.h>
#include <EterLib/Engine.h>

const int32_t c_lNormal_Waiting_Time = 5;
const int32_t c_fLine_Temp = 16;

void ShowArgument(ScriptGroup::TArgList& rArgumentList)
{
	for (auto itor = rArgumentList.begin(); itor != rArgumentList.end(); ++itor)
	{
		const std::string& rName = (*itor).strName;
		ConsoleLog(rName.c_str());
	}
}

const std::string& GetArgumentString(const char* c_szName, ScriptGroup::TArgList& rArgumentList)
{
	for (auto itor = rArgumentList.begin(); itor != rArgumentList.end(); ++itor)
	{
		const auto& rName = (*itor).strName;

		if (0 == rName.compare(c_szName))
			return (*itor).strValue;
	}

	static std::string strEmptyValue = "";
	return strEmptyValue;
}

const char* GetArgument(const char* c_szName, ScriptGroup::TArgList& rArgumentList)
{
	return GetArgumentString(c_szName, rArgumentList).c_str();
}

void GetCameraSettingFromArgList(ScriptGroup::TArgList& rArgList, SCameraSetting* pCameraSetting)
{
	int32_t ix = atoi(GetArgument("x", rArgList));
	int32_t iy = atoi(GetArgument("y", rArgList));
	int32_t iz = atoi(GetArgument("z", rArgList));
	int32_t iUpDir = atoi(GetArgument("up", rArgList));
	int32_t iViewDir = atoi(GetArgument("view", rArgList));
	int32_t iCrossDir = atoi(GetArgument("cross", rArgList));
	int32_t iDistance = atoi(GetArgument("distance", rArgList));
	int32_t iRot = atoi(GetArgument("rot", rArgList));
	int32_t iPitch = atoi(GetArgument("pitch", rArgList));

	ZeroMemory(pCameraSetting, sizeof(SCameraSetting));
	pCameraSetting->v3CenterPosition.x = float(ix);
	pCameraSetting->v3CenterPosition.y = float(iy);
	pCameraSetting->v3CenterPosition.z = float(iz);
	pCameraSetting->kCmrPos.m_fUpDir = float(iUpDir);
	pCameraSetting->kCmrPos.m_fViewDir = float(iViewDir);
	pCameraSetting->kCmrPos.m_fCrossDir = float(iCrossDir);
	pCameraSetting->fZoom = float(iDistance);
	pCameraSetting->fRotation = float(iRot);
	pCameraSetting->fPitch = float(iPitch);
}

void CPythonEventManager::__InitEventSet(TEventSet& rEventSet)
{
	rEventSet.ix = 0;
	rEventSet.iy = 0;
	rEventSet.iWidth = 0;
	rEventSet.iyLocal = 0;

	rEventSet.isLock = false;
	rEventSet.lLastDelayTime = 0;
	rEventSet.iCurrentLetter = 0;
	rEventSet.CurrentColor = D3DXCOLOR(1, 1, 1, 1);
	rEventSet.strCurrentLine = "";

	rEventSet.currentLine.textInstance.reset();
	rEventSet.currentLine.centered = false;
	rEventSet.ScriptTextLineList.clear();

	rEventSet.isConfirmWait = false;
	rEventSet.pConfirmTimeTextLine = nullptr;
	rEventSet.iConfirmEndTime = 0;

	rEventSet.lWaitingTime = c_lNormal_Waiting_Time;
	rEventSet.iRestrictedCharacterCount = 40;

	rEventSet.iVisibleStartLine = 0;
	rEventSet.iVisibleLineCount = BOX_VISIBLE_LINE_COUNT;

	rEventSet.iAdjustLine = 0;
	rEventSet.isWaitFlag = false;
	rEventSet.centerTextLines = false;
	rEventSet.poEventHandler = nullptr;
	rEventSet.isQuestInfo = false;

	__InsertLine(rEventSet);
}

int32_t CPythonEventManager::RegisterEventSet(const std::string& c_szFileName)
{
	auto vfs_string = CallFS().LoadFileToString(CallFS(), c_szFileName);
	if (!vfs_string)
	{
		SysLog("Failed to load {0}", c_szFileName);
		return false;
	}

	TEventSet* pEventSet = m_EventSetPool.Alloc();
	if (!pEventSet)
		return -1;

	if (!pEventSet->ScriptGroup.Create(vfs_string.value()))
	{
		__ClearEventSetp(pEventSet);
		return -1;
	}

	__InitEventSet(*pEventSet);

	int32_t iEmptySlotIndex = GetEmptyEventSetSlot();
	m_EventSetVector[iEmptySlotIndex] = pEventSet;
	return iEmptySlotIndex;
}

int32_t CPythonEventManager::RegisterEventSetFromString(const std::string& strScript, bool bIsQuestInfo)
{
	TEventSet* pEventSet = m_EventSetPool.Alloc();
	if (!pEventSet)
		return -1;

	const auto lines = pEventSet->ScriptGroup.Create(strScript);
	if (!lines)
	{
		__ClearEventSetp(pEventSet);
		return -1;
	}

	pEventSet->iTotalLineCount = lines;
	pEventSet->szFileName[0] = 0;
	__InitEventSet(*pEventSet);
	pEventSet->isQuestInfo = bIsQuestInfo;

	ScriptGroup::SCmd ScriptCommand;
	int32_t pEventPosition;
	int32_t iEventType;
	if (pEventSet->ScriptGroup.ReadCmd(ScriptCommand))
	{
		if (GetScriptEventIndex(ScriptCommand.name.c_str(), &pEventPosition, &iEventType))
		{
			if (EVENT_TYPE_RUN_CINEMA == iEventType)
			{
				__ClearEventSetp(pEventSet);
				return RegisterEventSet(GetArgumentString("value", ScriptCommand.argList));
			}
		}
	}

	int32_t iEmptySlotIndex = GetEmptyEventSetSlot();
	m_EventSetVector[iEmptySlotIndex] = pEventSet;
	return iEmptySlotIndex;
}

void CPythonEventManager::ClearEventSeti(int iIndex)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	__ClearEventSetp(m_EventSetVector[iIndex]);
	m_EventSetVector[iIndex] = NULL;
}

void CPythonEventManager::__ClearEventSetp(TEventSet* pEventSet)
{
	if (!pEventSet)
		return;

	pEventSet->ScriptTextLineList.clear();
	pEventSet->currentLine.textInstance.reset();
	pEventSet->strCurrentLine = "";
	pEventSet->iCurrentLetter = 0;

	m_EventSetPool.Free(pEventSet);
}

uint32_t CPythonEventManager::GetEmptyEventSetSlot()
{
	for (uint32_t i = 0; i < m_EventSetVector.size(); ++i)
	{
		if (nullptr == m_EventSetVector[i])
		{
			return i;
		}
	}

	m_EventSetVector.emplace_back(nullptr);
	return m_EventSetVector.size() - 1;
}

void CPythonEventManager::SetRestrictedCount(int32_t iIndex, int32_t iCount)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	if (!pEventSet)
	{
		SysLog("m_EventSetVector[iIndex={0}]==nullptr", iIndex);
		return;
	}

	pEventSet->iRestrictedCharacterCount = iCount;
}

void CPythonEventManager::SetEventHandler(int32_t iIndex, pybind11::handle poEventHandler)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	if (!pEventSet)
	{
		SysLog("m_EventSetVector[iIndex={0}]==nullptr", iIndex);
		return;
	}

	pEventSet->poEventHandler = poEventHandler;
}

int32_t CPythonEventManager::GetEventSetLocalYPosition(int32_t iIndex)
{
	if (!CheckEventSetIndex(iIndex))
		return 0;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	if (!pEventSet)
		return 0;

	return pEventSet->iyLocal;
}

void CPythonEventManager::AddEventSetLocalYPosition(int32_t iIndex, int32_t iAddValue)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	if (!pEventSet)
		return;

	pEventSet->iyLocal += iAddValue;
}

void CPythonEventManager::InsertText(int32_t iIndex, const char* c_szText, int32_t iX_pos)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	if (!pEventSet)
		return;

	pEventSet->strCurrentLine = c_szText;

	pEventSet->currentLine.ixLocal = iX_pos;
	pEventSet->currentLine.centered = true;

	if (pEventSet->currentLine.textInstance)
		pEventSet->currentLine.textInstance->SetValue(c_szText);

	__InsertLine(*pEventSet);
}

void CPythonEventManager::UpdateEventSet(int32_t iIndex, int32_t ix, int32_t iy)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	if (!pEventSet)
		return;

	pEventSet->ix = ix;
	pEventSet->iy = iy;

	if (pEventSet->isConfirmWait)
	{
		int32_t iLeftTime = std::max<int32_t>(0, pEventSet->iConfirmEndTime - timeGetTime() / 1000);
		pEventSet->pConfirmTimeTextLine->SetValue(_getf(m_strLeftTimeString.c_str(), iLeftTime));
	}

	if (pEventSet->isWaitFlag)
		return;

	// Process EventSet
	long lElapsedTime = CTimer::GetInstance()->GetElapsedMilliecond();

	pEventSet->lLastDelayTime = std::max<int32_t>(0, pEventSet->lLastDelayTime - lElapsedTime);

	while (lElapsedTime > 0)
	{
		pEventSet->lLastDelayTime -= lElapsedTime;

		if (pEventSet->lLastDelayTime <= 0)
		{
			lElapsedTime = -pEventSet->lLastDelayTime;
			if (lElapsedTime <= 0)
				break;

			ProcessEventSet(pEventSet);

			if (pEventSet->lLastDelayTime < 0)
			{
				pEventSet->lLastDelayTime = 0;
				break;
			}
		}
		else
			break;
	}
}

void CPythonEventManager::SetEventSetWidth(int32_t iIndex, int32_t iWidth)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	if (!pEventSet)
		return;

	pEventSet->iWidth = iWidth;
}

void CPythonEventManager::SetFontColor(int32_t iIndex, float r, float g, float b)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	if (!pEventSet)
		return;

	pEventSet->CurrentColor = D3DXCOLOR(r, g, b, 0.7f);
	pEventSet->currentLine.textInstance->SetColor(pEventSet->CurrentColor);
	pEventSet->currentLine.textInstance->Update();
	for (const auto& textLine : pEventSet->ScriptTextLineList)
	{
		textLine.textInstance->SetColor(pEventSet->CurrentColor);
		textLine.textInstance->Update();
	}
}

void CPythonEventManager::ProcessEventSet(TEventSet* pEventSet)
{
	if (pEventSet->isLock)
	{
		return;
	}

	auto rApp = CPythonApplication::GetInstance();

	ScriptGroup::SCmd ScriptCommand;
	if (!pEventSet->ScriptGroup.GetCmd(ScriptCommand))
	{
		pEventSet->isLock = true;
		return;
	}

	int32_t pEventPosition;
	int32_t iEventType;
	if (!GetScriptEventIndex(ScriptCommand.name.c_str(), &pEventPosition, &iEventType))
	{
		return;
	}

	switch (iEventType)
	{
	case EVENT_TYPE_LETTER:
	{
		const std::string& c_rstValue = GetArgumentString("value", ScriptCommand.argList);
		pEventSet->strCurrentLine.append(c_rstValue);
		pEventSet->currentLine.textInstance->SetValue(pEventSet->strCurrentLine);
		pEventSet->currentLine.textInstance->SetColor(pEventSet->CurrentColor);
		pEventSet->iCurrentLetter += c_rstValue.length();

		if (pEventSet->iCurrentLetter >= pEventSet->iRestrictedCharacterCount)
		{
			__InsertLine(*pEventSet);
		}

		pEventSet->lLastDelayTime = pEventSet->lWaitingTime;
		break;
	}

	case EVENT_TYPE_DELAY:
	{
		if (EVENT_POSITION_START == pEventPosition)
		{
			pEventSet->lWaitingTime = atoi(GetArgument("value", ScriptCommand.argList));
		}
		else
		{
			pEventSet->lWaitingTime = c_lNormal_Waiting_Time;
		}
		break;
	}

	case EVENT_TYPE_COLOR:
	{
		if (EVENT_POSITION_START == pEventPosition)
		{
			pEventSet->CurrentColor.r = (float)std::stof(GetArgument("r", ScriptCommand.argList));
			pEventSet->CurrentColor.g = (float)std::stof(GetArgument("g", ScriptCommand.argList));
			pEventSet->CurrentColor.b = (float)std::stof(GetArgument("b", ScriptCommand.argList));
			pEventSet->CurrentColor.a = 1.0f;
		}
		else
		{
			pEventSet->CurrentColor.r = 1.0f;
			pEventSet->CurrentColor.g = 1.0f;
			pEventSet->CurrentColor.a = 1.0f;
			pEventSet->CurrentColor.b = 1.0f;
		}
		break;
	}

	case EVENT_TYPE_COLOR256:
	{
		if (EVENT_POSITION_START == pEventPosition)
		{
			pEventSet->CurrentColor.r = float(std::stof(GetArgument("r", ScriptCommand.argList)) / 255.0f);
			pEventSet->CurrentColor.g = float(std::stof(GetArgument("g", ScriptCommand.argList)) / 255.0f);
			pEventSet->CurrentColor.b = float(std::stof(GetArgument("b", ScriptCommand.argList)) / 255.0f);
			pEventSet->CurrentColor.a = 1.0f;
		}
		else
		{
			pEventSet->CurrentColor.r = 1.0f;
			pEventSet->CurrentColor.g = 1.0f;
			pEventSet->CurrentColor.a = 1.0f;
			pEventSet->CurrentColor.b = 1.0f;
		}
		break;
	}

	case EVENT_TYPE_ENTER:
	{
		__InsertLine(*pEventSet);
		break;
	}

	case EVENT_TYPE_WAIT:
	{
		pEventSet->iyLocal = 0;
		pEventSet->isLock = true;
		break;
	}

	case EVENT_TYPE_NEXT:
	{
		MakeNextButton(pEventSet, BUTTON_TYPE_NEXT);
		pEventSet->iAdjustLine += 2;
		break;
	}

	case EVENT_TYPE_DONE:
	{
		MakeNextButton(pEventSet, BUTTON_TYPE_DONE);
		PyCallClassMemberFunc(pEventSet->poEventHandler, "DoneEvent");
		pEventSet->iAdjustLine += 2;
		break;
	}

	case EVENT_TYPE_CLEAR:
	{
		ClearLine(pEventSet);
		break;
	}

	case EVENT_TYPE_QUESTION:
	{
		MakeQuestion(pEventSet, ScriptCommand.argList);
		break;
	}

	case EVENT_TYPE_LEFT_IMAGE:
	{
		PyCallClassMemberFunc(pEventSet->poEventHandler, "OnLeftImage", GetArgument("src", ScriptCommand.argList));
		break;
	}

	case EVENT_TYPE_TOP_IMAGE:
	{
		PyCallClassMemberFunc(pEventSet->poEventHandler, "OnTopImage", GetArgument("src", ScriptCommand.argList));
		break;
	}

	case EVENT_TYPE_BACKGROUND_IMAGE:
	{
		PyCallClassMemberFunc(pEventSet->poEventHandler, "OnBackgroundImage", GetArgument("src", ScriptCommand.argList));
		break;
	}

	case EVENT_TYPE_IMAGE:
	{
		int32_t x = atoi(GetArgument("x", ScriptCommand.argList));
		int32_t y = atoi(GetArgument("y", ScriptCommand.argList));
		const char* src = GetArgument("src", ScriptCommand.argList);

		PyCallClassMemberFunc(pEventSet->poEventHandler, "OnImage", x, y, src);
		break;
	}

	case EVENT_TYPE_INSERT_IMAGE:
	{
		const std::string& imageFile = GetArgumentString("image_name", ScriptCommand.argList);
		const char* title = GetArgument("title", ScriptCommand.argList);
		const char* desc = GetArgument("desc", ScriptCommand.argList);
		int32_t index = atoi(GetArgument("index", ScriptCommand.argList));
		int32_t total = atoi(GetArgument("total", ScriptCommand.argList));

		if (imageFile.empty())
		{
			const char* imageType = GetArgument("image_type", ScriptCommand.argList);
			int32_t iItemIndex = atoi(GetArgument("idx", ScriptCommand.argList));
			PyCallClassMemberFunc(pEventSet->poEventHandler, "OnInsertItemIcon", imageType, iItemIndex, title, desc, index, total);
		}
		else
		{
			PyCallClassMemberFunc(pEventSet->poEventHandler, "OnInsertImage", imageFile, title, title, desc, index, total);
		}
		pEventSet->iAdjustLine += 2;
		break;
	}

	case EVENT_TYPE_ADD_MAP_SIGNAL:
	{
		float x, y;
		x = (float)std::stof(GetArgument("x", ScriptCommand.argList));
		y = (float)std::stof(GetArgument("y", ScriptCommand.argList));
		CPythonMiniMap::GetInstance()->AddSignalPoint(x, y);
		CPythonMiniMap::GetInstance()->OpenAtlasWindow();
		break;
	}

	case EVENT_TYPE_CLEAR_MAP_SIGNAL:
	{
		CPythonMiniMap::GetInstance()->ClearAllSignalPoint();
		break;
	}

	case EVENT_TYPE_QUEST_BUTTON_CLOSE:
	{
		PyCallClassMemberFunc(m_poInterface, "BINARY_ClearQuest", atoi(GetArgument("idx", ScriptCommand.argList)));
		break;
	}

	case EVENT_TYPE_QUEST_BUTTON:
	{
		const std::string& c_rstType = GetArgumentString("icon_type", ScriptCommand.argList);
		const std::string& c_rstFile = GetArgumentString("icon_name", ScriptCommand.argList);

		int32_t idx = atoi(GetArgument("idx", ScriptCommand.argList));
		const char* name = GetArgument("name", ScriptCommand.argList);

		if (!strcmp(name, "234?....")) // What the actual fuck does that mean?
		{
			PyCallClassMemberFunc(m_poInterface, "BINARY_RecvQuest", idx, name, "highlight", "");
		}
		else
		{
			if (c_rstFile.empty())
			{
				PyCallClassMemberFunc(m_poInterface, "RecvQuest", idx, name);
			}
			else
			{
				PyCallClassMemberFunc(m_poInterface, "BINARY_RecvQuest",
					idx, name, c_rstType, c_rstFile);
			}
		}
		break;
	}
	case EVENT_TYPE_SET_MESSAGE_POSITION:
	{
		break;
	}
	case EVENT_TYPE_ADJUST_MESSAGE_POSITION:
	{
		break;
	}
	case EVENT_TYPE_SET_CENTER_MAP_POSITION:
	{
		CPythonMiniMap::GetInstance()->SetAtlasCenterPosition(atoi(GetArgument("x", ScriptCommand.argList)), atoi(GetArgument("y", ScriptCommand.argList)));
		break;
	}
	case EVENT_TYPE_SLEEP:
		pEventSet->lLastDelayTime = atoi(GetArgument("value", ScriptCommand.argList));
		break;
	case EVENT_TYPE_SET_CAMERA:
	{
		SCameraSetting CameraSetting;
		GetCameraSettingFromArgList(ScriptCommand.argList, &CameraSetting);
		rApp->SetEventCamera(CameraSetting);
		break;
	}
	case EVENT_TYPE_BLEND_CAMERA:
	{
		SCameraSetting CameraSetting;
		GetCameraSettingFromArgList(ScriptCommand.argList, &CameraSetting);

		float fBlendTime = atoi(GetArgument("blendtime", ScriptCommand.argList));

		rApp->BlendEventCamera(CameraSetting, fBlendTime);
		break;
	}
	case EVENT_TYPE_RESTORE_CAMERA:
	{
		rApp->SetDefaultCamera();
		break;
	}
	case EVENT_TYPE_FADE_OUT:
	{
		float fSpeed = (float)std::stof(GetArgument("speed", ScriptCommand.argList));
		PyCallClassMemberFunc(pEventSet->poEventHandler, "FadeOut", fSpeed);
		pEventSet->isWaitFlag = true;
		break;
	}
	case EVENT_TYPE_FADE_IN:
	{
		PyCallClassMemberFunc(pEventSet->poEventHandler, "FadeIn", std::stof(GetArgument("speed", ScriptCommand.argList)));
		pEventSet->isWaitFlag = true;
		break;
	}
	case EVENT_TYPE_WHITE_OUT:
	{
		PyCallClassMemberFunc(pEventSet->poEventHandler, "WhiteOut", std::stof(GetArgument("speed", ScriptCommand.argList)));
		pEventSet->isWaitFlag = true;
		break;
	}
	case EVENT_TYPE_WHITE_IN:
	{
		PyCallClassMemberFunc(pEventSet->poEventHandler, "WhiteIn", std::stof(GetArgument("speed", ScriptCommand.argList)));
		pEventSet->isWaitFlag = true;
		break;
	}
	case EVENT_TYPE_CLEAR_TEXT:
	{
		ClearLine(pEventSet);
		break;
	}
	case EVENT_TYPE_TEXT_HORIZONTAL_ALIGN_CENTER:
	{
		// Center current line
		pEventSet->currentLine.centered = true;

		// Center following lines as well
		pEventSet->centerTextLines = true;
		break;
	}
	case EVENT_TYPE_TITLE_IMAGE:
	{
		PyCallClassMemberFunc(pEventSet->poEventHandler, "OnTitleImage", GetArgument("src", ScriptCommand.argList));
		break;
	}
	case EVENT_TYPE_DUNGEON_RESULT:
	{
		int32_t killstone_count = atoi(GetArgument("killstone_count", ScriptCommand.argList));
		int32_t killmob_count = atoi(GetArgument("killmob_count", ScriptCommand.argList));
		int32_t find_hidden = atoi(GetArgument("find_hidden", ScriptCommand.argList));
		int32_t hidden_total = atoi(GetArgument("hidden_total", ScriptCommand.argList));
		int32_t use_potion = atoi(GetArgument("use_potion", ScriptCommand.argList));
		int32_t is_revived = atoi(GetArgument("is_revived", ScriptCommand.argList));
		int32_t killallmob = atoi(GetArgument("killallmob", ScriptCommand.argList));
		int32_t total_time = atoi(GetArgument("total_time", ScriptCommand.argList));
		int32_t bonus_exp = atoi(GetArgument("bonus_exp", ScriptCommand.argList));

		PyCallClassMemberFunc(m_poInterface, "ShowDungeonResult",
				killstone_count,
				killmob_count,
				find_hidden,
				hidden_total,
				use_potion,
				is_revived,
				killallmob,
				total_time,
				bonus_exp);
		break;
	}
	case EVENT_TYPE_ITEM_NAME:
	{
		int32_t iIndex = atoi(GetArgument("value", ScriptCommand.argList));
		CItemData* pItemData;
		if (CItemManager::GetInstance()->GetItemDataPointer(iIndex, &pItemData))
		{
			pEventSet->strCurrentLine.append(pItemData->GetName());
			pEventSet->currentLine.textInstance->SetValue(pEventSet->strCurrentLine);
			pEventSet->currentLine.textInstance->SetColor(1.0f, 0.2f, 0.2f);
			pEventSet->iCurrentLetter += strlen(pItemData->GetName());

			if (pEventSet->iCurrentLetter >= pEventSet->iRestrictedCharacterCount)
				__InsertLine(*pEventSet);

			pEventSet->lLastDelayTime = pEventSet->lWaitingTime;
		}

		break;
	}
	case EVENT_TYPE_MONSTER_NAME:
	{
		// Load monster name from mob_names.txt
		int32_t iIndex = atoi(GetArgument("value", ScriptCommand.argList));
		auto optName = CPythonNonPlayer::GetInstance()->GetName(iIndex);

		if (optName)
		{
			pEventSet->strCurrentLine.append(optName.value());
			pEventSet->currentLine.textInstance->SetValue(pEventSet->strCurrentLine);
			pEventSet->iCurrentLetter += optName.value().length();

			if (pEventSet->iCurrentLetter >= pEventSet->iRestrictedCharacterCount)
				__InsertLine(*pEventSet);

			pEventSet->lLastDelayTime = pEventSet->lWaitingTime;
		}

		break;
	}
	case EVENT_TYPE_WINDOW_SIZE:
	{
		int32_t iWidth = atoi(GetArgument("width", ScriptCommand.argList));
		int32_t iHeight = atoi(GetArgument("height", ScriptCommand.argList));
		PyCallClassMemberFunc(pEventSet->poEventHandler, "OnSize", iWidth, iHeight);
		break;
	}
	case EVENT_TYPE_INPUT:
	{
		__InsertLine(*pEventSet);
		PyCallClassMemberFunc(pEventSet->poEventHandler, "OnInput");
		break;
	}
	case EVENT_TYPE_CONFIRM_WAIT:
	{
		int32_t iTimeOut = atoi(GetArgument("timeout", ScriptCommand.argList));
		pEventSet->isConfirmWait = TRUE;
		pEventSet->pConfirmTimeTextLine = pEventSet->currentLine.textInstance.get();
		pEventSet->iConfirmEndTime = timeGetTime() / 1000 + iTimeOut;
		__InsertLine(*pEventSet, TRUE);
		MakeNextButton(pEventSet, BUTTON_TYPE_CANCEL);
		break;
	}
	case EVENT_TYPE_END_CONFIRM_WAIT:
	{
		for (uint32_t i = 0; i < m_EventSetVector.size(); ++i)
		{
			if (NULL == m_EventSetVector[i])
			{
				continue;
			}

			TEventSet* pSet = m_EventSetVector[i];
			if (!pSet->isConfirmWait)
			{
				continue;
			}

			pSet->isConfirmWait = FALSE;
			pSet->pConfirmTimeTextLine = NULL;
			pSet->iConfirmEndTime = 0;

			PyCallClassMemberFunc(pSet->poEventHandler, "CloseSelf");
		}
		break;
	}
	case EVENT_TYPE_SELECT_ITEM:
	{
		PyCallClassMemberFunc(m_poInterface, "BINARY_OpenSelectItemWindow");
		break;
	}
	}
}

void CPythonEventManager::RenderEventSet(int32_t iIndex)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	if (!pEventSet)
		return;

	int32_t iCount = 0;

	for (auto itor = pEventSet->ScriptTextLineList.begin(); itor != pEventSet->ScriptTextLineList.end(); ++itor, ++iCount)
	{
		if (iCount < pEventSet->iVisibleStartLine)
			continue;

		if (iCount >= pEventSet->iVisibleStartLine + pEventSet->iVisibleLineCount)
			continue;

		RenderTextLine(*pEventSet, *itor);
	}

	if (iCount >= pEventSet->iVisibleStartLine &&
		iCount < pEventSet->iVisibleStartLine + pEventSet->iVisibleLineCount)
	{
		pEventSet->currentLine.iyLocal = pEventSet->iyLocal;
		RenderTextLine(*pEventSet, pEventSet->currentLine);
	}
}

void CPythonEventManager::Skip(int32_t iIndex)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];

	if (true == pEventSet->isLock)
	{
		pEventSet->lLastDelayTime = 0;
		pEventSet->isLock = false;
	}
	else
	{
		pEventSet->lLastDelayTime = -1000;
	}
}

bool CPythonEventManager::IsWait(int32_t iIndex)
{
	if (!CheckEventSetIndex(iIndex))
		return false;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	if (!pEventSet)
	{
		return false;
	}

	return pEventSet->isLock;
}

void CPythonEventManager::EndEventProcess(int32_t iIndex)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	if (!pEventSet)
	{
		return;
	}

	pEventSet->isWaitFlag = false;
}

void CPythonEventManager::MakeNextButton(TEventSet* pEventSet, int32_t iButtonType)
{
	__AddSpace(*pEventSet, c_fLine_Temp + 5);
	PyCallClassMemberFunc(pEventSet->poEventHandler, "MakeNextButton", iButtonType);
}

void CPythonEventManager::MakeQuestion(TEventSet* pEventSet, ScriptGroup::TArgList& rArgumentList)
{
	if (rArgumentList.empty())
		return;

	PyCallClassMemberFunc(pEventSet->poEventHandler, "MakeQuestion", rArgumentList);

	pEventSet->nAnswer = rArgumentList.size();

	int32_t iIndex = 0;
	for (const auto& rArgument : rArgumentList)
	{
		PyCallClassMemberFunc(pEventSet->poEventHandler, "AppendQuestion", rArgument.strValue, iIndex);
		++iIndex;
	}
}


void CPythonEventManager::SelectAnswer(int32_t iIndex, int32_t iAnswer)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	CPythonNetworkStream::GetInstance()->SendScriptAnswerPacket(iAnswer);
}

void CPythonEventManager::SetVisibleStartLine(int32_t iIndex, int32_t iStartLine)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];

	if (uint32_t(iStartLine) > pEventSet->ScriptTextLineList.size())
		return;

	pEventSet->iVisibleStartLine = iStartLine;
}

void CPythonEventManager::SetVisibleLineCount(int32_t iIndex, int32_t iLineCount)
{
	if (!CheckEventSetIndex(iIndex))
		return;

	TEventSet* pEventSet = m_EventSetVector[iIndex];

	pEventSet->iVisibleLineCount = iLineCount;
}

int32_t CPythonEventManager::GetVisibleStartLine(int32_t iIndex)
{
	if (!CheckEventSetIndex(iIndex))
		return 0;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	return pEventSet->iVisibleStartLine;
}

int32_t CPythonEventManager::GetLineCount(int32_t iIndex)
{
	if (!CheckEventSetIndex(iIndex))
		return 0;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	return pEventSet->ScriptTextLineList.size() + pEventSet->iAdjustLine;
}

int32_t CPythonEventManager::GetTotalLineCount(int32_t iIndex)
{
	if (!CheckEventSetIndex(iIndex))
		return 0;

	TEventSet* pEventSet = m_EventSetVector[iIndex];
	return pEventSet->iTotalLineCount;
}

void CPythonEventManager::ClearLine(TEventSet* pEventSet)
{
	if (!pEventSet)
		return;

	pEventSet->currentLine.textInstance.reset();
	pEventSet->ScriptTextLineList.clear();

	__InsertLine(*pEventSet);
}

void CPythonEventManager::__InsertLine(TEventSet& rEventSet, bool isCenter)
{
	if (rEventSet.currentLine.textInstance)
	{
		rEventSet.currentLine.iyLocal = rEventSet.iyLocal;
		rEventSet.ScriptTextLineList.push_back(std::move(rEventSet.currentLine));
		__AddSpace(rEventSet, c_fLine_Temp);
	}

	auto pkDefaultFont = Engine::GetFontManager().GetDefaultFont();
	if (!pkDefaultFont)
	{
		SysLog("CPythonEventManager::InsertLine - CANNOT_FIND_DEFAULT_FONT");
		return;
	}

	rEventSet.currentLine.textInstance.reset(new CGraphicTextInstance());
	if (!rEventSet.currentLine.textInstance)
	{
		SysLog("CPythonEventManager::InsertLine - OUT_OF_TEXT_LINE");
		return;
	}

	rEventSet.currentLine.textInstance->SetTextPointer(pkDefaultFont);
	rEventSet.currentLine.textInstance->SetColor(1.0f, 1.0f, 1.0f);
	rEventSet.currentLine.textInstance->SetValue("");

	if (rEventSet.centerTextLines || isCenter)
		rEventSet.currentLine.centered = true;
	else
		rEventSet.currentLine.centered = false;

	rEventSet.currentLine.ixLocal = 0;
	rEventSet.currentLine.iyLocal = rEventSet.iyLocal;
	rEventSet.iCurrentLetter = 0;
	rEventSet.strCurrentLine = "";
}

void CPythonEventManager::RenderTextLine(TEventSet& set, TTextLine& line)
{
	line.textInstance->Update();

	int32_t x = set.ix, y = set.iy;

	if (line.centered)
	{
		int32_t localX = line.ixLocal;
		if (0 == localX)
			localX = set.iWidth;

		x += (localX - line.textInstance->GetWidth()) / 2;
	}
	else
	{
		x += line.ixLocal;
	}

	y += line.iyLocal;
	line.textInstance->Render(x, y);
}

void CPythonEventManager::__AddSpace(TEventSet& rEventSet, int32_t iSpace)
{
	rEventSet.iyLocal += iSpace;
}

bool CPythonEventManager::GetScriptEventIndex(const char* c_szName, int32_t* pEventPosition, int32_t* pEventType)
{
	const char* c_szEventName;

	if ('/' == c_szName[0])
	{
		*pEventPosition = EVENT_POSITION_END;
		c_szEventName = &c_szName[1];
	}
	else
	{
		*pEventPosition = EVENT_POSITION_START;
		c_szEventName = &c_szName[0];
	}

	auto it = EventTypeMap.find(c_szEventName);
	if (it == EventTypeMap.end())
	{
		SysLog("Event name {0} not found", c_szEventName);
		return false;
	}

	*pEventType = it->second;
	return true;
}

bool CPythonEventManager::CheckEventSetIndex(int32_t iIndex)  const
{
	if (iIndex < 0)
		return false;

	if ((uint32_t)iIndex >= m_EventSetVector.size())
		return false;

	return true;
}

void CPythonEventManager::Destroy()
{
	m_EventSetVector.clear();
	m_EventSetPool.Clear();
}

void CPythonEventManager::SetInterfaceWindow(pybind11::handle poInterface, pybind11::handle from)
{
	if (!from || m_poInterface.is(from))
		m_poInterface = poInterface;
}

void CPythonEventManager::SetLeftTimeString(const char* c_szString)
{
	m_strLeftTimeString = c_szString;
}

CPythonEventManager::CPythonEventManager() : m_poInterface(), m_strLeftTimeString("Remaining time: %d seconds")
{
	EventTypeMap["LETTER"] = EVENT_TYPE_LETTER;
	EventTypeMap["COLOR"] = EVENT_TYPE_COLOR;
	EventTypeMap["DELAY"] = EVENT_TYPE_DELAY;
	EventTypeMap["ENTER"] = EVENT_TYPE_ENTER;
	EventTypeMap["WAIT"] = EVENT_TYPE_WAIT;
	EventTypeMap["CLEAR"] = EVENT_TYPE_CLEAR;
	EventTypeMap["QUESTION"] = EVENT_TYPE_QUESTION;
	EventTypeMap["NEXT"] = EVENT_TYPE_NEXT;
	EventTypeMap["DONE"] = EVENT_TYPE_DONE;

	EventTypeMap["LEFTIMAGE"] = EVENT_TYPE_LEFT_IMAGE;
	EventTypeMap["TOPIMAGE"] = EVENT_TYPE_TOP_IMAGE;
	EventTypeMap["BGIMAGE"] = EVENT_TYPE_BACKGROUND_IMAGE;
	EventTypeMap["IMAGE"] = EVENT_TYPE_IMAGE;

	EventTypeMap["ADDMAPSIGNAL"] = EVENT_TYPE_ADD_MAP_SIGNAL;
	EventTypeMap["CLEARMAPSIGNAL"] = EVENT_TYPE_CLEAR_MAP_SIGNAL;

	EventTypeMap["SETMSGPOS"] = EVENT_TYPE_SET_MESSAGE_POSITION;
	EventTypeMap["ADJMSGPOS"] = EVENT_TYPE_ADJUST_MESSAGE_POSITION;
	EventTypeMap["SETCMAPPOS"] = EVENT_TYPE_SET_CENTER_MAP_POSITION;

	EventTypeMap["QUESTBUTTON"] = EVENT_TYPE_QUEST_BUTTON;

	EventTypeMap["QUESTBUTTON_CLOSE"] = EVENT_TYPE_QUEST_BUTTON_CLOSE;

	EventTypeMap["SLEEP"] = EVENT_TYPE_SLEEP;
	EventTypeMap["SET_CAMERA"] = EVENT_TYPE_SET_CAMERA;
	EventTypeMap["BLEND_CAMERA"] = EVENT_TYPE_BLEND_CAMERA;
	EventTypeMap["RESTORE_CAMERA"] = EVENT_TYPE_RESTORE_CAMERA;
	EventTypeMap["FADE_OUT"] = EVENT_TYPE_FADE_OUT;
	EventTypeMap["FADE_IN"] = EVENT_TYPE_FADE_IN;
	EventTypeMap["WHITE_OUT"] = EVENT_TYPE_WHITE_OUT;
	EventTypeMap["WHITE_IN"] = EVENT_TYPE_WHITE_IN;
	EventTypeMap["CLEAR_TEXT"] = EVENT_TYPE_CLEAR_TEXT;
	EventTypeMap["TEXT_HORIZONTAL_ALIGN_CENTER"] = EVENT_TYPE_TEXT_HORIZONTAL_ALIGN_CENTER;
	EventTypeMap["TITLE_IMAGE"] = EVENT_TYPE_TITLE_IMAGE;

	EventTypeMap["RUN_CINEMA"] = EVENT_TYPE_RUN_CINEMA;
	EventTypeMap["DUNGEON_RESULT"] = EVENT_TYPE_DUNGEON_RESULT;

	EventTypeMap["ITEM"] = EVENT_TYPE_ITEM_NAME;
	EventTypeMap["MOB"] = EVENT_TYPE_MONSTER_NAME;

	EventTypeMap["COLOR256"] = EVENT_TYPE_COLOR256;
	EventTypeMap["WINDOW_SIZE"] = EVENT_TYPE_WINDOW_SIZE;

	EventTypeMap["INPUT"] = EVENT_TYPE_INPUT;
	EventTypeMap["CONFIRM_WAIT"] = EVENT_TYPE_CONFIRM_WAIT;
	EventTypeMap["END_CONFIRM_WAIT"] = EVENT_TYPE_END_CONFIRM_WAIT;

	EventTypeMap["INSERT_IMAGE"] = EVENT_TYPE_INSERT_IMAGE;

	EventTypeMap["SELECT_ITEM"] = EVENT_TYPE_SELECT_ITEM;
}

CPythonEventManager::~CPythonEventManager()
{

}
