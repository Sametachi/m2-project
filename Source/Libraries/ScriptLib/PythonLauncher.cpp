#include "StdAfx.h"
#include "PythonLauncher.h"
#include <boost/algorithm/string.hpp>
#include <VFE/Include/VFE.hpp>
#include <EterBase/Utils.h>

#define BINARY_MODULE(name) extern "C" PyObject* pybind11_init_impl_##name();
#include "ModuleList.inl"
#undef BINARY_MODULE

namespace
{
	static _inittab kModules[] = 
	{
		#define BINARY_MODULE(name) { #name, pybind11_init_impl_##name },
		#include "ModuleList.inl"
		#undef BINARY_MODULE
		{nullptr, nullptr}
	};
}

CPythonLauncher::CPythonLauncher(std::wstring programName, bool bInterpreter)
{
	if (!Py_IsInitialized())
	{
		PyStatus status;
		PyPreConfig preconfig;
		PyPreConfig_InitIsolatedConfig(&preconfig);

#ifdef _DEBUG
		if (bInterpreter)
		{
			preconfig.isolated = 0;
			preconfig.use_environment = 1;
		}
		else
#endif
		{
			preconfig.isolated = 1;
			preconfig.use_environment = 0;
		}

		preconfig.utf8_mode = 1;

		status = Py_PreInitialize(&preconfig);
		if (PyStatus_Exception(status))
		{
			Py_ExitStatusException(status);
		}

		if (PyImport_ExtendInittab(kModules) == -1)
			SysLog("FAILED TO EXTEND INIT TAB");

		PyConfig config;
		PyConfig_InitIsolatedConfig(&config);

		/* Set the program name. Implicitly preinitialize Python. */
		status = PyConfig_SetString(&config, &config.program_name, programName.c_str());
		if (PyStatus_Exception(status))
		{
			goto fail;
		}

		/* Set the filesystem encoding to utf-8 */
		status = PyConfig_SetString(&config, &config.filesystem_encoding, L"utf-8");
		if (PyStatus_Exception(status))
		{
			goto fail;
		}

/*
#ifdef _DEBUG
		config.verbose = 2;
#else
		config.verbose = 0;
#endif
*/
		config.verbose = 0;
		config.user_site_directory = 1;
		config.site_import = 0;

#ifdef _DEBUG
		if (bInterpreter)
		{
			config.write_bytecode = 1;
			config.use_environment = 1;
			config._init_main = 1;
			config.isolated = 0;
		}
		else
#endif
		{
			config._init_main = 0;
			config.use_environment = 0;
			config.isolated = 1;
			config.write_bytecode = 0;
		}

		status = Py_InitializeFromConfig(&config);
		if (PyStatus_Exception(status))
		{
			goto fail;
		}

#if defined(_DEBUG) && 0
		// todo: wchar_t* argv
		PySys_SetArgv(__argc, __wargv);
#else
		PySys_SetArgv(0, nullptr);
#endif

		InitializeLogging();

		status = _Py_InitializeMain();
		if (PyStatus_Exception(status))
		{
			goto fail;
		}

		goto cont;
	fail:
		PyConfig_Clear(&config);
		Py_ExitStatusException(status);
		return;
	cont:
		PyConfig_Clear(&config);
	}

	auto poModule = pybind11::module_::import("__main__");
	if (!poModule)
		return;

	auto builtins = pybind11::module_::import("builtins");

	builtins.attr("TRUE") = 1;
	builtins.attr("FALSE") = 0;
	builtins.attr("F") = 0;

#ifdef _DEBUG 
	builtins.attr("__DEBUG__") = 1;
	builtins.attr("__INTERPRETER__") = bInterpreter ? 1 : 0;
#else
	builtins.attr("__DEBUG__") = 0;
	builtins.attr("__INTERPRETER__") = 0;
#endif

	InitializeLogging();

#ifdef _DEBUG 
	if (bInterpreter)
	{
		std::string fn;
		GetExcutedFileName(fn);
		std::replace(fn.begin(), fn.end(), '\\', '/');

		fn = "sys.dllhandle = \"" + fn + "\"\n";

		// Required for fixing stuff with pyd

		try
		{
			pybind11::exec("import sys\n");
			pybind11::exec(fn.c_str());
		}
		catch (std::exception& ex)
		{
			SysLog("{0}", ex.what());
		}
	}
#endif
}

PYBIND11_EMBEDDED_MODULE(__logerr, m)
{
	m.def("write", [](std::string msg) {
		boost::trim(msg);
		if (msg.length() < 1)
			return;

		SysLog(msg.c_str());
	});
	m.def("flush", []() {});
}

PYBIND11_EMBEDDED_MODULE(__logout, m)
{
	m.def("write", [](std::string msg) {
		boost::trim(msg);
		if (msg.length() < 1)
			return;

		PyLog(msg.c_str());
	});
	m.def("flush", []() {});
}

extern "C" PyObject * pybind11_init_impl___logerr();
extern "C" PyObject * pybind11_init_impl___logout();

void CPythonLauncher::InitializeLogging()
{
	PySys_SetObject("stderr", pybind11_init_impl___logerr());
	PySys_SetObject("stdout", pybind11_init_impl___logout());
}

CPythonLauncher::~CPythonLauncher()
{
	Clear();
}

void CPythonLauncher::Clear(bool pyShutdown)
{
	if (Py_IsInitialized() && pyShutdown)
	{
		pybind11::finalize_interpreter();
	}
}

bool CPythonLauncher::Run()
{
	try 
	{
		py::module_ m_mainModule = py::module_::import("speck");
	}
	catch (py::error_already_set& e) 
	{
		FatalLog(e.what());
		return false;
	}

	return true;
}
