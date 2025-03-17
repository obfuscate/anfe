vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO obfuscate/vfspp
	REF 044a2fa05661093346644c23f754c8fe8bc3f16b
	SHA512 3d8fb7097af3d90174744ca45cbc5f5f344873517e3082a8413d46d636b7959b96fdf70775177f165d077260252e28a600b49c4acaf53a23e3fd6d1dcafdd7ef
	HEAD_REF master
)

# Update the CMakeLists.txt to correctly install the lib.
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

# Submodules
#	Miniz-cpp
vcpkg_from_github(OUT_SOURCE_PATH MINIZ_CPP_SOURCE_PATH
	REPO nextgeniuspro/miniz-cpp
	REF 8c8d47fac0ae85d83da27a8ad56f10f956913039
	SHA512 0823bbc85bb1ec10716c34e78d7c882afe13ab7c486e621af54bc5caa16fa0fea0cef77332af7373f3537aafcc52ea3ed61fb53d2ea414826d773e73305d70f5
	HEAD_REF master
	PATCHES
		miniz-cmake-bump.patch
)
if(NOT EXISTS "${SOURCE_PATH}/vendor/miniz-cpp/CMakeLists.txt")
	file(REMOVE_RECURSE "${SOURCE_PATH}/vendor/miniz-cpp")
	file(RENAME "${MINIZ_CPP_SOURCE_PATH}" "${SOURCE_PATH}/vendor/miniz-cpp")
endif()

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

# Support includes <3rdparty/header.hxx>
file(COPY "${SOURCE_PATH}/vendor/miniz-cpp/zip_file.hpp" DESTINATION "${CURRENT_PACKAGES_DIR}/include/${PORT}")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
