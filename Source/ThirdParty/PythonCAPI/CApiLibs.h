#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <Python.h>

extern void CApiLibs_Init(void);
extern int CApiLibs_TclInit(void);

#ifdef __cplusplus
}
#endif
