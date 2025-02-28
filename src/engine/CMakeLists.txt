set_library(engine)

load_3rdparty_library(engine fmt)
load_3rdparty_library(engine ufbx)
load_3rdparty_library(engine SDL3)

find_package(rttr CONFIG REQUIRED)
target_link_libraries(engine PUBLIC RTTR::Core)
