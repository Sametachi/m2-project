#pragma once

#include <Basic/Logging.hpp>
#include <Basic/PragmaWarnings.hpp>

#include "../eterLib/StdAfx.h"
#include "../eterGrnLib/StdAfx.h"


// PyBind11 include
#include <pybind11/pybind11.h>
#include <pybind11/eval.h>
#include <pybind11/embed.h>
namespace py = pybind11;

#include "PythonUtils.h"
#include "PythonLauncher.h"
#include "PythonResourceManager.h"
