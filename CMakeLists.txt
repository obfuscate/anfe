cmake_minimum_required(VERSION 3.28...3.30)

# Main project.
project(anfe
	VERSION 0.0.1
	DESCRIPTION "Main Launcher"
	LANGUAGES CXX)

include(scripts/cmake/helpers.cmake)

# Setup a global interface.
add_library(GlobalOptions INTERFACE)
add_library(GlobalOptionsOptimized INTERFACE)
add_library(GlobalOptionsUnoptimized INTERFACE)
target_compile_features(GlobalOptions INTERFACE cxx_std_20) # specify the C++ standard

# C++ complie options.
set(WIN_WARNINGS
	/W4			# Warning level.
	/WX			# Treat warnings as errors.

	/w44189		# Enable warning: '*' : local variable is initialized but not referenced
	/w44263		# Enable warning: 'function': member function does not override any base class virtual member function
	/w44264		# Enable warning: 'virtual_function': no override available for virtual member function from base 'class'; function is hidden
	/w44296		# Enable warning: 'operator': expression is always false
	/w44555		# Enable warning: expression has no effect; expected expression with side-effect
	/w44668		# Enable warning: 'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives'
	/w45233		# Enable warning: explicit lambda capture 'identifier' is not used
	/w45245		# Enable warning: 'function': unreferenced function with internal linkage has been removed
	/w45249		# Enable warning: 'bitfield' of type 'enumeration_name' has named enumerators with values that cannot be represented in the given bit field width of 'bitfield_width'
	/w44265		# Enable warning: 'class': class has virtual functions, but destructor is not virtual
	/w45038		# Enable warning: data member 'member1' will be initialized after data member 'member2'
	/w45204		# Enable warning: A class with virtual functions has non-virtual trivial destructor.
)

# Linker options.
set(WIN_COMPILE_LINKER_OPTIONS
	/Gy			# Enable function level linking
	/Gw			# Optimize Global Data
	/GR-		# Disable Run-Time Type Information
	/bigobj		# Increase number of sections in object files
	/Oi			# Enable intrinsics, irrelevant of optimization level
	/Zi
)

target_compile_options(GlobalOptions INTERFACE $<$<CXX_COMPILER_ID:MSVC>:${WIN_WARNINGS}>)
target_compile_options(GlobalOptions INTERFACE $<$<CXX_COMPILER_ID:MSVC>:${WIN_COMPILE_LINKER_OPTIONS}>)

set(MSVC_DEFINES
	# Use Unicode character set
	_UNICODE
	UNICODE

	# Disable MSVC specific security warnings
	_CRT_SECURE_NO_DEPRECATE
	_CRT_NONSTDC_NO_DEPRECATE

	# Never bring useless stuff from windows.h
	WIN32_LEAN_AND_MEAN
	NOMINMAX
	
	_CONSOLE
)

set(DEFINES
	LIBASSERT_USE_FMT
)

# Global preprocessor definitions
target_compile_definitions(GlobalOptions INTERFACE $<$<CXX_COMPILER_ID:MSVC>:${MSVC_DEFINES}>)
target_compile_definitions(GlobalOptions INTERFACE ${DEFINES})

target_link_options(GlobalOptions INTERFACE
	/INCREMENTAL:NO		# Disable incremental linking
	/CGTHREADS:8		# Sets number of cl.exe threads to use for optimization and code generation
	/OPT:REF,ICF=3		# Enable the optimizations that LINK performs during a build
)

target_compile_options(GlobalOptionsOptimized INTERFACE
	/O2			# Maximum speed optimization.
	/GF			#
	/Ob2		# Any suitable inline.
	/GS-		# 
	/fp:fast	# Enable fast floating point arithmetics by default
)

target_compile_definitions(GlobalOptionsOptimized INTERFACE DISABLE_ASSERTS)

target_compile_options(GlobalOptionsUnoptimized INTERFACE
	/Od			# Disable optimizations
)

# Other options
target_compile_options(GlobalOptions INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

add_executable(main_launcher src/main.cpp)

# Set output folder.
set_output(main_launcher)

set_global_compile_options(main_launcher)

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT main_launcher)

# Other libraries.
link_library(main_launcher engine "src/engine")

# Shaders project.
add_custom_target(Shaders)
#set_target_properties(Shaders PROPERTIES FOLDER "Auxiliary")
foreach(DIR ${SHADER_DIRS})
	file(GLOB_RECURSE SHADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS "${DIR}/*.hlsl" "${DIR}/*.h")
	target_sources(Shaders PRIVATE ${SHADERS})
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SHADERS})
endforeach()

# Default CLI.
set_target_properties(main_launcher PROPERTIES 
    VS_DEBUGGER_COMMAND_ARGUMENTS "--gapi dx12 -logstd --rootFolder ${CMAKE_SOURCE_DIR}")