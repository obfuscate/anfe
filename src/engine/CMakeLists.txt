set_library(engine)

load_3rdparty_library(engine fmt)
load_3rdparty_library(engine imgui)
load_3rdparty_library(engine SDL3)
load_3rdparty_library(engine ufbx)

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
#find_package(directx-headers CONFIG REQUIRED)
#find_package(directx12-agility CONFIG REQUIRED)
#target_link_libraries(engine PUBLIC Microsoft::DirectX-Headers)
#target_link_libraries(engine PUBLIC Microsoft::DirectX-Headers Microsoft::DirectX-Guids Microsoft::DirectX12-Agility)

#if(TARGET Microsoft::DirectX12-Agility)
#	file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/D3D12")
#	add_custom_command(TARGET engine POST_BUILD
#		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE> $<TARGET_FILE_DIR:engine>/D3D12
#		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> $<TARGET_FILE_DIR:engine>/D3D12
#	   COMMAND_EXPAND_LISTS
#	)
#endif()
target_link_libraries(engine PUBLIC d3d12.lib dxgi.lib d3dcompiler.lib)
