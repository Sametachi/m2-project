# CMAKE INTERNAL DOCUMENTATION

# Variables
| Name | Values | Description |
| -----| -------| ------------ |
| BUILD_STATIC | 1/0 | Set if the platform uses static libraries |
| BUILD_NO_CLIENT | 1/0 | Set if the platform cannot build the client |
| BUILD_NO_SERVER | 1/0 | Set if the platform cannot build the server |
| CFG_NO_PKGCONFIG | 1/0 | Set if the platform does not support pkg-config |
| CFG_NO_DIRECTX | 1/0 | Set if the platform does not support DirectX |
| CFG_VCPKG | 1/0 | Set if the toolchain contains vcpkg |
| CFG_NO_PTHREADS | 1/0 | Set If the platform does not supports pthread |
| LINK_EXTRA | list... | Extra libraries that has to be linked to any executable |
| SERVER_BUILD_VARIANT | debug/release/minsizerel/relwithdebinfo | Type of build of the server to install |
| SERVER_CONFIG_FILEPATH | ... | Path of the config |
| CMAKE_INSTALL_PREFIX | ... | Path of where to install the server |

## Options
- FORCE_INSTALL

# Functions

## find_dep(NAME dep_name [COMPONENTS components] [TARGET cmake_link_target_name] [DEBUG_POSTFRIX debug_postfix_name])
This function finds a dependecy

1. Attempt to find the dependency using normal cmake find_package (adjusted for vcpkg as well)
2. Attempt to find the dependency using pkg-config (on supporeted hardware)
3. Try to find the depencency inside the library path (adjusted for vcpkg as well)

If all of this three methods fails, the dependency is marked as not found

The DEBUGPOST is required for libraries that have _d/d at the end of their debug library, this behavour is currently
implemented in vcpkg manual library linking (but it can be extended to any platform in case)

Examples:
```
find_dep(NAME Boost COMPONENTS System TARGET Boost::boost)
find_dep(NAME DevIL)
find_dep(NAME fmt TARGET fmt::fmt)
```

## add_dep(NAME dep_name PROJECT project_name EXPORT private/public/interface)
Adds a dependency to the project

Examples:
```
add_dep(NAME DevIL PROJECT UserInterface EXPORT PRIVATE)
add_dep(NAME JPEG PROJECT EterImageLib EXPORT PUBLIC)
```

## do_pch(PROJECT project_name [NAME pch_name])
Adds grouping and precompiled headers to a project

If NAME is not specified, the default precompiled header name will be defaulted to Stdafx.h

Examples:
```
do_pch(PROJECT UserInterface)
do_pch(PROJECT CorsacBase NAME pch.h)
```

## GIT_WC_INFO(directory prefix)
Executes git branch information and stores them inside a prefix

## auto_dep(project_name export)
Macro that reads a defined "LINK_DEPS" macro to automatically link all the dependencies

## auto_dep_exe(project)
Performs "auto_dep" with private export and link additional libraries required for executables
