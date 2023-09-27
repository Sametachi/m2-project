#include "CApiLibs.h"
#include "socketmodule.h"

/*
#include <tcl.h>
#include <tclOO.h>
#include <tclTomMath.h>
*/
#undef Tcl_CreateInterp

void CApiLibs_Init(void)
{
#ifdef _DEBUG
	PyMODINIT_FUNC PyInit__socket(void);
	PyImport_AppendInittab(PySocket_MODULE_NAME, PyInit__socket);
	PyMODINIT_FUNC PyInit_select(void);
	PyImport_AppendInittab("select", PyInit_select);
	PyMODINIT_FUNC PyInit__ssl(void);
	PyImport_AppendInittab("_ssl", PyInit__ssl);
	/*
	PyMODINIT_FUNC PyInit__tkinter(void);
	PyImport_AppendInittab("_tkinter", PyInit__tkinter);
	*/
#endif
	PyMODINIT_FUNC PyInit__ctypes(void);
	PyImport_AppendInittab("_ctypes", PyInit__ctypes);
	PyMODINIT_FUNC PyInit_pyexpat(void);
	PyImport_AppendInittab("pyexpat", PyInit_pyexpat);
}

int CApiLibs_TclInit(void)
{
	/*
#ifdef _DEBUG
	Tcl_Interp* interp = Tcl_CreateInterp();

	if (Tcl_InitStubs(interp, "8.6.11", 1) == NULL)
		return 0;

	if (Tcl_TomMath_InitStubs(interp, "8.6.11") == NULL)
		return 0;

	if (Tcl_OOInitStubs(interp) == NULL)
		return 0;
#endif
	*/

	return 1;
}