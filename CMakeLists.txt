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

# C++ compile options.
target_compile_options(GlobalOptions INTERFACE $<$<CXX_COMPILER_ID:MSVC>:${WIN_WARNINGS}>)
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
