include_guard()

include(GenerateExportHeader)

function(link_library target library_name library_path)
	add_subdirectory(${library_path})
	target_link_libraries(${target} PRIVATE ${library_name})
endfunction()

# Allows to put all artifacts in one directory.
function(set_output target)
	set_target_properties(${target}
		PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/$<CONFIG>/lib
			COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/$<CONFIG>/bin
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/$<CONFIG>/lib
			PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/$<CONFIG>/bin
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/$<CONFIG>/bin
	)
endfunction()

function(set_library target)
	file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
	file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

	add_library(${target} SHARED)
	target_sources(${target} PUBLIC ${headers} PRIVATE ${sources})

	target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
	target_link_libraries(${target} PRIVATE GlobalOptions)

	string(TOUPPER ${target} target_upper)
	get_target_property(binary_dir ${target} BINARY_DIR)
	generate_export_header(${target}
		EXPORT_FILE_NAME ${binary_dir}/generated/cxx/${target}/export.h
		EXPORT_MACRO_NAME ${target_upper}_API
	)

	target_sources(${target} PUBLIC ${binary_dir}/generated/cxx/${target}/export.h)
	target_include_directories(${target} PUBLIC ${binary_dir}/generated/cxx)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Sources" FILES ${sources} ${headers})
	source_group(TREE ${binary_dir}/generated/cxx PREFIX "Sources" FILES ${binary_dir}/generated/cxx/${target}/export.h)

	set_output(${target})
endfunction()
