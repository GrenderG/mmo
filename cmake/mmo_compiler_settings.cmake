set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

if (MSVC)
	include("mmo_compilers/msvc")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	include("mmo_compilers/clang")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	include("mmo_compilers/gcc")
else()
	message(FATAL_ERROR "Unsupported compiler! Currently, this project supports Microsoft Visual Studio 2017 or newer, GCC 8 or newer and Clang.")
endif()

