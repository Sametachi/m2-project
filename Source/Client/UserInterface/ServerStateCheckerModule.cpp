#include "StdAfx.h"
#include "ServerStateChecker.h"

static void serverstatecheckerCreate(pybind11::handle poWnd)
{
	CServerStateChecker::GetInstance()->Create(poWnd);
}

static void serverstatecheckerUpdate()
{
	CServerStateChecker::GetInstance()->Update();
}

static void serverstatecheckerRequest()
{
	CServerStateChecker::GetInstance()->Request();
}

static void serverstatecheckerAddChannel(uint32_t nServerIndex, std::string szAddr, uint32_t nPort)
{
	CServerStateChecker::GetInstance()->AddChannel(nServerIndex, szAddr.c_str(), nPort);
}

static void serverstatecheckerInitialize()
{
	CServerStateChecker::GetInstance()->Initialize();
}

PYBIND11_EMBEDDED_MODULE(ServerStateChecker, m)
{
	m.def("Create",	serverstatecheckerCreate);
	m.def("Update",	serverstatecheckerUpdate);
	m.def("Request",	serverstatecheckerRequest);
	m.def("AddChannel",	serverstatecheckerAddChannel);
	m.def("Initialize",	serverstatecheckerInitialize);
}
