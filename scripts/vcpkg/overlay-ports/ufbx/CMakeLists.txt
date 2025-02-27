cmake_minimum_required(VERSION 3.19)

project(ufbx LANGUAGES C CXX)

add_library(ufbx STATIC)

target_sources(ufbx
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ufbx.h>
		$<INSTALL_INTERFACE:include/ufbx/ufbx.h>
	PRIVATE
		ufbx.c
)

target_compile_features(ufbx PRIVATE cxx_std_17) # specify the C++ standard

target_include_directories(ufbx
	PUBLIC
		$<INSTALL_INTERFACE:include>
)

target_compile_options(ufbx
	PRIVATE
		$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2> # Maximum speed.
		$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/Ob2> # Inline any suitable.
		$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/fp:fast> # Fast math.
)

install(FILES ufbx.h DESTINATION "include/ufbx")

install(
	TARGETS ufbx
	EXPORT ufbxTargets
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)

install(
	EXPORT ufbxTargets
	FILE ufbxConfig.cmake
	NAMESPACE ufbx::
	DESTINATION share/ufbx
)
