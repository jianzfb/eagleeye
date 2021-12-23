# - Try to find NumPy
# Once done, this will define
#
#  NUMPY_FOUND - system has numpy
#  NUMPY_INCLUDE_DIRS - the numpy include directories

find_package(PythonInterp 3.6)
find_package(PythonLibs 3.6)

set(_NUMPY_SEARCH_DIRECTORIES)

if(PYTHONLIBS_FOUND)
	list(APPEND _NUMPY_SEARCH_DIRECTORIES ${PYTHON_INCLUDE_DIRS})
endif()

if(PYTHONINTERP_FOUND)
	execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c "import numpy; print(numpy.get_include(), end='')"
		OUTPUT_VARIABLE _PYTHON_NUMPY_PATH
		RESULT_VARIABLE _ERROR_FINDING_NUMPY)
	if(${_ERROR_FINDING_NUMPY} EQUAL 0)
		list(APPEND _NUMPY_SEARCH_DIRECTORIES ${_PYTHON_NUMPY_PATH})
	endif()
endif()

find_path(NUMPY_INCLUDE_DIR
        NAMES numpy/ndarrayobject.h
        PATHS ${_NUMPY_SEARCH_DIRECTORIES}
)


include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(NumPy DEFAULT_MSG NUMPY_INCLUDE_DIR)

if(NUMPY_FOUND)
	set(NUMPY_INCLUDE_DIRS ${NUMPY_INCLUDE_DIR})
endif()

mark_as_advanced(NUMPY_INCLUDE_DIR NUMPY_INCLUDE_DIRS)

