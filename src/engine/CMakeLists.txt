set_library(engine)

load_3rdparty_library(engine fmt)
load_3rdparty_library(engine ufbx)
load_3rdparty_library(engine imgui)
load_3rdparty_library(engine EnTT)

# LLGL
load_3rdparty_library(engine LLGL)
add_custom_command(TARGET engine POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:LLGL::LLGL_Direct3D12> $<TARGET_FILE_DIR:engine>
	COMMENT "Copying LLGL::LLGL_Direct3D12 backend DLL to the output directory"
)
add_custom_command(TARGET engine POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:LLGL::LLGL_Vulkan> $<TARGET_FILE_DIR:engine>
	COMMENT "Copying LLGL::LLGL_Vulkan backend DLL to the output directory"
)

# libassert
find_package(libassert CONFIG REQUIRED)
target_link_libraries(engine PUBLIC libassert::assert)

# RTTR
find_package(rttr CONFIG REQUIRED)
target_link_libraries(engine PUBLIC RTTR::Core)