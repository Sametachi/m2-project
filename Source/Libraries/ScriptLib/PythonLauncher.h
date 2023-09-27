#pragma once
#include <Basic/Singleton.h>
#include <string>

class CPythonLauncher : public Singleton<CPythonLauncher>
{
public:
	CPythonLauncher(std::wstring programName, bool bInterpreter = false);
	virtual ~CPythonLauncher();

	void Clear(bool pyShutdown = true);
	bool Run();

	static void InitializeLogging();
};
