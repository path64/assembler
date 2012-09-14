PROJECT(yasm)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.4)
if (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif (COMMAND cmake_policy)

# Where to look first for cmake modules
set(YASM_MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
set(CMAKE_MODULE_PATH "${YASM_MODULE_DIR}")

set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/lib")
if (WIN32)
   # we don't want to be forced to set two paths into the build tree 
   set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
endif (WIN32)

OPTION(BUILD_STATIC "Build a monolithic executable with no plugin support" OFF)

if (WIN32)
    OPTION(BUILD_TESTS "Enable unit tests" OFF)
else (WIN32)
    OPTION(BUILD_TESTS "Enable unit tests" ON)
endif (WIN32)

INCLUDE(InstallOptions.cmake)
INCLUDE(YasmMacros)

OPTION(ENABLE_NLS "Enable message translations" OFF)
OPTION(WITH_XML "Enable XML debug dumps" OFF)
if (WITH_XML)
    ADD_DEFINITIONS(-DWITH_XML)
endif (WITH_XML)

OPTION(BUILD_TEST_COVERAGE "Enable test coverage if possible" ON)

OPTION(INSTALL_GPUASM "Install gpuasm in make install target" ON)

# Default build type to debug if not set
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set (PACKAGE_NAME "pathas")
set (PACKAGE_BUILD "${PSC_FULL_VERSION}")
IF(NOT PACKAGE_BUILD)
	set (PACKAGE_BUILD "FIXME")
ENDIF(NOT PACKAGE_BUILD)
set (PACKAGE_VERSION ${PACKAGE_BUILD})
set (PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

INCLUDE_DIRECTORIES(BEFORE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${yasm_SOURCE_DIR}
    ${yasm_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    )

INCLUDE(CTest)
#INCLUDE(Dart)

set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Yasm is a modular assembler")
set (CPACK_PACKAGE_NAME "yasm")
set (CPACK_PACKAGE_VENDOR "Tortall Networks")
set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/pkg-descr")
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/Copying/COPYING")
set (CPACK_PACKAGE_VERSION_MAJOR "${YASM_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${YASM_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${YASM_VERSION_SUBMINOR}")
set (CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set (CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set (CPACK_SOURCE_IGNORE_FILES
     "~$"
     "\\\\.gitignore"
     "/\\\\.git/"
     "\\\\.o$"
     "\\\\.swp$"
     "/objdir.*"
     "/doc.*"
     )
#message ("CPACK_SOURCE_IGNORE_FILES = ${CPACK_SOURCE_IGNORE_FILES}")
INCLUDE(CPack)

INCLUDE(ConfigureChecks.cmake)

# Generate diagnostic includes
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib/yasmx/Basic)
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/yasmx/Basic)
ADD_CUSTOM_COMMAND(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/lib/yasmx/Basic/DiagnosticGroups.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/include/yasmx/Basic/DiagnosticCategories.h
        ${CMAKE_CURRENT_BINARY_DIR}/lib/yasmx/Basic/CategoryNameTable.inc
        ${CMAKE_CURRENT_BINARY_DIR}/include/yasmx/Basic/DiagnosticKinds.h
        ${CMAKE_CURRENT_BINARY_DIR}/lib/yasmx/Basic/StaticDiagInfo.inc
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/include/yasmx/Basic/gen_diag.py
        ${CMAKE_CURRENT_BINARY_DIR}/lib/yasmx/Basic/DiagnosticGroups.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/include/yasmx/Basic/DiagnosticCategories.h
        ${CMAKE_CURRENT_BINARY_DIR}/lib/yasmx/Basic/CategoryNameTable.inc
        ${CMAKE_CURRENT_BINARY_DIR}/include/yasmx/Basic/DiagnosticKinds.h
        ${CMAKE_CURRENT_BINARY_DIR}/lib/yasmx/Basic/StaticDiagInfo.inc
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/include/yasmx/Basic/gen_diag.py
    )
ADD_CUSTOM_TARGET(DiagnosticIncludes
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lib/yasmx/Basic/DiagnosticGroups.cpp
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/include/yasmx/Basic/DiagnosticCategories.h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lib/yasmx/Basic/CategoryNameTable.inc
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/include/yasmx/Basic/DiagnosticKinds.h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lib/yasmx/Basic/StaticDiagInfo.inc)

ADD_SUBDIRECTORY(tools)
ADD_SUBDIRECTORY(modules)
ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(frontends)
IF(BUILD_TESTS)
    ADD_SUBDIRECTORY(unittests)
ENDIF(BUILD_TESTS)
ADD_SUBDIRECTORY(regression)
