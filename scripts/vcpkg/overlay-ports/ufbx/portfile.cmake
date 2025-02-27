vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO ufbx/ufbx
	REF v0.17.1
	SHA512 c96d5bd947d66aaa2d88ae6ff69ba37cf9973e6f83e967eab0ab5442f2b941738755b2efc43ce7e20d5eb1ac8de9304612f58bb95e2ec8df76cbbe920340b1f0
	HEAD_REF master
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

vcpkg_cmake_configure(
	SOURCE_PATH "${SOURCE_PATH}"
	WINDOWS_USE_MSBUILD
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup()

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
