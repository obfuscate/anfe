set_library(engine)

load_3rdparty_library(engine fmt)
load_3rdparty_library(engine imgui)
load_3rdparty_library(engine SDL3)
load_3rdparty_library(engine ufbx)
load_3rdparty_library(engine vfspp)

# flecs
find_package(flecs CONFIG REQUIRED)
target_link_libraries(engine PUBLIC $<IF:$<TARGET_EXISTS:flecs::flecs>,flecs::flecs,flecs::flecs_static>)

# libassert
find_package(libassert CONFIG REQUIRED)
target_link_libraries(engine PUBLIC libassert::assert)

# RTTR
find_package(rttr CONFIG REQUIRED)
target_link_libraries(engine PUBLIC RTTR::Core)

# DirectX
target_link_libraries(engine PUBLIC d3d12.lib dxgi.lib d3dcompiler.lib)

# Add shaders.
add_shader_directories("${CMAKE_SOURCE_DIR}/resources/shaders")