set(SSL_INC _ssl.c _ssl.h _ssl_data.h _ssl_data_111.h _ssl_data_300.h)
set(SSL_DIR _ssl/cert.c _ssl/debughelpers.c _ssl/misc.c)
set(SSL_DIR_CLINC _ssl/clinic/cert.c.h)

set(CLINC clinic/arraymodule.c.h
clinic/audioop.c.h
clinic/binascii.c.h
clinic/cmathmodule.c.h
clinic/fcntlmodule.c.h
clinic/gcmodule.c.h
clinic/grpmodule.c.h
clinic/itertoolsmodule.c.h
clinic/mathmodule.c.h
clinic/md5module.c.h
clinic/overlapped.c.h
clinic/posixmodule.c.h
clinic/pwdmodule.c.h
clinic/pyexpat.c.h
clinic/readline.c.h
clinic/resource.c.h
clinic/selectmodule.c.h
clinic/sha1module.c.h
clinic/sha256module.c.h
clinic/sha512module.c.h
clinic/signalmodule.c.h
clinic/spwdmodule.c.h
clinic/symtablemodule.c.h
clinic/termios.c.h
clinic/unicodedata.c.h
clinic/zlibmodule.c.h
clinic/_abc.c.h
clinic/_asynciomodule.c.h
clinic/_bisectmodule.c.h
clinic/_bz2module.c.h
clinic/_codecsmodule.c.h
clinic/_collectionsmodule.c.h
clinic/_contextvarsmodule.c.h
clinic/_cryptmodule.c.h
clinic/_cursesmodule.c.h
clinic/_curses_panel.c.h
clinic/_datetimemodule.c.h
clinic/_dbmmodule.c.h
clinic/_elementtree.c.h
clinic/_gdbmmodule.c.h
clinic/_hashopenssl.c.h
clinic/_heapqmodule.c.h
clinic/_localemodule.c.h
clinic/_lsprof.c.h
clinic/_lzmamodule.c.h
clinic/_opcode.c.h
clinic/_operator.c.h
clinic/_pickle.c.h
clinic/_queuemodule.c.h
clinic/_randommodule.c.h
clinic/_sre.c.h
clinic/_ssl.c.h
clinic/_statisticsmodule.c.h
clinic/_struct.c.h
clinic/_testmultiphase.c.h
clinic/_tkinter.c.h
clinic/_tracemalloc.c.h
clinic/_weakref.c.h
clinic/_winapi.c.h)
set(TK tkinter.h _tkinter.c)
set(EXPAT pyexpat.c)
set(SM socketmodule.c socketmodule.h)
set(SELM selectmodule.c)
set(MAIN_FILES CApiLibs.h CApiLibs.c)
set(CTYPES _ctypes/callbacks.c
_ctypes/callproc.c
_ctypes/cfield.c
_ctypes/ctypes.h
_ctypes/ctypes_dlfcn.h
_ctypes/darwin
_ctypes/malloc_closure.c
_ctypes/stgdict.c
_ctypes/_ctypes.c)
set(PYCORE pycore/pycore_abstract.h
pycore/pycore_accu.h
pycore/pycore_asdl.h
pycore/pycore_ast.h
pycore/pycore_ast_state.h
pycore/pycore_atomic.h
pycore/pycore_atomic_funcs.h
pycore/pycore_bitutils.h
pycore/pycore_blocks_output_buffer.h
pycore/pycore_bytes_methods.h
pycore/pycore_call.h
pycore/pycore_ceval.h
pycore/pycore_code.h
pycore/pycore_compile.h
pycore/pycore_condvar.h
pycore/pycore_context.h
pycore/pycore_dtoa.h
pycore/pycore_fileutils.h
pycore/pycore_format.h
pycore/pycore_gc.h
pycore/pycore_getopt.h
pycore/pycore_gil.h
pycore/pycore_hamt.h
pycore/pycore_hashtable.h
pycore/pycore_import.h
pycore/pycore_initconfig.h
pycore/pycore_interp.h
pycore/pycore_list.h
pycore/pycore_long.h
pycore/pycore_moduleobject.h
pycore/pycore_object.h
pycore/pycore_parser.h
pycore/pycore_pathconfig.h
pycore/pycore_pyarena.h
pycore/pycore_pyerrors.h
pycore/pycore_pyhash.h
pycore/pycore_pylifecycle.h
pycore/pycore_pymem.h
pycore/pycore_pystate.h
pycore/pycore_runtime.h
pycore/pycore_structseq.h
pycore/pycore_symtable.h
pycore/pycore_sysmodule.h
pycore/pycore_traceback.h
pycore/pycore_tuple.h
pycore/pycore_ucnhash.h
pycore/pycore_unionobject.h
pycore/pycore_warnings.h)

source_group(SSL FILES ${SSL_INC})
source_group(SSL/_ssl FILES ${SSL_DIR})
source_group(SSL/clinic FILES ${SSL_DIR_CLINC})
source_group(clinic FILES ${CLINC})
source_group(Tkinter FILES ${TK})
source_group(Expat FILES ${EXPAT})
source_group(SelectModule FILES ${SELM})
source_group(SocketModule FILES ${SM})
source_group(Main FILES ${MAIN_FILES})
source_group(CTypes FILES ${CTYPES})
source_group(PythonCore FILES ${PYCORE})

#set(CTYPES_DARWIN _ctypes/darwin/dlfcn.h _ctypes/darwin/dlfcn_simple.c)
#source_group(CTypes/MacOSX FILES ${CTYPES_DARWIN})

add_library(CAPILibs STATIC ${PYCORE} ${CTYPES} ${MAIN_FILES} ${CLINC} ${SSL_INC} ${EXPAT} ${SSL_DIR_CLINC} ${SELM} ${SM}) # ${TK} 

add_dep(NAME libffi PROJECT CAPILibs EXPORT PUBLIC)
add_dep(NAME OpenSSL PROJECT CAPILibs EXPORT PUBLIC)
add_dep(NAME pybind11 PROJECT CAPILibs EXPORT PUBLIC)
add_dep(NAME expat PROJECT CAPILibs EXPORT PUBLIC)
add_dep(NAME Python3 PROJECT CAPILibs EXPORT PUBLIC)
target_include_directories(CAPILibs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pycore ../tcl/generic ../tk/generic ../tk/xlib)
target_compile_definitions(CAPILibs PRIVATE Py_BUILD_CORE=1 USE_TCLOO_STUBS=1 USE_TK_STUBS=1 USE_TCL_STUBS=1 STATIC_BUILD=1 XML_STATIC=1)
set_target_properties(CAPILibs PROPERTIES FOLDER ThirdParty)
