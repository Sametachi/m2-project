#include "StdAfx.h"
#include "PythonEventManager.h"
#include "PythonNetworkStream.h"

static int32_t eventRegisterEventSet(std::string szFileName)
{
	return CPythonEventManager::GetInstance()->RegisterEventSet(szFileName.c_str());
}

static int32_t eventRegisterEventSetFromString(std::string szEventString)
{
	return CPythonEventManager::GetInstance()->RegisterEventSetFromString(szEventString);
}

static void eventClearEventSet(int32_t iIndex)
{
	CPythonEventManager::GetInstance()->ClearEventSeti(iIndex);

}

static void eventSetRestrictedCount(int32_t iIndex, int32_t iCount)
{
	CPythonEventManager::GetInstance()->SetRestrictedCount(iIndex, iCount);

}

static int32_t eventGetEventSetLocalYPosition(int32_t iIndex)
{
	return CPythonEventManager::GetInstance()->GetEventSetLocalYPosition(iIndex);

}

static void eventAddEventSetLocalYPosition(int32_t iIndex, int32_t iPos)
{
	CPythonEventManager::GetInstance()->AddEventSetLocalYPosition(iIndex, iPos);

}

static void eventInsertText(int32_t iIndex, std::string szText)
{
	CPythonEventManager::GetInstance()->InsertText(iIndex, szText.c_str());

}

static void eventInsertTextInline(int32_t iIndex, std::string szText, int32_t iXIndex)
{
	CPythonEventManager::GetInstance()->InsertText(iIndex, szText.c_str(), iXIndex);

}

static void eventUpdateEventSet(int32_t iIndex, int32_t ix, int32_t iy)
{
	CPythonEventManager::GetInstance()->UpdateEventSet(iIndex, ix, -iy);

}

static void eventRenderEventSet(int32_t iIndex)
{
	CPythonEventManager::GetInstance()->RenderEventSet(iIndex);

}

static void eventSetEventSetWidth(int32_t iIndex, int32_t iWidth)
{
	CPythonEventManager::GetInstance()->SetEventSetWidth(iIndex, iWidth);
}

static void eventSkip(int32_t iIndex)
{
	CPythonEventManager::GetInstance()->Skip(iIndex);

}

static bool eventIsWait(int32_t iIndex)
{
	return  CPythonEventManager::GetInstance()->IsWait(iIndex);

}

static void eventEndEventProcess(int32_t iIndex)
{
	CPythonEventManager::GetInstance()->EndEventProcess(iIndex);

}

static void eventSelectAnswer(int32_t iIndex, int32_t iAnswer)
{
	CPythonEventManager::GetInstance()->SelectAnswer(iIndex, iAnswer);

}

static int32_t eventGetLineCount(int32_t iIndex)
{
	return CPythonEventManager::GetInstance()->GetLineCount(iIndex);

}

static void eventSetVisibleStartLine(int32_t iIndex, int32_t iStartLine)
{
	CPythonEventManager::GetInstance()->SetVisibleStartLine(iIndex, iStartLine);

}

static int32_t eventGetVisibleStartLine(int32_t iIndex)
{
	return CPythonEventManager::GetInstance()->GetVisibleStartLine(iIndex);

}

static void eventSetEventHandler(int32_t iIndex, pybind11::handle pyHandle)
{
	CPythonEventManager::GetInstance()->SetEventHandler(iIndex, pyHandle);

}

static void eventSetInterfaceWindow(pybind11::handle pyHandle)
{
	auto rpem = CPythonEventManager::GetInstance();
	rpem->SetInterfaceWindow(pyHandle);
}

static void eventSetLeftTimeString(std::string szText)
{
	auto rpem = CPythonEventManager::GetInstance();
	rpem->SetLeftTimeString(szText.c_str());

}

static void eventQuestButtonClick(int32_t iIndex)
{
	CPythonNetworkStream::GetInstance()->SendScriptButtonPacket(iIndex);
}

static void eventDestroy()
{
	auto rpem = CPythonEventManager::GetInstance();
	rpem->Destroy();

}



PYBIND11_EMBEDDED_MODULE(event, m)
{
	m.def("RegisterEventSet",	eventRegisterEventSet);
	m.def("RegisterEventSetFromString",	eventRegisterEventSetFromString);
	m.def("ClearEventSet",	eventClearEventSet);
	m.def("SetRestrictedCount",	eventSetRestrictedCount);
	m.def("GetEventSetLocalYPosition",	eventGetEventSetLocalYPosition);
	m.def("AddEventSetLocalYPosition",	eventAddEventSetLocalYPosition);
	m.def("InsertText",	eventInsertText);
	m.def("InsertTextInline",	eventInsertTextInline);
	m.def("UpdateEventSet",	eventUpdateEventSet);
	m.def("RenderEventSet",	eventRenderEventSet);
	m.def("SetEventSetWidth",	eventSetEventSetWidth);
	m.def("Skip",	eventSkip);
	m.def("IsWait",	eventIsWait);
	m.def("EndEventProcess",	eventEndEventProcess);
	m.def("SelectAnswer",	eventSelectAnswer);
	m.def("GetLineCount",	eventGetLineCount);
	m.def("SetVisibleStartLine",	eventSetVisibleStartLine);
	m.def("GetVisibleStartLine",	eventGetVisibleStartLine);
	m.def("SetEventHandler",	eventSetEventHandler);
	m.def("SetInterfaceWindow",	eventSetInterfaceWindow);
	m.def("SetLeftTimeString",	eventSetLeftTimeString);
	m.def("QuestButtonClick",	eventQuestButtonClick);
	m.def("Destroy",	eventDestroy);

	m.attr("BOX_VISIBLE_LINE_COUNT") = int32_t(CPythonEventManager::BOX_VISIBLE_LINE_COUNT);
	m.attr("BUTTON_TYPE_NEXT") = int32_t(CPythonEventManager::BUTTON_TYPE_NEXT);
	m.attr("BUTTON_TYPE_DONE") = int32_t(CPythonEventManager::BUTTON_TYPE_DONE);
	m.attr("BUTTON_TYPE_CANCEL") = int32_t(CPythonEventManager::BUTTON_TYPE_CANCEL);
}
