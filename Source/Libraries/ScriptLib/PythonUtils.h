#pragma once

#include <pybind11/pybind11.h>
namespace py = pybind11;

// PyBin11 implementation.
template <typename... Args> bool PyCallClassMemberFunc(py::handle o, const std::string& attr, Args &&... arg)
{
    if (o && !py::isinstance<py::none>(o) && py::hasattr(o, attr.c_str()))
    {
        auto func = o.attr(attr.c_str());
        if (func.is_none())
            return false;
        try
        {
            auto retObj = func(std::forward<Args>(arg)...);
            if (py::isinstance<py::bool_>(retObj))
                return py::cast<bool>(retObj);

            return true;
        }
        catch (py::error_already_set& e)
        {
            SysLog("{}", e.what());
            return false;
        }
    }

    return false;
}
