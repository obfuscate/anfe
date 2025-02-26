cmake_minimum_required(VERSION 3.28...3.30)

# Main project.
project(anfe
	VERSION 0.0.1
	DESCRIPTION "Main Launcher"
	LANGUAGES CXX)

include(scripts/cmake/helpers.cmake)

# Setup a global interface.
add_library(GlobalOptions INTERFACE)
target_compile_features(GlobalOptions INTERFACE cxx_std_17) # specify the C++ standard

# C++ compile options.
target_compile_options(GlobalOptions INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/W4>) # Warning level.
target_compile_options(GlobalOptions INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/WX>) # Treat warnings as errors.
target_compile_options(GlobalOptions INTERFACE $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2>) # Maximum speed.
target_compile_options(GlobalOptions INTERFACE $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/Ob2>) # Inline any suitable.
target_compile_options(GlobalOptions INTERFACE $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/fp:fast>) # Fast math.

add_executable(main_launcher src/main.cpp)

# Set output folder.
set_output(main_launcher)

# Inherit all global properties.
target_link_libraries(main_launcher PRIVATE GlobalOptions)

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT main_launcher)

# Other libraries.
link_library(main_launcher engine "src/engine")
