set_library(engine)

find_package(ufbx CONFIG REQUIRED)
target_link_libraries(engine PRIVATE ufbx::ufbx)
