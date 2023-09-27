#include "StdAfx.h"

static void profilerPush(std::string szName)
{


}

static void profilerPop(std::string szName)
{


}



PYBIND11_EMBEDDED_MODULE(profiler, m)
{
	m.def("Push",	profilerPush);
	m.def("Pop",	profilerPop);

}
